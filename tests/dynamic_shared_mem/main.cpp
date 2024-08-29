#include <librecuda.h>

#include <iostream>
#include <iomanip>
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

    char name_buffer[256] = {};
    libreCuDeviceGetName(name_buffer, 256, device);
    std::cout << "Device Name: " + std::string(name_buffer) << std::endl;

    int maxSharedMemoryPerBlock{};
    CUDA_CHECK(libreCuDeviceGetAttribute(&maxSharedMemoryPerBlock, CU_DEVICE_ATTRIBUTE_MAX_SHARED_MEMORY_PER_BLOCK, device));

    int maxSharedMemoryPerBlockOptIn{};
    CUDA_CHECK(libreCuDeviceGetAttribute(&maxSharedMemoryPerBlockOptIn, CU_DEVICE_ATTRIBUTE_MAX_SHARED_MEMORY_PER_BLOCK_OPTIN, device));

    std::cout << "Maximum shared memory per block: " << maxSharedMemoryPerBlock << " bytes" << std::endl;
    std::cout << "Maximum shared memory per block: " << maxSharedMemoryPerBlockOptIn << " bytes" << std::endl;

    LibreCUmodule module{};

    // read cubin file
    uint8_t *image;
    size_t n_bytes;
    {
        std::ifstream input("write_float.cubin", std::ios::binary);
        std::vector<uint8_t> bytes(
                (std::istreambuf_iterator<char>(input)),
                (std::istreambuf_iterator<char>()));
        input.close();
        image = new uint8_t[bytes.size()];
        memcpy(image, bytes.data(), bytes.size());
        n_bytes = bytes.size();
    }
    CUDA_CHECK(libreCuModuleLoadData(&module, image, n_bytes));

    // read functions
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

    // find function
    LibreCUFunction func{};
    CUDA_CHECK(libreCuModuleGetFunction(&func, module, "write_float_sum"));

    // set dynamic shared memory
    CUDA_CHECK(libreCuFuncSetAttribute(func, CU_FUNC_ATTRIBUTE_MAX_DYNAMIC_SHARED_SIZE_BYTES, 8192));

    // create stream
    LibreCUstream stream{};
    CUDA_CHECK(libreCuStreamCreate(&stream, 0));

    void *float_dst_va{};
    CUDA_CHECK(libreCuMemAlloc(&float_dst_va, sizeof(float), true));

    float float_value = 0.31415f;
    short short_value = 314;

    std::cout << std::fixed;
    std::cout << std::setprecision(5);

    std::cout << "A value: " << short_value << std::endl;
    std::cout << "B value: " << float_value << std::endl;
    std::cout << "Dst value (pre exec): " << *(float *) (float_dst_va) << std::endl;

    void *params[] = {
            &float_dst_va, // dst
            &short_value, // a
            &float_value // b
    };
    CUDA_CHECK(
            libreCuLaunchKernel(func,
                                1, 1, 1,
                                1, 1, 1,
                                8192,
                                stream,
                                params, sizeof(params) / sizeof(void *),
                                nullptr
            )
    );

    // dispatch built up command buffer to GPU
    CUDA_CHECK(libreCuStreamCommence(stream));

    // wait for work to complete
    CUDA_CHECK(libreCuStreamAwait(stream));
    std::cout << "Dst value (post exec): " << *(float *) (float_dst_va) << std::endl;

    // free memory
    CUDA_CHECK(libreCuMemFree(float_dst_va));

    // destroy stream
    CUDA_CHECK(libreCuStreamDestroy(stream));

    // unload module
    CUDA_CHECK(libreCuModuleUnload(module));

    // destroy ctx
    CUDA_CHECK(libreCuCtxDestroy(ctx));
    return 0;
}