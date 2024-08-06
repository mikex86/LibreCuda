#include "librecuda.h"

#include <iostream>
#include <cstdint>
#include <vector>
#include <fstream>
#include <cstring>

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

    LibreCUmodule module{};

    // read cubin file
    uint8_t *image;
    size_t n_bytes;
    {
        std::ifstream input("complex.cubin", std::ios::binary);
        std::vector<uint8_t> bytes(
                (std::istreambuf_iterator<char>(input)),
                (std::istreambuf_iterator<char>()));
        input.close();
        image = new uint8_t[bytes.size()];
        memcpy(image, bytes.data(), bytes.size());
        n_bytes = bytes.size();
    }
    CUDA_CHECK(libreCuModuleLoadData(&module, image, n_bytes));

    uint32_t num_funcs{};
    CUDA_CHECK(libreCuModuleGetFunctionCount(&num_funcs, module));
    std::cout << "Num functions: " << num_funcs << std::endl;

    auto *functions = new LibreCUFunction[num_funcs];
    CUDA_CHECK(libreCuModuleEnumerateFunctions(functions, num_funcs, module));

    for (size_t i = 0; i < num_funcs; i++) {
        LibreCUFunction func = functions[i];
        const char *func_name{};
        CUDA_CHECK(libreCuFuncGetName(&func_name, func));
        std::cout << "  function \"" << func_name << "\"" << std::endl;
    }

    delete[] functions;

    LibreCUFunction func{};
    CUDA_CHECK(libreCuModuleGetFunction(&func, module, "matmul_kernel"));

    CUDA_CHECK(libreCuModuleUnload(module));

    void *device_ptr{};
    CUDA_CHECK(libreCuMemAlloc(&device_ptr, 1024 * sizeof(float)));

    float data[] = {
            1.0f,
            2.0f,
            3.0f,
            4.0f,
            5.0f
    };

    CUDA_CHECK(libreCuMemCpy(device_ptr, device_ptr, sizeof(data)));

    CUDA_CHECK(libreCuMemFree(device_ptr));
    CUDA_CHECK(libreCuCtxDestroy(ctx));
    return 0;
}