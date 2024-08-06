#ifndef LIBRECUDA_H_
#define LIBRECUDA_H_

#include <cstdint>
#include "librecuda_status.h"

#define LIBRECUDA_EXPORT extern "C"

struct LibreCUdevice_;
struct LibreCUcontext_;
struct LibreCUmodule_;
struct LibreCUFunction_;

typedef LibreCUdevice_ *LibreCUdevice;
typedef LibreCUcontext_ *LibreCUcontext;
typedef LibreCUmodule_ *LibreCUmodule;
typedef LibreCUFunction_ *LibreCUFunction;

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

LIBRECUDA_EXPORT libreCudaStatus_t libreCuMemAlloc(void **pDevicePointer, size_t bytesize);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuMemCpy(void *dst, void *src, size_t byteCount);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuMemFree(void *devicePointer);

LIBRECUDA_EXPORT libreCudaStatus_t
libreCuModuleLoadData(LibreCUmodule *pModuleOut, const void *image, size_t imageSize);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuModuleGetFunctionCount(uint32_t *count, LibreCUmodule mod);

LIBRECUDA_EXPORT libreCudaStatus_t
libreCuModuleEnumerateFunctions(LibreCUFunction *functionsOut, uint32_t numFunctions, LibreCUmodule mod);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuFuncGetName(const char **pNameOut, LibreCUFunction func);

LIBRECUDA_EXPORT libreCudaStatus_t
libreCuModuleGetFunction(LibreCUFunction *pFunc, LibreCUmodule module, const char *name);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuLaunchKernel();

LIBRECUDA_EXPORT libreCudaStatus_t libreCuModuleUnload(LibreCUmodule module);

#endif //LIBRECUDA_H_