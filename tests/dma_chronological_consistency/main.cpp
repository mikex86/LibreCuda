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
    uint8_t host_array[1024 * 1024 + 128 + 3]{}; // size chosen to require all 3 memcpy hierarchy kernels to be launched
    for (size_t i = 0; i < sizeof(host_array); i++) {
        host_array[i] = i % 256;
    }

    // declare host array
    uint8_t dst_host_array[sizeof(host_array)] = {};

    // allocate memory
    uint8_t *device_array_1{};
    uint8_t *device_array_2{};
    uint8_t *device_array_3{};
    uint8_t *device_array_4{};
    uint8_t *device_array_5{};
    uint8_t *device_array_6{};
    CUDA_CHECK(libreCuMemAlloc(reinterpret_cast<void **>(&device_array_1), sizeof(host_array)));
    CUDA_CHECK(libreCuMemAlloc(reinterpret_cast<void **>(&device_array_2), sizeof(host_array)));
    CUDA_CHECK(libreCuMemAlloc(reinterpret_cast<void **>(&device_array_3), sizeof(host_array)));
    CUDA_CHECK(libreCuMemAlloc(reinterpret_cast<void **>(&device_array_4), sizeof(host_array)));
    CUDA_CHECK(libreCuMemAlloc(reinterpret_cast<void **>(&device_array_5), sizeof(host_array)));
    CUDA_CHECK(libreCuMemAlloc(reinterpret_cast<void **>(&device_array_6), sizeof(host_array)));

    // copy to gpu
    CUDA_CHECK(libreCuMemCpy(device_array_1, host_array, sizeof(host_array), stream));

    // copy d2d
    CUDA_CHECK(libreCuMemCpy(device_array_2, device_array_1, sizeof(host_array), stream));
    CUDA_CHECK(libreCuMemCpy(device_array_3, device_array_2, sizeof(host_array), stream));
    CUDA_CHECK(libreCuMemCpy(device_array_4, device_array_3, sizeof(host_array), stream));
    CUDA_CHECK(libreCuMemCpy(device_array_5, device_array_4, sizeof(host_array), stream));
    CUDA_CHECK(libreCuMemCpy(device_array_6, device_array_5, sizeof(host_array), stream));

    // copy back to host
    CUDA_CHECK(libreCuMemCpy(dst_host_array, device_array_6, sizeof(host_array), stream));

    // commence stream
    CUDA_CHECK(libreCuStreamCommence(stream));
    CUDA_CHECK(libreCuStreamAwait(stream));

    // print device array
    bool is_equal = true;
    size_t i;
    for (i = 0; i < sizeof(host_array); i++) {
        if (host_array[i] != dst_host_array[i]) {
            is_equal = false;
            break;
        }
    }
    if (!is_equal) {
        std::cerr << "Mismatch at " + std::to_string(i) + ": memcpy screwed something up!" << std::endl;
    } else {
        std::cout << "Memory is equal!" << std::endl;
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