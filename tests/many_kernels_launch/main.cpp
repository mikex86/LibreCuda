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

    size_t num_kernels = 1025;
    LibreCUmodule modules[num_kernels];
    for (int i = 0; i < num_kernels; i++) {
        CUDA_CHECK(libreCuModuleLoadData(modules + i, image, n_bytes));
    }

    // find functions
    LibreCUFunction funcs[num_kernels];
    for (int i = 0; i < num_kernels; i++) {
        CUDA_CHECK(libreCuModuleGetFunction(funcs + i, modules[i], "emtpy_kernel"));
    }

    // create stream
    LibreCUstream stream{};
    CUDA_CHECK(libreCuStreamCreate(&stream, 0));

    void *params[] = {};

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_kernels; ++i) {
        CUDA_CHECK(libreCuLaunchKernel(funcs[i],
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
    for (int i = 0; i < num_kernels; ++i) {
        CUDA_CHECK(libreCuModuleUnload(modules[i]));
    }

    // destroy ctx
    CUDA_CHECK(libreCuCtxDestroy(ctx));
    return 0;
}
