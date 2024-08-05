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
    } else {
        std::string device_file = getDeviceFile(ctx->device);

        device_fd = open(device_file.c_str(), O_RDWR | O_CLOEXEC);
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

    // create ctx share
    NvHandle ctx_share{};
    {
        NV_CTXSHARE_ALLOCATION_PARAMETERS params{
                .hVASpace = ctx->va_space_handle,
                .flags = NV_CTXSHARE_ALLOCATION_FLAGS_SUBCONTEXT_ASYNC
        };
        RM_ALLOC(fd_ctl, FERMI_CONTEXT_SHARE_A, root, channel_group, 0, &params, sizeof(params), &ctx_share);
    }

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

    // create command queue
    ctx->command_queue = new NvCommandQueue(ctx);
    LIBRECUDA_ERR_PROPAGATE(ctx->command_queue->initializeQueue());

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

    // delete command queue
    delete ctx->command_queue;
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

libreCudaStatus_t libreCuMemAlloc(void **pDevicePointer, size_t bytesize) {
    LIBRECUDA_VALIDATE(pDevicePointer != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(bytesize > 0, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_ENSURE_CTX_VALID();
    NvU64 va_address{};
    {
        bool hugePages = bytesize > (16 << 20);
        gpuAlloc(current_ctx, bytesize, false, hugePages, false, 0, &va_address);
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
    NvU64 program_gpu_va{};
    size_t allocated_size = ceilDiv(imageSize, 0x1000) * 0x1000 + 0x1000;
    LIBRECUDA_ERR_PROPAGATE(
            gpuAlloc(current_ctx, allocated_size, false, false, true, 0, &program_gpu_va)
    );

    // copy to gpu
    memcpy(reinterpret_cast<void *>(program_gpu_va), current_ctx, imageSize);

    // iterate over sections
    std::unordered_map<std::string, NvU32> shared_mem{};
    std::unordered_set<std::string> function_names{};
    for (const auto &section: elf_reader.sections) {
        const std::string &section_name = section->get_name();
        if (section_name.size() > 11 && section_name.substr(0, 11) == ".nv.shared.") {
            std::string function_name = section_name.substr(11);
            shared_mem[function_name] = elf_reader.get_section_entry_size();
        }
        if (section_name.size() > 6 && section_name.substr(0, 6) == ".text.") {
            std::string function_name = section_name.substr(6);
            function_names.insert(function_name);
        }
    }

    LIBRECUDA_SUCCEED();
}