#include "librecuda"

#include <iostream>

inline void cudaCheck(libreCudaStatus_t error, const char *file, int line) {
    if (error != LIBRECUDA_SUCCESS) {
        const char *error_string;
        libreCuGetErrorString(error, &error_string);
        printf("[CUDA ERROR] at file %s:%d: %s\n", file, line, error_string);
        exit(EXIT_FAILURE);
    }
};
#define CUDA_CHECK(err) (cudaCheck(err, __FILE__, __LINE__))

int main() {
    CUDA_CHECK(libreCuInit(0));

    int device_count{};
    CUDA_CHECK(libreCuDeviceGetCount(&device_count));
    std::cout << "Device count: " + std::to_string(device_count) << std::endl;

    LibreCUdevice device{};
    CUDA_CHECK(libreCuDeviceGet(&device, 0));

    LibreCUcontext ctx{};
    CUDA_CHECK(libreCuCtxCreate_v2(&ctx, 0, device));

    void *device_ptr{};
    CUDA_CHECK(libreCuMemAlloc(&device_ptr, 1024 * sizeof(float)));

    std::cout << "Virtual address ptr: " << device_ptr << std::endl;

    CUDA_CHECK(libreCuMemFree(device_ptr));

    CUDA_CHECK(libreCuCtxDestroy(ctx));
    return 0;
}