struct struct_t {
    int x, y, z;
    int w, h, d;
    char str[33];
    char me_ugly;
};

extern "C" __global__ void read_from_struct(struct_t s, int *pWout) {
    *pWout = s.w;
}