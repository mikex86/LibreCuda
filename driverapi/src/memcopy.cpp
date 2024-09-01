#include "memcopy.h"

#include "memcopy_kernel.h"

#define CEIL_DIV(x, y) (((x) + (y) - 1) / (y))

#define MEMCPY_BLOCK_SIZE 256UL
#define MEMCPY_THREAD_COPYSIZE 16UL
#define MAX_GRID_SIZE_X 65535UL
#define MEMCPY_HIGHBW_BYTE_GRANULARITY 4096
#define MEMCPY_FINISH_BYTE_GRANULARITY 4

static bool initialized = false;
static LibreCUmodule module;
static LibreCUFunction kernelHighBW;
static LibreCUFunction kernelLowLatency;
static LibreCUFunction kernelTrailingCpy;


libreCudaStatus_t loadMemcpyKernelsIfNeeded() {
    if (initialized) {
        return LIBRECUDA_SUCCESS;
    }
    // TODO: sm_75 seems to still be binary compatible for sm_89, so here it goes.
    //  add more variants as needed
    LIBRECUDA_ERR_PROPAGATE(libreCuModuleLoadData(&module, memcy_cubin_data_sm75, sizeof(memcy_cubin_data_sm75)));
    LIBRECUDA_ERR_PROPAGATE(libreCuModuleGetFunction(&kernelHighBW, module, "memcpyKernelHighBW"));
    LIBRECUDA_ERR_PROPAGATE(libreCuModuleGetFunction(&kernelLowLatency, module, "memcpyKernelLowLatency"));
    LIBRECUDA_ERR_PROPAGATE(libreCuModuleGetFunction(&kernelTrailingCpy, module, "memcpyKernelTrailing"));

    initialized = true;
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t memcpyD2D(void *dst, void *src, size_t size, LibreCUstream stream, bool async) {
    uint32_t blockSizeX = MEMCPY_BLOCK_SIZE;

    bool use_high_bw = size > (1024 * 1024); // 1 MiB


    // technically the three kernels can run concurrently, so that's what we want to do.
    // if the entire memcpyD2D command is logically !asymc, we only want to sync at the last kernel of the three
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
                libreCuLaunchKernel(kernelHighBW,
                                    gridSizeX, gridSizeY, 1,
                                    blockSizeX, 1, 1,
                                    0, stream, params,
                                    sizeof(params) / sizeof(void *), nullptr, async ? true : has_synced);
        );
        if (!async) {
            has_synced = true;
        }
    }

    if (launch_kernel_2) {
        size_t bytes_copied = use_high_bw ? ((size / MEMCPY_HIGHBW_BYTE_GRANULARITY) * MEMCPY_HIGHBW_BYTE_GRANULARITY)
                                          : 0;
        size_t bytes_remaining = size - bytes_copied;
        size_t words_remaining = bytes_remaining / sizeof(uint32_t);
        auto gridSizeX = CEIL_DIV(words_remaining, MEMCPY_BLOCK_SIZE);

        void *dst_remain = reinterpret_cast<uint8_t *>(dst) + bytes_copied;
        void *src_remain = reinterpret_cast<uint8_t *>(src) + bytes_copied;
        void *params[] = {
                &dst_remain, &src_remain, &words_remaining
        };
        LIBRECUDA_ERR_PROPAGATE(
                libreCuLaunchKernel(kernelLowLatency,
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
                libreCuLaunchKernel(kernelTrailingCpy,
                                    gridSizeX, 1, 1,
                                    blockSizeX, 1, 1,
                                    0, stream, params,
                                    sizeof(params) / sizeof(void *), nullptr, async ? true : has_synced)
        );
    }

    LIBRECUDA_SUCCEED();
}
