#include <cstdint>

#define MEMCPY_BLOCK_SIZE 256UL

extern "C" __global__ void memcpyKernelHighBW(uint32_t *dst, const uint32_t *src) {
    size_t idx = ((MEMCPY_BLOCK_SIZE * (blockIdx.y * gridDim.x + blockIdx.x)) << 2) + threadIdx.x;

    #pragma unroll
    for (int i = 0; i < 4; i++) {
        dst[idx] = src[idx];
        idx += MEMCPY_BLOCK_SIZE;
    }
}

extern "C" __global__ void memcpyKernelLowLatency(uint32_t *dst, const uint32_t *src, size_t n) {
    size_t tid = MEMCPY_BLOCK_SIZE * blockIdx.x + threadIdx.x;
    if (tid < n) {
        dst[tid] = src[tid];
    }
}

extern "C" __global__ void memcpyKernelTrailing(uint8_t *dst, const uint8_t *src, size_t n) {
    size_t tid = MEMCPY_BLOCK_SIZE * blockIdx.x + threadIdx.x;
    if (tid < n) {
        dst[tid] = src[tid];
    }
}