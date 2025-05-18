#include "memcopy.h"

#include <cmdqueue.h>
#include <librecuda_internal.h>
#include <unordered_map>
#include "memcopy_kernels.h"

#define CEIL_DIV(x, y) (((x) + (y) - 1) / (y))

#define MEMCPY_BLOCK_SIZE 256UL
#define MEMCPY_THREAD_COPYSIZE 16UL
#define MAX_GRID_SIZE_X 65535UL
#define MEMCPY_HIGHBW_BYTE_GRANULARITY 4096
#define MEMCPY_FINISH_BYTE_GRANULARITY 4

static std::unordered_map<int, bool> initialized;
static std::unordered_map<int, LibreCUmodule> module;
static std::unordered_map<int, LibreCUFunction> kernelHighBW;
static std::unordered_map<int, LibreCUFunction> kernelLowLatency;
static std::unordered_map<int, LibreCUFunction> kernelTrailingCpy;


libreCudaStatus_t loadMemcpyKernelsIfNeeded(const LibreCUdevice device) {
    if (initialized[device->ordinal]) {
        return LIBRECUDA_SUCCESS;
    }

    int compute_capability_major{}, compute_capability_minor{};
    LIBRECUDA_ERR_PROPAGATE(
        libreCuDeviceGetAttribute(&compute_capability_major, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, device));
    LIBRECUDA_ERR_PROPAGATE(
        libreCuDeviceGetAttribute(&compute_capability_minor, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, device));

    int compute_capability = compute_capability_major * 10 + compute_capability_minor;

    const uint8_t *cubin_data;
    size_t cubin_size{};

#define ARCH_KERNEL(capability) \
            case capability: \
            cubin_data = MEMCPY_SM_##capability; \
            cubin_size = sizeof(MEMCPY_SM_##capability); \
            break;

    switch (compute_capability) {
        ARCH_KERNEL(50)
        ARCH_KERNEL(53)
        ARCH_KERNEL(60)
        ARCH_KERNEL(61)
        ARCH_KERNEL(62)
        ARCH_KERNEL(70)
        ARCH_KERNEL(72)
        ARCH_KERNEL(75)
        ARCH_KERNEL(80)
        ARCH_KERNEL(86)
        ARCH_KERNEL(87)
        ARCH_KERNEL(89)
        ARCH_KERNEL(90)
        default:
            LIBRECUDA_FAIL(LIBRECUDA_ERROR_COMPAT_NOT_SUPPORTED_ON_DEVICE);
    }
    LIBRECUDA_ERR_PROPAGATE(libreCuModuleLoadData(&module[device->ordinal], cubin_data, cubin_size));
    LIBRECUDA_ERR_PROPAGATE(
        libreCuModuleGetFunction(&kernelHighBW[device->ordinal], module[device->ordinal], "memcpyKernelHighBW")
    );
    LIBRECUDA_ERR_PROPAGATE(
        libreCuModuleGetFunction(&kernelLowLatency[device->ordinal], module[device->ordinal], "memcpyKernelLowLatency")
    );
    LIBRECUDA_ERR_PROPAGATE(
        libreCuModuleGetFunction(&kernelTrailingCpy[device->ordinal], module[device->ordinal], "memcpyKernelTrailing")
    );

    initialized[device->ordinal] = true;
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t memcpyD2D(void *dst, void *src, size_t size, LibreCUstream stream, bool async) {
    LIBRECUDA_ENSURE_CTX_VALID();

    uint32_t blockSizeX = MEMCPY_BLOCK_SIZE;

    bool use_high_bw = size > (1024 * 1024); // 1 MiB

    // technically the three kernels can run concurrently, so that's what we want to do.
    // if the entire memcpyD2D command is logically !async, we only want to sync at the last kernel of the three
    // that runs
    bool launch_kernel_1 = use_high_bw;
    bool launch_kernel_2 = !use_high_bw || size % MEMCPY_HIGHBW_BYTE_GRANULARITY != 0;
    bool launch_kernel_3 = size % MEMCPY_FINISH_BYTE_GRANULARITY != 0;

    bool has_synced = false;

    if (launch_kernel_1) {
        auto gridSizeX = (size /
                          (MEMCPY_THREAD_COPYSIZE * MEMCPY_BLOCK_SIZE));
        auto gridSizeY = CEIL_DIV(gridSizeX, MAX_GRID_SIZE_X);

        void *params[] = {
            &dst, &src
        };
        LIBRECUDA_ERR_PROPAGATE(
            libreCuLaunchKernel(kernelHighBW[current_ctx->device->ordinal],
                gridSizeX, gridSizeY, 1,
                blockSizeX, 1, 1,
                0, stream, params,
                sizeof(params) / sizeof(void *), nullptr, async ? true : has_synced)
        );
        if (!async) {
            has_synced = true;
        }
    }

    if (launch_kernel_2) {
        size_t bytes_copied = use_high_bw
                                  ? ((size / MEMCPY_HIGHBW_BYTE_GRANULARITY) * MEMCPY_HIGHBW_BYTE_GRANULARITY)
                                  : 0;
        size_t bytes_remaining = size - bytes_copied;
        size_t words_remaining = bytes_remaining / sizeof(uint32_t);
        auto gridSizeX = CEIL_DIV(words_remaining, MEMCPY_BLOCK_SIZE);

        void *dst_remain = static_cast<uint8_t *>(dst) + bytes_copied;
        void *src_remain = static_cast<uint8_t *>(src) + bytes_copied;
        void *params[] = {
            &dst_remain, &src_remain, &words_remaining
        };
        LIBRECUDA_ERR_PROPAGATE(
            libreCuLaunchKernel(kernelLowLatency[current_ctx->device->ordinal],
                gridSizeX, 1, 1,
                blockSizeX, 1, 1,
                0, stream, params,
                sizeof(params) / sizeof(void *), nullptr, async ? true : has_synced)
        );
        if (!async) {
            has_synced = true;
        }
    }

    if (launch_kernel_3) {
        size_t bytes_copied = ((size / MEMCPY_HIGHBW_BYTE_GRANULARITY) * MEMCPY_HIGHBW_BYTE_GRANULARITY);
        size_t bytes_remaining = size - bytes_copied;
        auto gridSizeX = CEIL_DIV(bytes_remaining, MEMCPY_BLOCK_SIZE);

        void *dst_remain = reinterpret_cast<uint8_t *>(dst) + bytes_copied;
        void *src_remain = reinterpret_cast<uint8_t *>(src) + bytes_copied;
        void *params[] = {
            &dst_remain, &src_remain, &bytes_remaining
        };
        LIBRECUDA_ERR_PROPAGATE(
            libreCuLaunchKernel(kernelTrailingCpy[current_ctx->device->ordinal],
                gridSizeX, 1, 1,
                blockSizeX, 1, 1,
                0, stream, params,
                sizeof(params) / sizeof(void *), nullptr, async ? true : has_synced)
        );
    }

    LIBRECUDA_SUCCEED();
}
