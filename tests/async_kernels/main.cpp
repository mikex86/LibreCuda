#include <librecuda.h>

#include <iostream>
#include <vector>
#include <fstream>
#include <cstring>
#include <chrono>

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
    CUDA_CHECK(libreCuModuleGetFunction(&func, module, "write_float"));

    // create stream
    LibreCUstream stream{};
    CUDA_CHECK(libreCuStreamCreate(&stream, 0));

    void *float_dst_compute_va{};
    void *float_dst_dma_va{};
    CUDA_CHECK(libreCuMemAlloc(&float_dst_compute_va, sizeof(float), true));
    CUDA_CHECK(libreCuMemAlloc(&float_dst_dma_va, sizeof(float), true));
    *(float *) float_dst_compute_va = 0.0f;
    *(float *) float_dst_dma_va = 0.0f;

    // first time execution of the kernel
    auto start = std::chrono::high_resolution_clock::now();
    {
        void *params[] = {
                &float_dst_compute_va, &float_dst_dma_va
        };
        CUDA_CHECK(
                libreCuLaunchKernel(func,
                                    1, 1, 1,
                                    1, 1, 1,
                                    0,
                                    stream,
                                    params, sizeof(params) / sizeof(void *),
                                    nullptr,
                                    false
                )
        );
    }
    CUDA_CHECK(libreCuStreamCommence(stream));
    CUDA_CHECK(libreCuStreamAwait(stream));
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Single kernel took: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << "ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    {
        void *params[] = {
                &float_dst_compute_va, &float_dst_dma_va
        };
        for (int i = 0; i < 5; i++) {
            CUDA_CHECK(
                    libreCuLaunchKernel(func,
                                        1, 1, 1,
                                        1, 1, 1,
                                        0,
                                        stream,
                                        params, sizeof(params) / sizeof(void *),
                                        nullptr,
                                        true
                    )
            );
        }
    }
    CUDA_CHECK(libreCuStreamCommence(stream));
    CUDA_CHECK(libreCuStreamAwait(stream));
    end = std::chrono::high_resolution_clock::now();
    std::cout << "5xAsync kernel took: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << "ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    {
        void *params[] = {
                &float_dst_compute_va, &float_dst_dma_va
        };
        for (int i = 0; i < 5; i++) {
            CUDA_CHECK(
                    libreCuLaunchKernel(func,
                                        1, 1, 1,
                                        1, 1, 1,
                                        0,
                                        stream,
                                        params, sizeof(params) / sizeof(void *),
                                        nullptr,
                                        false
                    )
            );
        }
    }
    CUDA_CHECK(libreCuStreamCommence(stream));
    CUDA_CHECK(libreCuStreamAwait(stream));
    end = std::chrono::high_resolution_clock::now();
    std::cout << "5xSync kernel took: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << "ms" << std::endl;

    // free memory
    CUDA_CHECK(libreCuMemFree(float_dst_compute_va));
    CUDA_CHECK(libreCuMemFree(float_dst_dma_va));

    // destroy stream
    CUDA_CHECK(libreCuStreamDestroy(stream));

    // unload module
    CUDA_CHECK(libreCuModuleUnload(module));

    // destroy ctx
    CUDA_CHECK(libreCuCtxDestroy(ctx));
    return 0;
}