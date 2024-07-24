#include "librecuda"

#include "librecuda_status.h"
#include "librecuda_status_internal.h"

#include "nvidia/nv_escape.h"
#include "nvidia/nv-ioctl-numbers.h"
#include "nvidia/uvm_linux_ioctl.h"
#include "nvidia/ctrl0000gpu.h"
#include "nvidia/nvos.h"
#include "nvidia/nv-ioctl.h"
#include "nvidia/dev_mmu.h"
#include "nvidia/cl0040.h"
#include "nvidia/cl0080.h"
#include "nvidia/cl2080.h"
#include "nvidia/ctrl2080gpu.h"
#include "nvidia/cl90f1.h"
#include "nvidia/clc461.h"

#include <string>
#include <algorithm>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <vector>
#include <cstring>
#include <set>
#include <unordered_map>

struct LibreCUdevice_ {
    int ordinal;
    NvU32 instance;
    NvProcessorUuid uuid;
};

struct LibreCUcontext_ {
    int device_fd;
    int flags;
    LibreCUdevice device;
    NvHandle device_handle;
    NvU64 uvm_vaddr = 0x1000000000;
};

#define LIBRECUDA_DEBUG(msg) std::cerr << "[LibreCuda Debug]: " << msg << std::endl

#define LIBRECUDA_VALIDATE_UVM_IOCTL(ret, data_ptr) {                                    \
    int return_value = ret;                                                              \
    int status = (data_ptr) != nullptr ? (data_ptr)->rmStatus : return_value;            \
    if (status != 0) {                                                                   \
        LIBRECUDA_DEBUG("UVM_IOCTL failed with return code " + std::to_string(status));  \
        LIBRECUDA_FAIL(LIBRECUDA_ERROR_UNKNOWN);                                         \
    }                                                                                    \
}

static inline int uvm_ioctl(int fd, int cmd, void *data) {
    return ioctl(fd, cmd, data);
}

#define UVM_IOCTL(fd, nr, data, data_size) LIBRECUDA_VALIDATE_UVM_IOCTL(uvm_ioctl(fd, nr, data), data)

#define LIBRECUDA_VALIDATE_NVIOWR(ret) {                                                    \
    int return_value = ret;                                                                 \
    if (return_value != 0) {                                                                \
        LIBRECUDA_DEBUG("NVIOWR failed with return code " + std::to_string(return_value));  \
        LIBRECUDA_FAIL(LIBRECUDA_ERROR_UNKNOWN);                                            \
    }                                                                                       \
}

static inline libreCudaStatus_t nv_iowr(int fd, int nr, void *data, size_t data_size) {
    return ioctl(fd, (3 << 30) | (data_size & 0x1FFF) << 16 | ('F' & 0xFF) << 8 | (nr & 0xFF), data);
}

#define LIBRECUDA_VALIDATE_INITIALIZED() { if (!librecudaInitialized) { LIBRECUDA_FAIL(LIBRECUDA_ERROR_NOT_INITIALIZED); } }

#define NV_IOWR(fd, nr, data, data_size) LIBRECUDA_VALIDATE_NVIOWR(nv_iowr(fd, nr, data, data_size))

