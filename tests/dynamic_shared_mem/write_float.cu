extern "C" __global__ void write_float_sum(float *dst, short a, float b) {
    extern __shared__ float sharedData[];

    sharedData[1024] = (float) a;
    sharedData[1025] = (float) b;

    *dst = (sharedData[1024] + sharedData[1025]);
}