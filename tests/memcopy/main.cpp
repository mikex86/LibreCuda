#include <librecuda.h>

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
    CUDA_CHECK(libreCuCtxCreate_v2(&ctx, CU_CTX_SCHED_YIELD, device));

    // create stream
    LibreCUstream stream{};
    CUDA_CHECK(libreCuStreamCreate(&stream, 0));

    // declare host array
    float host_array[] = {
            1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f
    };

    // declare host array
    float dst_host_array[10] = {};

    // allocate memory
    float *device_array_1{};
    float *device_array_2{};
    CUDA_CHECK(libreCuMemAlloc(reinterpret_cast<void **>(&device_array_1), sizeof(host_array)));
    CUDA_CHECK(libreCuMemAlloc(reinterpret_cast<void **>(&device_array_2), sizeof(host_array)));

    // copy to gpu
    CUDA_CHECK(libreCuMemCpy(device_array_1, host_array, sizeof(host_array), stream));

    // copy d2d
    CUDA_CHECK(libreCuMemCpy(device_array_2, device_array_1, sizeof(host_array), stream));

    // copy back to host
    CUDA_CHECK(libreCuMemCpy(dst_host_array, device_array_1, sizeof(host_array), stream));

    // commence stream
    CUDA_CHECK(libreCuStreamCommence(stream));
    CUDA_CHECK(libreCuStreamAwait(stream));

    // print device array
    for (int i = 0; i < 10; i++) {
        std::cout << dst_host_array[i] << ", ";
    }

    // destroy stream
    CUDA_CHECK(libreCuStreamDestroy(stream));

    // free memory
    CUDA_CHECK(libreCuMemFree(device_array_1));
    CUDA_CHECK(libreCuMemFree(device_array_2));

    // destroy ctx
    CUDA_CHECK(libreCuCtxDestroy(ctx));
    return 0;
}