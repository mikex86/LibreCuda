extern "C" __global__ void write_float(float *dst, size_t n) {
    size_t tid = blockDim.x * blockIdx.x + threadIdx.x;
    dst[tid] = 1.0f;
}