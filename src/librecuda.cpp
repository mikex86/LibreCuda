#include "librecuda.h"

#include "librecuda_status.h"
#include "librecuda_status_internal.h"

#include "librecuda_internal.h"
#include "cmdqueue.h"

#include "nvidia/nv-unix-nvos-params-wrappers.h"
#include "nvidia/nv_escape.h"
#include "nvidia/nv-ioctl-numbers.h"
#include "nvidia/uvm_linux_ioctl.h"
#include "nvidia/ctrl0000gpu.h"
#include "nvidia/nvos.h"
#include "nvidia/nv-ioctl.h"
#include "nvidia/dev_mmu.h"
#include "nvidia/cl0080.h"
#include "nvidia/cl2080.h"
#include "nvidia/ctrl2080gpu.h"

#include "nvidia/clc56f.h"
#include "nvidia/g_allclasses.h"
#include "nvidia/ctrl0080gpu.h"
#include "nvidia/ctrlc36f.h"
#include "nvidia/ctrla06c.h"

#include <elfio/elfio.hpp>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <string>
#include <algorithm>
#include <sstream>
#include <vector>
#include <cstring>
#include <set>
#include <unordered_map>
#include <unordered_set>

#define GPU_FIFO_AREA_SIZE 0x200000

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

libreCudaStatus_t rm_ctrl(int fd, NvV32 cmd, NvHandle client, NvHandle object, void *params, NvU32 paramSize) {
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

static inline NvU64 ceilDiv(NvU64 a, NvU64 b) {
    return (a + b - 1) / b;
}

static inline NvU32 maxOf(NvU32 a, NvU32 b) {
    return (a > b) ? a : b;
}

NvU64 bump_alloc_virtual_addr(LibreCUcontext ctx, size_t size, NvU32 alignment = 4 << 10) {
    NvU64 va_address = ceilDiv(ctx->uvm_vaddr, alignment) * alignment;
    ctx->uvm_vaddr = va_address + size;
    return va_address;
}

libreCudaStatus_t
makeGpuFifo(LibreCUcontext ctx,
            NvU64 gpfifoAreaVa, NvHandle gpfifoAreaHandle,
            NvHandle ctxShare, NvHandle channelGroup,
            NvU64 offset, NvU32 entries,
            GPFifo *pGPFifoOut) {
    LIBRECUDA_VALIDATE(ctx != nullptr, LIBRECUDA_ERROR_UNKNOWN);
    LIBRECUDA_VALIDATE(gpfifoAreaVa != 0, LIBRECUDA_ERROR_UNKNOWN);
    LIBRECUDA_VALIDATE(gpfifoAreaHandle != 0, LIBRECUDA_ERROR_UNKNOWN);
    LIBRECUDA_VALIDATE(pGPFifoOut != nullptr, LIBRECUDA_ERROR_UNKNOWN);

    void *notifier_va{};
    NvHandle notifier_memory_handle{};
    {
        LIBRECUDA_ERR_PROPAGATE(
                gpuSystemAlloc(
                        ctx,
                        48 << 20, true, 0,
                        reinterpret_cast<NvU64 *>(&notifier_va),
                        &notifier_memory_handle
                )
        );
    }

    NvHandle gpfifo{};
    {
        NV_CHANNELGPFIFO_ALLOCATION_PARAMETERS params{
                .hObjectError=notifier_memory_handle,
                .hObjectBuffer=gpfifoAreaHandle,
                .gpFifoOffset=gpfifoAreaVa + offset,
                .gpFifoEntries=entries,
                .hContextShare=ctxShare,
                .hUserdMemory={
                        gpfifoAreaHandle
                },
                .userdOffset={
                        entries * 8 + offset
                }
        };
        RM_ALLOC(fd_ctl, AMPERE_CHANNEL_GPFIFO_A, root, channelGroup, 0, &params, sizeof(params), &gpfifo);
    }

    RM_ALLOC(fd_ctl, ctx->device->compute_class, root, gpfifo, 0, nullptr, 0, nullptr);
    RM_ALLOC(fd_ctl, AMPERE_DMA_COPY_B, root, gpfifo, 0, nullptr, 0, nullptr);

    // get work submit token
    NvU32 work_submit_token;
    {
        NVC36F_CTRL_CMD_GPFIFO_GET_WORK_SUBMIT_TOKEN_PARAMS params{.workSubmitToken=0xFFFFFFFF};
        RM_CTRL(fd_ctl, NVC36F_CTRL_CMD_GPFIFO_GET_WORK_SUBMIT_TOKEN, root, gpfifo, &params, sizeof(params));
        LIBRECUDA_VALIDATE(params.workSubmitToken != 0xFFFFFFFF, LIBRECUDA_ERROR_UNKNOWN);
        work_submit_token = params.workSubmitToken;
    }

    NvU64 channel_base_va = bump_alloc_virtual_addr(ctx, 0x4000000);
    {
        UVM_REGISTER_CHANNEL_PARAMS params{
                .gpuUuid = ctx->device->uuid,
                .rmCtrlFd=fd_ctl,
                .hClient=root,
                .hChannel=gpfifo,
                .base=channel_base_va,
                .length=0x4000000
        };
        UVM_IOCTL(fd_uvm, UVM_REGISTER_CHANNEL, &params, sizeof(UVM_REGISTER_CHANNEL_PARAMS));
    }

    GPFifo fifo{
            .ring=reinterpret_cast<NvU64 *>(gpfifoAreaVa + offset),
            .entries_count=entries,
            .token=work_submit_token,
            .controls = reinterpret_cast<AmpereAControlGPFifo *>(gpfifoAreaVa + offset + entries * 8)
    };
    *pGPFifoOut = fifo;
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t
memMapToCpu(LibreCUcontext ctx, NvHandle memoryHandle, size_t size, NvU64 cpuVirtualAddress, NvU32 mapFlags,
            bool isSystemAlloc) {
    LIBRECUDA_VALIDATE(ctx != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(memoryHandle != 0, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(cpuVirtualAddress != 0, LIBRECUDA_ERROR_INVALID_VALUE);

    int device_fd;
    if (isSystemAlloc) {
        device_fd = open("/dev/nvidiactl", O_RDWR | O_CLOEXEC);
        LIBRECUDA_VALIDATE(fd_ctl != -1, LIBRECUDA_ERROR_INVALID_DEVICE);
    } else {
        std::string device_file = getDeviceFile(ctx->device);

        device_fd = open(device_file.c_str(), O_RDWR | O_CLOEXEC);
        LIBRECUDA_VALIDATE(device_fd != -1, LIBRECUDA_ERROR_INVALID_DEVICE);
        nv_ioctl_register_fd_t params{
                .ctl_fd=fd_ctl
        };
        NV_IOWR(device_fd, NV_ESC_REGISTER_FD, &params, sizeof(params));
    }

    nv_ioctl_nvos33_parameters_with_fd parameters{
            .params={
                    .hClient=root,
                    .hDevice=ctx->device_handle,
                    .hMemory=memoryHandle,
                    .length=size,
                    .flags=mapFlags
            },
            .fd=device_fd
    };
    NV_IOWR(fd_ctl, NV_ESC_RM_MAP_MEMORY, &parameters, sizeof(parameters));
    LIBRECUDA_VALIDATE(
            mmap(reinterpret_cast<void *>(cpuVirtualAddress), size,
                 PROT_READ | PROT_WRITE,
                 MAP_SHARED | MAP_FIXED,
                 device_fd, 0
            ) != MAP_FAILED,
            LIBRECUDA_ERROR_UNKNOWN
    );
    close(device_fd);

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

    // create gpu mmio region
    NvU32 *gpu_mmio;
    {
        size_t mmio_sz = 0x10000;
        gpu_mmio = reinterpret_cast<NvU32 *>(bump_alloc_virtual_addr(ctx, mmio_sz));
        LIBRECUDA_ERR_PROPAGATE(
                memMapToCpu(
                        ctx,
                        user_mode_handle, mmio_sz,
                        reinterpret_cast<NvU64>(gpu_mmio), 2,
                        false
                )
        );
    }
    device->gpu_mmio = gpu_mmio;

    // obtain device uuid
    {
        NV2080_CTRL_GPU_GET_GID_INFO_PARAMS params{
                .flags=NV2080_GPU_CMD_GPU_GET_GID_FLAGS_FORMAT_BINARY,
                .length=16
        };
        RM_CTRL(fd_ctl, NV2080_CTRL_CMD_GPU_GET_GID_INFO, root, sub_device_handle, &params, sizeof(params));
        memcpy(device->uuid.uuid, params.data, 16);
    }

    // create virtual address space
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
    ctx->va_space_handle = va_space;

    // create channel group
    NvHandle channel_group{};
    {
        NV_CHANNEL_GROUP_ALLOCATION_PARAMETERS params{
                .engineType=NV2080_ENGINE_TYPE_GRAPHICS
        };
        RM_ALLOC(fd_ctl, KEPLER_CHANNEL_GROUP_A, root, device_handle, 0, &params, sizeof(params),
                 &channel_group);
    }
    ctx->channel_group = channel_group;

    // create ctx share
    NvHandle ctx_share{};
    {
        NV_CTXSHARE_ALLOCATION_PARAMETERS params{
                .hVASpace = ctx->va_space_handle,
                .flags = NV_CTXSHARE_ALLOCATION_FLAGS_SUBCONTEXT_ASYNC
        };
        RM_ALLOC(fd_ctl, FERMI_CONTEXT_SHARE_A, root, channel_group, 0, &params, sizeof(params), &ctx_share);
    }
    ctx->ctx_share = ctx_share;

    // get compute class
    {
        NvU32 class_list[NV0080_CTRL_GPU_CLASSLIST_MAX_SIZE];
        NV0080_CTRL_GPU_GET_CLASSLIST_PARAMS params{
                .numClasses=NV0080_CTRL_GPU_CLASSLIST_MAX_SIZE,
                .classList=class_list
        };
        RM_CTRL(fd_ctl, NV0080_CTRL_CMD_GPU_GET_CLASSLIST, root, device_handle, &params, sizeof(params));

        NvU32 compute_class = 0;
        for (NvU32 class_id: class_list) {
            switch (class_id) {
                case ADA_COMPUTE_A:
                case AMPERE_COMPUTE_B:
                    compute_class = class_id;
                    goto loop_end;
                default:
                    break;
            }
        }
        loop_end:
        ctx->device->compute_class = compute_class;
    }

    // create compute & dma gpfifo
    GPFifo compute_gpfifo{};
    GPFifo dma_gpfifo{};
    {
        NvU64 gpfifo_area_va{};
        NvHandle gpfifo_area_handle{};

        // alloc gpfifo area
        {
            // CUDA maps with 0x10d0000
            // but NVOS33_FLAGS_CACHING_TYPE_UNCACHED is ignored in kernel when calling from userspace?
            // also NVOS33_FLAGS_RESERVE_ON_UNMAP_ENABLE allows dangling pointer to remain valid but not mapped to GPU? why?
            LIBRECUDA_ERR_PROPAGATE(
                    gpuAlloc(ctx, GPU_FIFO_AREA_SIZE, true, true, true,
                             (NVOS33_FLAGS_MAPPING_DIRECT << 16) | (NVOS33_FLAGS_MAP_FIXED_ENABLE << 18) |
                             (NVOS33_FLAGS_RESERVE_ON_UNMAP_ENABLE << 19) |
                             (NVOS33_FLAGS_CACHING_TYPE_WRITECOMBINED << 23),
                             &gpfifo_area_va,
                             &gpfifo_area_handle
                    )
            );
        }

        // create compute gpfifo
        {
            LIBRECUDA_ERR_PROPAGATE(
                    makeGpuFifo(
                            ctx,
                            gpfifo_area_va, gpfifo_area_handle,
                            ctx_share, channel_group,
                            0, 0x10000,
                            &compute_gpfifo
                    )
            );
        }

        // create dma gpfifo
        {
            LIBRECUDA_ERR_PROPAGATE(
                    makeGpuFifo(
                            ctx,
                            gpfifo_area_va, gpfifo_area_handle,
                            ctx_share, channel_group,
                            0x100000, 0x10000,
                            &dma_gpfifo
                    )
            );
        }
    }
    // enable gpfifo schedule
    {
        NVA06C_CTRL_GPFIFO_SCHEDULE_PARAMS params{.bEnable = true};
        RM_CTRL(fd_ctl, NVA06C_CTRL_CMD_GPFIFO_SCHEDULE, root, channel_group, &params, sizeof(params));
    }
    ctx->compute_gpfifo = compute_gpfifo;
    ctx->dma_gpfifo = dma_gpfifo;

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

static std::unordered_map<NvU64, NvHandle> va_to_mem_handle{};


libreCudaStatus_t memUVMMap(LibreCUcontext ctx, NvHandle memoryHandle, NvU64 virtualAddress, size_t size) {
    LIBRECUDA_VALIDATE(ctx != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(memoryHandle != 0, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(virtualAddress != 0, LIBRECUDA_ERROR_INVALID_VALUE);

    // create external range
    {
        UVM_CREATE_EXTERNAL_RANGE_PARAMS params{
                .base=virtualAddress,
                .length=size
        };
        UVM_IOCTL(fd_uvm, UVM_CREATE_EXTERNAL_RANGE, &params, sizeof(params));
    }

    // map obtained memory handle to the target virtual address
    {
        UVM_MAP_EXTERNAL_ALLOCATION_PARAMS params{
                .base=virtualAddress,
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
                .hMemory=memoryHandle
        };
        UVM_IOCTL(fd_uvm, UVM_MAP_EXTERNAL_ALLOCATION, &params, sizeof(params));
    }
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t
gpuAlloc(LibreCUcontext ctx, size_t size, bool physicalContiguity, bool hugePages, bool mapToCpu, NvU32 mapFlags,
         NvU64 *pVaOut, NvHandle *pMemoryHandleOut) {
    LIBRECUDA_VALIDATE(ctx != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(pVaOut != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);

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
                .attr2 = static_cast<NvU32>(((NVOS32_ATTR2_ZBC_PREFER_NO_ZBC << 0) |
                                             (NVOS32_ATTR2_GPU_CACHEABLE_YES << 2) |
                                             (hugePages ? (NVOS32_ATTR2_PAGE_SIZE_HUGE_2MB << 20) : 0))),
                .format=NV_MMU_PTE_KIND_GENERIC_MEMORY,
                .size=size,
                .alignment=alignment,
                .offset=0,
                .limit=size - 1
        };
        RM_ALLOC(fd_ctl, NV1_MEMORY_USER, root, ctx->device_handle, 0, &params, sizeof(params), &memory_handle);
    }

    // bump allocate virtual address
    NvU64 va_address = bump_alloc_virtual_addr(ctx, size, alignment);

    // map obtained virtual address to refer to the memory allocated
    LIBRECUDA_ERR_PROPAGATE(memUVMMap(ctx, memory_handle, va_address, size));

    // map to cpu if requested
    if (mapToCpu) {
        LIBRECUDA_ERR_PROPAGATE(memMapToCpu(ctx, memory_handle, size, va_address, mapFlags, false));
    }

    *pVaOut = va_address;

    if (pMemoryHandleOut != nullptr) {
        *pMemoryHandleOut = memory_handle;
    }
    va_to_mem_handle[va_address] = memory_handle;
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t libreCuMemAlloc(void **pDevicePointer, size_t bytesize, bool mapToCpu) {
    LIBRECUDA_VALIDATE(pDevicePointer != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(bytesize > 0, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_ENSURE_CTX_VALID();
    NvU64 va_address{};
    {
        bool hugePages = bytesize > (16 << 20);
        gpuAlloc(current_ctx, bytesize, false, hugePages, mapToCpu, 0, &va_address);
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

libreCudaStatus_t
gpuSystemAlloc(LibreCUcontext ctx, size_t size, bool mapToCpu, NvU32 mapFlags, NvU64 *pVaOut,
               NvHandle *pMemHandleOut) {
    LIBRECUDA_VALIDATE(pVaOut != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(ctx != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);

    NvHandle memory_handle{};
    {
        NV_MEMORY_ALLOCATION_PARAMS params{
                .owner=root,
                .type=NVOS32_TYPE_NOTIFIER,
                .flags=(NVOS32_ALLOC_FLAGS_IGNORE_BANK_PLACEMENT | NVOS32_ALLOC_FLAGS_MEMORY_HANDLE_PROVIDED |
                        NVOS32_ALLOC_FLAGS_MAP_NOT_REQUIRED),
                .attr=(NVOS32_ATTR_PHYSICALITY_ALLOW_NONCONTIGUOUS << 27) | (NVOS32_ATTR_LOCATION_PCI << 25),
                .attr2=(NVOS32_ATTR2_ZBC_PREFER_NO_ZBC << 0) | (NVOS32_ATTR2_GPU_CACHEABLE_NO << 2),
                .format=6,
                .size=size,
                .alignment=(4 << 10),
                .offset=0,
                .limit=size - 1
        };
        RM_ALLOC(fd_ctl, NV1_MEMORY_SYSTEM, root, ctx->device_handle, 0, &params, sizeof(params), &memory_handle);
    }

    // bump allocate virtual address
    NvU64 va_address = bump_alloc_virtual_addr(ctx, size, 4 << 10);

    // map to cpu if requested
    if (mapToCpu) {
        LIBRECUDA_ERR_PROPAGATE(memMapToCpu(ctx, memory_handle, size, va_address, mapFlags, true));
    }

    // map obtained virtual address to refer to the memory allocated
    LIBRECUDA_ERR_PROPAGATE(memUVMMap(ctx, memory_handle, va_address, size));

    *pVaOut = va_address;

    if (pMemHandleOut != nullptr) {
        *pMemHandleOut = memory_handle;
    }

    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t libreCuMemFree(void *devicePointer) {
    LIBRECUDA_VALIDATE(devicePointer != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_ENSURE_CTX_VALID();

    LIBRECUDA_ERR_PROPAGATE(gpuFree(current_ctx, reinterpret_cast<NvU64>(devicePointer)));

    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t libreCuMemCpy(void *dst, void *src, size_t byteCount) {
    LIBRECUDA_VALIDATE(dst != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(src != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_ENSURE_CTX_VALID();


    LIBRECUDA_SUCCEED();
}


struct RelocInfo {
    ELFIO::Elf64_Addr apply_image_offset;
    ELFIO::Elf64_Addr rel_sym_offset;
    uint32_t typ;
};

#define EIATTR_MIN_STACK_SIZE 0x1204

// Constants for attributes defined in .nv.info.<func_name> sections
// along with their size in increments of 32-bit word (section is .align 4)
#define EIATTR_CUDA_API_VERSION 0x3704
#define EIATTR_CUDA_API_VERSION_ATTR_WORD_LEN 2

#define EIATTR_PARAM_CBANK 0x0a04
#define EIATTR_PARAM_CBANK_ATTR_WORD_LEN 3

#define EIATTR_CBANK_PARAM_SIZE 0x1903
#define EIATTR_CBANK_PARAM_SIZE_ATTR_WORD_LEN 1

#define EIATTR_KPARAM_INFO 0x1704
#define EIATTR_KPARAM_INFO_ATTR_WORD_LEN 4

#define EIATTR_MAXREG_COUNT 0x1b03
#define EIATTR_MAXREG_COUNT_ATTR_WORD_LEN 4

#define EIATTR_EXIT_INSTR_OFFSETS 0x1c04
#define EIATTR_EXIT_INSTR_ATTR_WORD_LEN 4


libreCudaStatus_t libreCuModuleLoadData(LibreCUmodule *pModule, const void *image, size_t imageSize) {
    LIBRECUDA_VALIDATE(pModule != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(image != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_ENSURE_CTX_VALID();

    // parse image as elf binary
    ELFIO::elfio elf_reader{};
    {
        const auto *data = reinterpret_cast<const char *>(image);
        std::string s(reinterpret_cast<const char *>(data), imageSize);
        std::istringstream istream(s);
        elf_reader.load(istream);
    }

    // allocate memory for the program
    NvU64 module_gpu_va{};

    // Ensure at least 4KB of space after the program to mitigate prefetch memory faults.
    size_t allocated_size = ceilDiv(imageSize, 0x1000) * 0x1000 + 0x1000;
    LIBRECUDA_ERR_PROPAGATE(
            gpuAlloc(current_ctx, allocated_size, false, false, true, 0, &module_gpu_va)
    );

    std::unordered_set<std::string> function_names{}; // list of all function names
    std::unordered_map<std::string, NvU32> function_argcs{}; // maps function names to the number of input parameters they request
    std::unordered_map<std::string, NvU32> function_shared_mem{}; // maps function names to their shared memory requirements
    std::unordered_map<std::string, NvU64> function_addrs{}; // maps function names to their addresses in the uploaded memory
    std::unordered_map<std::string, NvU32> function_regs{}; // maps function names to their register requirements (n registers)
    std::unordered_map<std::string, NvU32> function_local_mem_reqs{}; // maps function names to their local mem requirements
    std::unordered_map<std::string, NvU64> function_sizes{}; // maps function names to their function sizes in bytes
    std::unordered_map<std::string, std::vector<KernelConstantInfo>> constants{}; // maps function names to their list of constants
    std::unordered_map<std::string, std::vector<KernelParamInfo>> param_infos{}; // maps function names to function parameter info
    std::unordered_map<std::string, size_t> param_total_sizes{}; // maps function names to their total parameter size

    // iterate over sections
    std::vector<RelocInfo> relocs{};

    // find symbol table
    size_t symtab_idx{};
    {
        size_t idx = 0;
        for (const auto &s: elf_reader.sections) {
            if (s->get_type() == ELFIO::SHT_SYMTAB) {
                symtab_idx = idx;
                break;
            }
            idx++;
        }
    }

    for (const auto &section: elf_reader.sections) {
        const std::string &section_name = section->get_name();
        // prepare code relocations
        if (section->get_type() == ELFIO::SHT_REL || section->get_type() == ELFIO::SHT_RELA) {
            std::string target_section_name{};
            if (section->get_type() == ELFIO::SHT_REL) {
                LIBRECUDA_VALIDATE(section_name.substr(0, 4) == ".rel", LIBRECUDA_ERROR_INVALID_IMAGE);
                target_section_name = section_name.substr(5);
            } else if (section->get_type() == ELFIO::SHT_RELA) {
                LIBRECUDA_VALIDATE(section_name.substr(0, 5) == ".rela", LIBRECUDA_ERROR_INVALID_IMAGE);
                target_section_name = section_name.substr(6);
            }

            // find target section address
            NvU64 target_image_off{};
            for (const auto &s: elf_reader.sections) {
                if (s->get_name() == target_section_name) {
                    target_image_off = s->get_offset();
                    break;
                }
            }

            // get rel info
            {
                ELFIO::relocation_section_accessor rel_accessor(elf_reader, section.get());

                size_t num_entries = rel_accessor.get_entries_num();
                for (ELFIO::Elf_Xword i = 0; i < num_entries; i++) {

                    ELFIO::Elf64_Addr offset{};
                    ELFIO::Elf_Word symbol{};
                    uint32_t type{};
                    ELFIO::Elf_Sxword addend{};

                    LIBRECUDA_VALIDATE(
                            rel_accessor.get_entry(i, offset, symbol, type, addend),
                            LIBRECUDA_ERROR_INVALID_IMAGE
                    );

                    ELFIO::Elf64_Addr sym_section_offs;
                    ELFIO::Elf64_Addr st_value{};
                    {
                        auto sym_tab = elf_reader.sections[symtab_idx];
                        ELFIO::symbol_section_accessor sym_acc{elf_reader, sym_tab};

                        std::string name{};
                        ELFIO::Elf_Xword size{};
                        uint8_t bind{};
                        uint8_t st_type{};
                        ELFIO::Elf_Half section_index{};
                        uint8_t other{};

                        LIBRECUDA_VALIDATE(
                                sym_acc.get_symbol(symbol, name, st_value, size, bind, st_type, section_index, other),
                                LIBRECUDA_ERROR_INVALID_IMAGE
                        );

                        auto s = elf_reader.sections[section_index];
                        sym_section_offs = s->get_offset();
                    }

                    relocs.push_back(RelocInfo{
                            .apply_image_offset=target_image_off + offset,
                            .rel_sym_offset=sym_section_offs + st_value,
                            .typ=type
                    });
                }
            }

        }
        if (section_name.size() > 11 && section_name.substr(0, 11) == ".nv.shared.") {
            std::string function_name = section_name.substr(11);
            function_shared_mem[function_name] = elf_reader.get_section_entry_size();
        } else if (section_name.size() > 6 && section_name.substr(0, 6) == ".text.") {
            std::string function_name = section_name.substr(6);
            function_names.insert(function_name);

            // map to where the function is in the allocated memory
            function_addrs[function_name] = module_gpu_va + section->get_offset();

            // extract number of registers from section info
            function_regs[function_name] = section->get_info() >> 24;

            // map function to its size
            function_sizes[function_name] = section->get_size();
        } else if (section_name.size() > 12 && section_name.substr(0, 12) == ".nv.constant") {
            std::string constant_nr_str = section_name.substr(12);

            size_t end{};
            NvU32 constant_nr = std::stoi(constant_nr_str, &end);
            LIBRECUDA_VALIDATE(constant_nr_str[end] == '.', LIBRECUDA_ERROR_INVALID_IMAGE);

            std::string function_name = constant_nr_str.substr(end + 1);

            auto &func_constants_list = constants[function_name];

            auto const_info = KernelConstantInfo{
                    .address=module_gpu_va + section->get_offset(),
                    .size=section->get_size()
            };
            if (func_constants_list.size() < constant_nr + 1) {
                func_constants_list.resize(constant_nr + 1);
            }
            func_constants_list[constant_nr] = const_info;
        } else if (section_name == ".nv.info") {
            const char *data = section->get_data();
            for (int off = 0; off < section->get_size(); off += (sizeof(NvU32) * 3)) {
                const auto *line = reinterpret_cast<const NvU32 *>(data + off);
                NvU32 type = line[0];
                NvU32 func = line[1];
                NvU32 value = line[2];

                if ((type & 0xffff) == EIATTR_MIN_STACK_SIZE) {
                    auto target = elf_reader.sections[func +
                                                      5]; // ignores null section, .shstrtab, .strtab, .symtab & .debug_frame
                    auto target_name = target->get_name();

                    NvU32 local_mem_req = value + 0x240;

                    NvU32 max_local_mem_req = function_local_mem_reqs[target_name];
                    function_local_mem_reqs[target_name] = maxOf(max_local_mem_req, local_mem_req);
                }
            }
        } else if (section_name.size() > 9 && section_name.substr(0, 9) == ".nv.info.") {
            auto target_function_name = section_name.substr(9);
            const char *data = section->get_data();

            for (int off = 0; off < section->get_size();) {
                const auto *line = reinterpret_cast<const NvU32 *>(data + off);
                NvU32 key = line[0];
                NvU16 type = key & 0xffff;
                switch (type) {
                    case EIATTR_CUDA_API_VERSION: {
                        off += (EIATTR_CUDA_API_VERSION_ATTR_WORD_LEN * sizeof(NvU32));
                        break;
                    }
                    case EIATTR_PARAM_CBANK: {
                        off += (EIATTR_PARAM_CBANK_ATTR_WORD_LEN * sizeof(NvU32));
                        break;
                    }
                    case EIATTR_CBANK_PARAM_SIZE: {
                        off += (EIATTR_CBANK_PARAM_SIZE_ATTR_WORD_LEN * sizeof(NvU32));
                        size_t total_size = (key >> 16) & 0xffff;
                        param_total_sizes[target_function_name] = total_size;
                        break;
                    }
                    case EIATTR_KPARAM_INFO: {
                        NvU32 attr_data = line[2];
                        NvU16 param_idx = attr_data & 0xffff;
                        NvU16 param_offset = (attr_data >> 16) & 0xffff;
                        off += (EIATTR_KPARAM_INFO_ATTR_WORD_LEN * sizeof(NvU32));
                        param_infos[target_function_name].push_back(KernelParamInfo{
                                .param_index=param_idx,
                                .param_offset=param_offset
                        });
                        break;
                    }
                    case EIATTR_MAXREG_COUNT: {
                        off += (EIATTR_MAXREG_COUNT_ATTR_WORD_LEN * sizeof(NvU32));
                        break;
                    }
                    case EIATTR_EXIT_INSTR_OFFSETS: {
                        off += (EIATTR_EXIT_INSTR_OFFSETS * sizeof(NvU32));
                        break;
                    }
                    default: {
                        // TODO: This isn't strictly correct,
                        //  it just means we don't know the attribute yet.
                        //  But we HAVE to abort here, because of the variable length nature of attributes
                        LIBRECUDA_FAIL(LIBRECUDA_ERROR_INVALID_IMAGE)
                    }
                }
            }
        }
    }

    // finalize parameter info
    for (const auto &function_name: function_names) {
        auto &params = param_infos[function_name];

        // sort by param_index
        {
            auto compare = [](const KernelParamInfo &a, const KernelParamInfo &b) {
                return a.param_index < b.param_index;
            };

            std::sort(params.begin(), params.end(), compare);
        }

        // compute parameter sizes
        for (size_t i = 0; i < params.size(); i++) {
            auto &current = params[i];
            if (i + 1 < params.size()) {
                const auto &next = params[i + 1];
                current.param_size = next.param_offset - current.param_offset;
            } else {
                size_t total_param_size = param_total_sizes[function_name];
                current.param_size = total_param_size - current.param_offset;
            }
        }
    }

    std::vector<LibreCUFunction_> functions{};
    functions.reserve(function_names.size());
    for (const auto &func_name: function_names) {
        // TODO: validate each of those map lookups and throw invalid image errors
        functions.push_back(LibreCUFunction_{
                .name=func_name,
                .argc=function_argcs[func_name],
                .func_va_addr=function_addrs[func_name],
                .shared_mem=function_shared_mem[func_name],
                .num_registers=function_regs[func_name],
                .local_mem_req=function_local_mem_reqs[func_name],
                .function_size=function_sizes[func_name],
                .constants=constants[func_name],
                .param_info=param_infos[func_name]
        });
    }
    *pModule = new LibreCUmodule_{
            .functions=functions,
            .module_va_addr=module_gpu_va
    };

    // apply relocations
    for (auto &reloc: relocs) {
        LIBRECUDA_VALIDATE(reloc.apply_image_offset < imageSize, LIBRECUDA_ERROR_INVALID_IMAGE);
        switch (reloc.typ) {
            case 0x2: {
                auto *loc_ptr = reinterpret_cast<NvU64 *>((NvU8 *) image + reloc.apply_image_offset);
                *loc_ptr = module_gpu_va + reloc.rel_sym_offset;
                break;
            }
            case 0x38: {
                auto *loc_ptr = reinterpret_cast<NvU32 *>((NvU8 *) image + reloc.apply_image_offset + 4);
                *loc_ptr = (module_gpu_va + reloc.rel_sym_offset) & 0xffffffff;
                break;
            }
            case 0x39: {
                auto *loc_ptr = reinterpret_cast<NvU32 *>((NvU8 *) image + reloc.apply_image_offset + 4);
                *loc_ptr = ((module_gpu_va + reloc.rel_sym_offset) >> 32);
                break;
            }
        }
    }

    // copy to gpu
    memcpy(reinterpret_cast<void *>(module_gpu_va), image, imageSize);
    LIBRECUDA_SUCCEED();
}


libreCudaStatus_t libreCuModuleGetFunctionCount(uint32_t *count, LibreCUmodule mod) {
    LIBRECUDA_VALIDATE(count != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    *count = mod->functions.size();
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t
libreCuModuleEnumerateFunctions(LibreCUFunction *functions, uint32_t numFunctions, LibreCUmodule mod) {
    LIBRECUDA_VALIDATE(functions != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(mod != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    for (uint32_t i = 0; i < numFunctions; i++) {
        functions[i] = &mod->functions[i];
    }
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t libreCuModuleGetFunction(LibreCUFunction *pFunc, LibreCUmodule module, const char *name) {
    LIBRECUDA_VALIDATE(pFunc != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(module != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);

    bool found = false;

    for (auto &function: module->functions) {
        if (function.name == name) {
            found = true;
            *pFunc = &function;
            break;
        }
    }
    if (!found) {
        LIBRECUDA_FAIL(LIBRECUDA_ERROR_NOT_FOUND);
    }

    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t libreCuLaunchKernel(LibreCUFunction function,
                                      uint32_t gridDimX, uint32_t gridDimY, uint32_t gridDimZ,
                                      uint32_t blockDimX, uint32_t blockDimY, uint32_t blockDimZ,
                                      uint32_t sharedMemBytes,
                                      LibreCUstream stream,
                                      void **kernelParams, size_t numParams,
                                      void **extra) {
    LIBRECUDA_VALIDATE(function != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(stream != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    stream->command_queue->launchFunction(function,
                                          gridDimX, gridDimY, gridDimZ,
                                          blockDimX, blockDimY, blockDimZ,
                                          kernelParams,
                                          numParams
    );
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t libreCuModuleUnload(LibreCUmodule function) {
    LIBRECUDA_ENSURE_CTX_VALID();
    LIBRECUDA_VALIDATE(function != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_ERR_PROPAGATE(gpuFree(current_ctx, function->module_va_addr));
    delete function;
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t libreCuFuncGetName(const char **pNameOut, LibreCUFunction func) {
    LIBRECUDA_VALIDATE(pNameOut != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    *pNameOut = func->name.c_str();
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t libreCuStreamCreate(LibreCUstream *pStreamOut, uint32_t flags) {
    LIBRECUDA_VALIDATE(pStreamOut != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);

    // create command queue
    auto command_queue = new NvCommandQueue(current_ctx);
    LIBRECUDA_ERR_PROPAGATE(command_queue->initializeQueue());

    *pStreamOut = new LibreCUstream_{
            .command_queue=command_queue
    };
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t libreCuStreamCommence(LibreCUstream stream) {
    LIBRECUDA_VALIDATE(stream != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    stream->command_queue->startExecution(COMPUTE);
    LIBRECUDA_SUCCEED();
}


libreCudaStatus_t libreCuStreamAwait(LibreCUstream stream) {
    LIBRECUDA_VALIDATE(stream != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    stream->command_queue->awaitExecution();
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t libreCuStreamDestroy(LibreCUstream stream) {
    LIBRECUDA_VALIDATE(stream != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);

    // delete command queue
    delete stream->command_queue;

    delete stream;
    LIBRECUDA_SUCCEED();
}
