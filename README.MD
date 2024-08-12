# LibreCUDA

LibreCUDA is a project aimed at replacing the CUDA driver API to enable launching CUDA code on Nvidia GPUs without relying on the proprietary CUDA runtime. It achieves this by communicating directly with the hardware via ioctls, (specifically what Nvidia's open-gpu-kernel-modules refer to as the rmapi), as well as QMD, Nvidia's MMIO command queue structure. LibreCUDA is capable of uploading CUDA ELF binaries onto the GPU and launching them via the command queue.

## Current features

- Allocate and free gpu memory & map the memory to be accessible by the CPU
- Capable of uploading CUDA ELF binaries to the GPU
- Launches CUDA kernels via the command queue

## Example

Below is an example demonstrating the usage of LibreCUDA:

```cpp
int main() {
    libreCuInit(0);

    int device_count{};
    libreCuDeviceGetCount(&device_count);
    std::cout << "Device count: " + std::to_string(device_count) << std::endl;

    LibreCUdevice device{};
    libreCuDeviceGet(&device, 0);

    LibreCUcontext ctx{};
    libreCuCtxCreate_v2(&ctx, CU_CTX_SCHED_YIELD, device);

    LibreCUmodule module{};

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
    libreCuModuleLoadData(&module, image, n_bytes);

    uint32_t num_funcs{};
    libreCuModuleGetFunctionCount(&num_funcs, module);
    std::cout << "Num functions: " << num_funcs << std::endl;

    auto *functions = new LibreCUFunction[num_funcs];
    libreCuModuleEnumerateFunctions(functions, num_funcs, module);

    for (size_t i = 0; i < num_funcs; i++) {
        LibreCUFunction func = functions[i];
        const char *func_name{};
        libreCuFuncGetName(&func_name, func);
        std::cout << "  function \"" << func_name << "\"" << std::endl;
    }

    delete[] functions;

    LibreCUFunction func{};
    libreCuModuleGetFunction(&func, module, "write_float");

    LibreCUstream stream{};
    libreCuStreamCreate(&stream, 0);

    void *float_dst_va{};
    libreCuMemAlloc(&float_dst_va, sizeof(float), true);

    float float_value = 3.1415f;
    void *float_src_va{};
    libreCuMemAlloc(&float_src_va, sizeof(float), true);
    *(float *) (float_src_va) = float_value;

    std::cout << "Src value: " << float_value << std::endl;
    std::cout << "Dst value (pre exec): " << *(float *) (float_dst_va) << std::endl;

    void *params[] = {
            &float_dst_va, // dst
            &float_src_va // src
    };
    libreCuLaunchKernel(func,
                        1, 1, 1,
                        1, 1, 1,
                        0,
                        stream,
                        params, sizeof(params) / sizeof(void *),
                        nullptr
    );

    libreCuStreamCommence(stream);
    
    libreCuStreamAwait(stream);
    std::cout << "Dst value (post exec): " << *(float *) (float_dst_va) << std::endl;

    libreCuMemFree(float_dst_va);
    libreCuStreamDestroy(stream);
    libreCuModuleUnload(module);
    libreCuCtxDestroy(ctx);
    return 0;
}
```

### Outputs

```console
Device count: 1
Num functions: 1
  function "write_float"
Src value: 3.1415
Dst value (pre exec): 0
Dst value (post exec): 3.1415
```

## Project Status

The project is in its early stages and currently implements only rudimentary CUDA functions. It is not yet ready for production use.

## Contributing

Contributions are welcome! Please submit issues and pull requests to help improve LibreCUDA.

## License

This project is licensed under the MIT License.
