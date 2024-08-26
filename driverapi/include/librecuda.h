#ifndef LIBRECUDA_H_
#define LIBRECUDA_H_

#include <cstdint>
#include "librecuda_status.h"

#define LIBRECUDA_EXPORT extern "C"

struct LibreCUdevice_;
struct LibreCUcontext_;
struct LibreCUmodule_;
struct LibreCUFunction_;
struct LibreCUstream_;

typedef LibreCUdevice_ *LibreCUdevice;
typedef LibreCUcontext_ *LibreCUcontext;
typedef LibreCUmodule_ *LibreCUmodule;
typedef LibreCUFunction_ *LibreCUFunction;
typedef LibreCUstream_ *LibreCUstream;

#define CU_CTX_SCHED_SPIN 0x01
#define CU_CTX_SCHED_YIELD 0x02
#define CU_CTX_SCHED_BLOCKING_SYNC 0x04

LIBRECUDA_EXPORT libreCudaStatus_t libreCuInit(int flags);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuDeviceGetCount(int *pDeviceCount);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuDeviceGet(LibreCUdevice *device, int deviceOrdinal);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuGetErrorString(libreCudaStatus_t status, const char **pStatusName);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuCtxCreate_v2(LibreCUcontext *pCtx, int flags, LibreCUdevice device);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuCtxDestroy(LibreCUcontext ctx);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuCtxSetCurrent(LibreCUcontext ctx);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuCtxGetCurrent(LibreCUcontext *pCtxOut);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuMemAlloc(void **pDevicePointer, size_t bytesize, bool mapToCpu = false);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuMemCpy(void *dst, void *src, size_t byteCount, LibreCUstream stream);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuMemFree(void *devicePointer);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuStreamCreate(LibreCUstream *pStreamOut, uint32_t flags);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuStreamDestroy(LibreCUstream stream);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuDeviceGetName(char *pDeviceName, int length, LibreCUdevice device);

LIBRECUDA_EXPORT libreCudaStatus_t
libreCuModuleLoadData(LibreCUmodule *pModuleOut, const void *image, size_t imageSize);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuModuleGetFunctionCount(uint32_t *count, LibreCUmodule mod);

LIBRECUDA_EXPORT libreCudaStatus_t
libreCuModuleEnumerateFunctions(LibreCUFunction *functionsOut, uint32_t numFunctions, LibreCUmodule mod);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuFuncGetName(const char **pNameOut, LibreCUFunction func);

LIBRECUDA_EXPORT libreCudaStatus_t
libreCuModuleGetFunction(LibreCUFunction *pFunc, LibreCUmodule module, const char *name);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuLaunchKernel(LibreCUFunction function,
                                                       uint32_t gridDimX, uint32_t gridDimY,
                                                       uint32_t gridDimZ,
                                                       uint32_t blockDimX, uint32_t blockDimY,
                                                       uint32_t blockDimZ,
                                                       uint32_t sharedMemBytes, LibreCUstream stream,
                                                       void **kernelParams, size_t numParams,
                                                       void **extra);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuStreamCommence(LibreCUstream stream);
LIBRECUDA_EXPORT libreCudaStatus_t libreCuStreamAwait(LibreCUstream stream);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuModuleUnload(LibreCUmodule module);

#endif //LIBRECUDA_H_