extern "C" __global__ void write_float_ptr(float *dst, float *src) {
    *dst = *src;
}

extern "C" __global__ void write_float_value(float *dst, float value) {
    *dst = value;
}

__device__ int global_int = 0x69;

extern "C" __global__ void write_float_sum(float *dst, short a, float b) {
    *dst = (a + b + global_int);
}