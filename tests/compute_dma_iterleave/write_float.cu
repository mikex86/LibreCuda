extern "C" __global__ void write_float(float *dst, float *input) {
    double x = 0;
    int n = 100000000;
    for (int i = 0; i < n; i++) {
        x += 1.0;
    }
    x /= n;
    *dst = (float) x + (*input);
}