static inline libreCudaStatus_t rm_alloc(int fd, NvV32 clss,
                                         NvHandle client, NvHandle parent, NvHandle object,
                                         void *params, NvU32 paramSize,
                                         NvHandle *pObjectNew) {
    LIBRECUDA_VALIDATE(fd > 0, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(pObjectNew != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    NVOS21_PARAMETERS parameters{
            .hRoot=client,
            .hObjectParent=parent,
            .hObjectNew=object,
            .hClass=clss,
            .pAllocParms=params,
            .paramsSize=paramSize
    };
    NV_IOWR(fd, NV_ESC_RM_ALLOC, &parameters, sizeof(parameters));
    *pObjectNew = parameters.hObjectNew;
    if (parameters.status != 0) {
        LIBRECUDA_DEBUG("rm_alloc failed with status: " + std::to_string(parameters.status));
        LIBRECUDA_FAIL(LIBRECUDA_ERROR_UNKNOWN);
    }
    LIBRECUDA_SUCCEED();
}

#define LIBRECUDA_VALIDATE_RM_ALLOC(status) { if (status != LIBRECUDA_SUCCESS) { LIBRECUDA_FAIL(LIBRECUDA_ERROR_UNKNOWN); } }

#define RM_ALLOC(fd, clss, client, parent, object, params, param_size, pObjectNew) LIBRECUDA_VALIDATE_RM_ALLOC(rm_alloc(fd, clss, client, parent, object, params, param_size, pObjectNew));

static inline libreCudaStatus_t rm_ctrl(int fd,
                                        NvV32 cmd,
                                        NvHandle client, NvHandle object,
                                        void *params, NvU32 paramSize) {
    LIBRECUDA_VALIDATE(params != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    NVOS54_PARAMETERS parameters{
            .hClient=client,
            .hObject=object,
            .cmd=cmd,
            .params=params,
            .paramsSize=paramSize
    };
    NV_IOWR(fd, NV_ESC_RM_CONTROL, &parameters, sizeof(parameters));
    if (parameters.status != 0) {
        LIBRECUDA_DEBUG("rm_ctrl failed with status: " + std::to_string(parameters.status));
        LIBRECUDA_FAIL(LIBRECUDA_ERROR_UNKNOWN);
    }
    LIBRECUDA_SUCCEED();
}

#define LIBRECUDA_VALIDATE_RM_CTRL(status) { if (status != LIBRECUDA_SUCCESS) { LIBRECUDA_FAIL(LIBRECUDA_ERROR_UNKNOWN); } }

#define RM_CTRL(fd, cmd, client, object, params, paramSize) LIBRECUDA_VALIDATE_RM_CTRL(rm_ctrl(fd, cmd, client, object, params, paramSize))

NvHandle root{};

static int fd_ctl = 0;
static int fd_uvm = 0;

static int num_devices;
static std::vector<nv_ioctl_card_info_t> devices{};
static std::vector<NvU32> device_instances{};
static std::vector<LibreCUdevice_> public_device_handles;

static bool librecudaInitialized = false;

LibreCUcontext current_ctx = nullptr;
#define LIBRECUDA_ENSURE_CTX_VALID() LIBRECUDA_VALIDATE(current_ctx != nullptr, LIBRECUDA_ERROR_INVALID_CONTEXT);

#define LIBRECUDA_ERR_PROPAGATE(status) { if (status != LIBRECUDA_SUCCESS) { LIBRECUDA_FAIL(status); } }

libreCudaStatus_t libreCuInit(int flags) {
    if (librecudaInitialized) {
        return LIBRECUDA_SUCCESS;
    }

    internalLibreCuInitStatusNames();
    fd_ctl = open("/dev/nvidiactl", O_RDWR | O_CLOEXEC);
    LIBRECUDA_VALIDATE(fd_ctl != -1, LIBRECUDA_ERROR_COMPAT_NOT_SUPPORTED_ON_DEVICE);

    fd_uvm = open("/dev/nvidia-uvm", O_RDWR | O_CLOEXEC);
    LIBRECUDA_VALIDATE(fd_uvm != -1, LIBRECUDA_ERROR_COMPAT_NOT_SUPPORTED_ON_DEVICE);


    // create rm client
    RM_ALLOC(fd_ctl, NV01_ROOT_CLIENT, 0, 0, 0, nullptr, 0, &root);

    // initialize nvidia_uvm
    {
        UVM_INITIALIZE_PARAMS params{.flags=0};
        UVM_IOCTL(fd_uvm, UVM_INITIALIZE, &params, sizeof(params));
    }

    // initialize mm
    {
        int fd_uvm_2 = open("/dev/nvidia-uvm", O_RDWR | O_CLOEXEC);
        UVM_MM_INITIALIZE_PARAMS params{
                .uvmFd=fd_uvm
        };
        UVM_IOCTL(fd_uvm_2, UVM_MM_INITIALIZE, &params, sizeof(params));
    }

    // obtaining basic card info
    {
        constexpr int n_infos = 128; // kernel respects this size; see open-gpu-kernel-modules/kernel-open/nvidia/nv.c:2451
        nv_ioctl_card_info_t card_infos[n_infos];
        NV_IOWR(fd_ctl, NV_ESC_CARD_INFO, card_infos, sizeof(card_infos));

        // count number of valid entries
        int num_gpus = 0;
        for (; num_gpus < n_infos; num_gpus++) {
            if (!card_infos[num_gpus].valid) {
                break;
            }
        }

        // respect CUDA_VISIBLE_DEVICES env variable
        std::set<int> visible_ordinals{};
        {
            const char *env_var_cstr = getenv("CUDA_VISIBLE_DEVICES");
            if (env_var_cstr != nullptr) {
                std::string env_var = std::string(env_var_cstr);
                size_t current_start = 0;

                size_t i;
                for (i = 0; i < env_var.size(); i++) {
                    if (env_var[i] == ',') {
                        std::string ordinal_string = env_var.substr(current_start, i - current_start);
                        size_t pos{};
                        int parsed = std::stoi(ordinal_string, &pos);
                        if (pos != ordinal_string.size()) {
                            continue;
                        }
                        visible_ordinals.insert(parsed);
                        current_start = i + 1;
                    }
                }
                std::string ordinal_string = env_var.substr(current_start, i - current_start);
                visible_ordinals.insert(std::stoi(ordinal_string));
            } else {
                // consider all devices visible
                for (int i = 0; i < num_gpus; i++) {
                    visible_ordinals.insert(i);
                }
            }
        }
        // build valid visible devices
        for (int i = 0; i < num_gpus; i++) {
            if (visible_ordinals.find(i) != visible_ordinals.end()) {
                devices.push_back(card_infos[i]);
            }
        }
        num_devices = num_gpus;
    }

    // obtaining gpu instances
    {
        for (nv_ioctl_card_info_t card_info: devices) {
            NV0000_CTRL_GPU_GET_ID_INFO_V2_PARAMS params{
                    .gpuId=card_info.gpu_id,
            };
            RM_CTRL(fd_ctl, NV0000_CTRL_CMD_GPU_GET_ID_INFO_V2, root, root, &params, sizeof(params));
            device_instances.push_back(params.deviceInstance);
        }
    }

    // creating public device handles
    {
        for (int ordinal = 0; ordinal < num_devices; ordinal++) {
            NvU32 device_instance = device_instances[ordinal];
            public_device_handles.push_back(LibreCUdevice_{
                    .ordinal=ordinal,
                    .instance=device_instance
            });
        }
    }

    librecudaInitialized = true;
    LIBRECUDA_SUCCEED();
}

static std::string getDeviceFile(LibreCUdevice device) {
    return "/dev/nvidia" + std::to_string(device->instance);
}

libreCudaStatus_t libreCuDeviceGetCount(int *pDeviceCount) {
    LIBRECUDA_VALIDATE_INITIALIZED();
    LIBRECUDA_VALIDATE(pDeviceCount != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    *pDeviceCount = num_devices;
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t libreCuDeviceGet(LibreCUdevice *device, int deviceOrdinal) {
    LIBRECUDA_VALIDATE_INITIALIZED();
    LIBRECUDA_VALIDATE(device != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(deviceOrdinal >= 0 && deviceOrdinal < num_devices, LIBRECUDA_ERROR_INVALID_DEVICE);
    *device = &public_device_handles[deviceOrdinal];
    LIBRECUDA_SUCCEED();
}


libreCudaStatus_t libreCuGetErrorString(libreCudaStatus_t status, const char **pStatusName) {
    if (!internalLibreCuInitStatusNamesInitialized()) {
        // we want to tolerate this failure to easy debugging for the user to not descend into madness
        internalLibreCuInitStatusNames();
    }
    const char *status_name = internalLibreCuGetStatusName(static_cast<int>(status));
    LIBRECUDA_VALIDATE(pStatusName != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    *pStatusName = status_name;
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t libreCuCtxCreate_v2(LibreCUcontext *pCtx, int flags, LibreCUdevice device) {
    LIBRECUDA_VALIDATE_INITIALIZED();
    LIBRECUDA_VALIDATE(pCtx != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(device != nullptr, LIBRECUDA_ERROR_INVALID_DEVICE);
    LIBRECUDA_VALIDATE(device->ordinal >= 0 && device->ordinal < num_devices, LIBRECUDA_ERROR_INVALID_DEVICE);
    LIBRECUDA_VALIDATE(
            std::find(device_instances.begin(), device_instances.end(), device->instance) != device_instances.end(),
            LIBRECUDA_ERROR_INVALID_DEVICE
    );

    std::string device_file = getDeviceFile(device);

    auto *ctx = new LibreCUcontext_{
            .flags=flags,
            .device = &public_device_handles[device->ordinal]
    };

    // create & register device device fd
    {
        int device_fd = open(device_file.c_str(), O_RDWR | O_CLOEXEC);
        nv_ioctl_register_fd_t params{
                .ctl_fd=fd_ctl
        };
        NV_IOWR(device_fd, NV_ESC_REGISTER_FD, &params, sizeof(params));
        ctx->device_fd = device_fd;
    }

    // create device rm handle
    NvHandle device_handle{};
    {
        NV0080_ALLOC_PARAMETERS params{
                .deviceId=device->instance,
                .hClientShare=root,
                .vaMode=NV_DEVICE_ALLOCATION_VAMODE_MULTIPLE_VASPACES
        };
        RM_ALLOC(fd_ctl, NV01_DEVICE_0, root, root, 0, &params, sizeof(params), &device_handle);
        ctx->device_handle = device_handle;
    }

    // create nv20 sub-device handle
    NvHandle sub_device_handle{};
    RM_ALLOC(fd_ctl, NV20_SUBDEVICE_0, root, device_handle, 0, nullptr, 0, &sub_device_handle);

    // create turing user mode handle
    NvHandle user_mode_handle{};
    RM_ALLOC(fd_ctl, TURING_USERMODE_A, root, sub_device_handle, 0, nullptr, 0, &user_mode_handle);

    // obtain device uuid
    {
        NV2080_CTRL_GPU_GET_GID_INFO_PARAMS params{
                .flags=NV2080_GPU_CMD_GPU_GET_GID_FLAGS_FORMAT_BINARY,
                .length=16
        };
        RM_CTRL(fd_ctl, NV2080_CTRL_CMD_GPU_GET_GID_INFO, root, sub_device_handle, &params, sizeof(params));
        memcpy(device->uuid.uuid, params.data, 16);
    }

    // create va space
    NvHandle va_space{};
    {
        NV_VASPACE_ALLOCATION_PARAMETERS params{
                .index=NV_VASPACE_ALLOCATION_INDEX_GPU_NEW,
                .flags = NV_VASPACE_ALLOCATION_FLAGS_ENABLE_PAGE_FAULTING |
                         NV_VASPACE_ALLOCATION_FLAGS_IS_EXTERNALLY_OWNED,
                .vaSize = 0x1fffffb000000,
                .bigPageSize = 0, // use system default
                .vaBase = 0x1000,
        };
        RM_ALLOC(fd_ctl, FERMI_VASPACE_A, root, device_handle, 0, &params, sizeof(params), &va_space);
    }

    // register gpu
    {
        UVM_REGISTER_GPU_PARAMS params{
                .gpu_uuid=device->uuid,
                .rmCtrlFd = -1,
        };
        UVM_IOCTL(fd_uvm, UVM_REGISTER_GPU, &params, sizeof(params));
    }

    // register gpu virtual address space
    {
        UVM_REGISTER_GPU_VASPACE_PARAMS params{
                .gpuUuid=device->uuid,
                .rmCtrlFd=fd_ctl,
                .hClient=root,
                .hVaSpace=va_space
        };
        UVM_IOCTL(fd_uvm, UVM_REGISTER_GPU_VASPACE, &params, sizeof(params));
    }

    *pCtx = ctx;
    current_ctx = ctx;

    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t libreCuCtxDestroy(LibreCUcontext ctx) {
    LIBRECUDA_VALIDATE(ctx != nullptr, LIBRECUDA_ERROR_INVALID_CONTEXT);

    // set current context to none, if current context is equal to the context to destroy
    if (current_ctx == ctx) {
        current_ctx = nullptr;
    }

    // close device fd
    close(ctx->device_fd);
    delete ctx;

    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t libreCuCtxSetCurrent(LibreCUcontext ctx) {
    LIBRECUDA_VALIDATE(ctx != nullptr, LIBRECUDA_ERROR_INVALID_CONTEXT);
    current_ctx = ctx;
    LIBRECUDA_SUCCEED();
}

static NvU64 ceilDiv(NvU64 a, NvU64 b) {
    return (a + b - 1) / b;
}

static std::unordered_map<NvU64, NvHandle> va_to_mem_handle{};

static libreCudaStatus_t
gpuAlloc(LibreCUcontext ctx, size_t size, bool physicalContiguity, bool hugePages, NvU64 *pVaOut) {
    NvU64 alignment = hugePages ? (2 << 20) : (4 << 10);
    size = ceilDiv(size, alignment) * alignment; // round up to next multiple of alignment

    // perform allocation
    NvHandle memory_handle{};
    {
        NV_MEMORY_ALLOCATION_PARAMS params{
                .owner=root,
                .attr=static_cast<NvU32>((hugePages ? (NVOS32_ATTR_PAGE_SIZE_HUGE << 23) : 0) |
                                         ((physicalContiguity ? NVOS32_ATTR_PHYSICALITY_CONTIGUOUS
                                                              : NVOS32_ATTR_PHYSICALITY_ALLOW_NONCONTIGUOUS) << 27)),
                .format=NV_MMU_PTE_KIND_GENERIC_MEMORY,
                .size=size,
                .alignment=alignment,
                .offset=0,
                .limit=size - 1,
        };
        RM_ALLOC(fd_ctl, NV1_MEMORY_USER, root, ctx->device_handle, 0, &params, sizeof(params), &memory_handle);
    }

    // bump allocate virtual address
    NvU64 va_address;
    {
        va_address = ceilDiv(ctx->uvm_vaddr, alignment) * alignment;
        ctx->uvm_vaddr = va_address + size;
    }

    // create external range
    {
        UVM_CREATE_EXTERNAL_RANGE_PARAMS params{
                .base=va_address,
                .length=size
        };
        UVM_IOCTL(fd_uvm, UVM_CREATE_EXTERNAL_RANGE, &params, sizeof(params));
    }

    // map obtained memory handle to our bumped pointer
    {
        UVM_MAP_EXTERNAL_ALLOCATION_PARAMS params{
                .base=va_address,
                .length=size,
                .perGpuAttributes={
                        {
                                .gpuUuid=ctx->device->uuid,
                                .gpuMappingType=UvmGpuMappingTypeReadWriteAtomic
                        }
                },
                .gpuAttributesCount=1,
                .rmCtrlFd=fd_ctl,
                .hClient=root,
                .hMemory=memory_handle
        };
        UVM_IOCTL(fd_uvm, UVM_MAP_EXTERNAL_ALLOCATION, &params, sizeof(params));
    }
    *pVaOut = va_address;
    va_to_mem_handle[va_address] = memory_handle;
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t libreCuMemAlloc(void **pDevicePointer, size_t bytesize) {
    LIBRECUDA_VALIDATE(pDevicePointer != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(bytesize > 0, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_ENSURE_CTX_VALID();
    NvU64 va_address{};
    {
        bool hugePages = bytesize > (16 << 20);
        gpuAlloc(current_ctx, bytesize, false, hugePages, &va_address);
    }
    *pDevicePointer = reinterpret_cast<void *>(va_address);
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t gpuFree(LibreCUcontext ctx, NvU64 virtualAddress) {
    auto it = va_to_mem_handle.find(virtualAddress);
    if (it == va_to_mem_handle.end()) {
        LIBRECUDA_FAIL(LIBRECUDA_ERROR_INVALID_VALUE);
    }
    NvHandle mem_handle = it->second;
    NVOS00_PARAMETERS params{
            .hRoot = root,
            .hObjectParent = ctx->device_handle,
            .hObjectOld = mem_handle
    };
    NV_IOWR(fd_ctl, NV_ESC_RM_FREE, &params, sizeof(params));

    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t libreCuMemFree(void *devicePointer) {
    LIBRECUDA_VALIDATE(devicePointer != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_ENSURE_CTX_VALID();

    LIBRECUDA_ERR_PROPAGATE(gpuFree(current_ctx, reinterpret_cast<NvU64>(devicePointer)));

    LIBRECUDA_SUCCEED();
}
