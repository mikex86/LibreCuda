#ifndef LIBRECUDA_LIBRECUDA_INTERNAL_H
#define LIBRECUDA_LIBRECUDA_INTERNAL_H

#include <sys/ioctl.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <nvidia/ctrl2080gr.h>

#include "nvidia/nvtypes.h"
#include "nvidia/nvCpuUuid.h"
#include "nvidia/nvos.h"
#include "nvidia/nv_escape.h"
#include "nvidia/clc56f.h"

struct NvCommandQueue;

enum DriverType {
    OPEN_KERNEL_MODULES, NVIDIA_PROPRIETARY
};

extern DriverType driver_type;

struct GPFifo {
    NvU64 *ring;
    NvU32 entries_count;
    NvU32 token;
    AmpereAControlGPFifo *controls;
    NvU32 put_value = 0;
};

struct LibreCUdevice_ {
    int ordinal;
    NvU32 instance;
    NvProcessorUuid uuid;
    NvU32 *gpu_mmio;
    NvU32 compute_class;
    NV2080_CTRL_GR_INFO device_info[NV2080_CTRL_GR_INFO_INDEX_MAX + 1];
};

#define UVM_HEAP_START 0x1000000000

struct LibreCUcontext_ {
    int device_fd;
    int flags;
    LibreCUdevice device;
    NvHandle device_handle;
    NvU64 uvm_vaddr = UVM_HEAP_START;
    NvHandle va_space_handle{};

    GPFifo compute_gpfifo;
    GPFifo dma_gpfifo;

    NvHandle ctx_share;
    NvHandle channel_group;
};

extern LibreCUcontext current_ctx;

struct KernelConstantInfo {
    NvU32 const_nr;
    NvU64 address;
    NvU64 size;
};

struct KernelParamInfo {
    NvU16 param_index;
    NvU16 param_offset;
    NvU16 param_size;
};

struct LibreCUmodule_ {
    /**
     * List of all functions of the module
     */
    std::vector<LibreCUFunction_> functions{};

    /**
     * GPU virtual address where the elf binary was copied.
     * This address is freed on unload.
     */
    NvU64 module_va_addr{};
};


struct LibreCUFunction_ {
    std::string name;
    NvU64 func_va_addr;
    NvU32 shared_mem;
    NvU32 num_registers;
    NvU32 local_mem_req;
    NvU64 function_size;
    std::vector<KernelConstantInfo> constants;
    std::vector<KernelParamInfo> param_info;

    /**
     * Virtual address of shader local memory used for shaders/kernels.
     */
    NvU64 shader_local_memory_va{};
};

struct LibreCUstream_ {
    NvCommandQueue *command_queue;
};

struct LibreCuRoute {
    NvBool valid;
    NvU32 swizId;
    NvU32 engineId;
};


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
    NVOS21_PARAMETERS parameters{
        .hRoot = client,
        .hObjectParent = parent,
        .hObjectNew = object,
        .hClass = clss,
        .pAllocParms = params,
        .paramsSize = paramSize
    };
    NV_IOWR(fd, NV_ESC_RM_ALLOC, &parameters, sizeof(parameters));
    if (pObjectNew != nullptr) {
        *pObjectNew = parameters.hObjectNew;
    }
    if (parameters.status != 0) {
        LIBRECUDA_FAIL(LIBRECUDA_ERROR_UNKNOWN);
    }
    LIBRECUDA_SUCCEED();
}

#define LIBRECUDA_VALIDATE_RM_ALLOC(status) { libreCudaStatus_t status_val = status; if (status_val != LIBRECUDA_SUCCESS) { LIBRECUDA_DEBUG(__FILE__ ":" __LIBRECUDA_TOSTRING(__LINE__) ": rm_alloc failed with status " + std::to_string(status_val)); LIBRECUDA_FAIL(LIBRECUDA_ERROR_UNKNOWN); } }

#define RM_ALLOC(fd, clss, client, parent, object, params, param_size, pObjectNew) LIBRECUDA_VALIDATE_RM_ALLOC(rm_alloc(fd, clss, client, parent, object, params, param_size, pObjectNew));

static inline libreCudaStatus_t rm_ctrl(int fd,
                                        NvV32 cmd,
                                        NvHandle client, NvHandle object,
                                        void *params, NvU32 paramSize);

#define LIBRECUDA_VALIDATE_RM_CTRL(status) { libreCudaStatus_t status_val = status; if (status_val != LIBRECUDA_SUCCESS) { LIBRECUDA_DEBUG(__FILE__ ":" __LIBRECUDA_TOSTRING(__LINE__) ": rm_ctrl failed with status " + std::to_string(status_val)); LIBRECUDA_FAIL(LIBRECUDA_ERROR_UNKNOWN); } }

#define RM_CTRL(fd, cmd, client, object, params, paramSize) LIBRECUDA_VALIDATE_RM_CTRL(rm_ctrl(fd, cmd, client, object, params, paramSize))

#define LIBRECUDA_ENSURE_CTX_VALID() LIBRECUDA_VALIDATE(current_ctx != nullptr, LIBRECUDA_ERROR_INVALID_CONTEXT);

libreCudaStatus_t
gpuAlloc(LibreCUcontext ctx, size_t size, bool physicalContiguity, bool hugePages, bool mapToCpu, NvU32 mapFlags,
         NvU64 *pVaOut, NvHandle *pMemoryHandleOut = nullptr);

libreCudaStatus_t
gpuSystemAlloc(LibreCUcontext ctx, size_t size, bool mapToCpu, NvU32 mapFlags,
               NvU64 *pVaOut, NvHandle *pMemoryHandleOut = nullptr);

libreCudaStatus_t gpuFree(LibreCUcontext ctx, NvU64 virtualAddress);

/**
 * @param ptr the pointer to check for being a device pointer
 * @return true if the pointer is a device pointer.
 * This does not mean the ptr is still allocated. It just means it is or was a device pointer.
 */
bool isDevicePtr(void *ptr);

/**
 * @param ptr the pointer to check for being mapped to host.
 * @return true if the pointer is a device pointer and mapped to host. will return false for host pointers.
 */
bool isHostMappedPtr(void *ptr);

#endif //LIBRECUDA_LIBRECUDA_INTERNAL_H
