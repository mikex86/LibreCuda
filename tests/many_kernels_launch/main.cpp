#include <chrono>
#include <fstream>
#include <vector>
#include <cstring>
#include <librecuda.h>

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
    size_t n_bytes; {
        std::ifstream input("empty_kernel.cubin", std::ios::binary);
        std::vector<uint8_t> bytes(
            (std::istreambuf_iterator<char>(input)),
            (std::istreambuf_iterator<char>()));
        input.close();
        image = new uint8_t[bytes.size()];
        std::memcpy(image, bytes.data(), bytes.size());
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
    CUDA_CHECK(libreCuModuleGetFunction(&func, module, "emtpy_kernel"));

    // create stream
    LibreCUstream stream{};
    CUDA_CHECK(libreCuStreamCreate(&stream, 0));

    void *params[] = {};
    size_t num_kernels = 1025;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_kernels; ++i) {
        CUDA_CHECK(libreCuLaunchKernel(func,
                            1, 1, 1,
                            1, 1, 1,
                            0,
                            stream,
                            params, sizeof(params) / sizeof(void *),
                            nullptr
        ));
    }
    // dispatch built up command buffer to GPU
    CUDA_CHECK(libreCuStreamCommence(stream));
    auto end = std::chrono::high_resolution_clock::now();

    // wait for work to complete
    CUDA_CHECK(libreCuStreamAwait(stream));

    // Calculate the duration in seconds as a double
    std::chrono::duration<double> elapsed = end - start;
    double elapsedSeconds = elapsed.count();

    // Print the elapsed time
    std::cout << "Average time: " << elapsedSeconds / num_kernels << ", Total: " << elapsedSeconds;

    // destroy stream
    CUDA_CHECK(libreCuStreamDestroy(stream));

    // unload module
    CUDA_CHECK(libreCuModuleUnload(module));

    // destroy ctx
    CUDA_CHECK(libreCuCtxDestroy(ctx));
    return 0;
}
