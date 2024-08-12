extern "C" __global__ void write_float_ptr(float *dst, float *src) {
    *dst = *src;
}

extern "C" __global__ void write_float_value(float *dst, float value) {
    *dst = value;
}


extern "C" __global__ void write_float_sum(float *dst, short a, float b) {
    *dst = (a + b);
}