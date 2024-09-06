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
struct LibreCUEvent_;

typedef LibreCUdevice_ *LibreCUdevice;
typedef LibreCUcontext_ *LibreCUcontext;
typedef LibreCUmodule_ *LibreCUmodule;
typedef LibreCUFunction_ *LibreCUFunction;
typedef LibreCUstream_ *LibreCUstream;
typedef LibreCUEvent_ *LibreCUEvent;

#define CU_CTX_SCHED_SPIN 0x01
#define CU_CTX_SCHED_YIELD 0x02
#define CU_CTX_SCHED_BLOCKING_SYNC 0x04

enum LibreCuDeviceAttribute {
    CU_DEVICE_ATTRIBUTE_MAX_SHARED_MEMORY_PER_BLOCK = 8,
    CU_DEVICE_ATTRIBUTE_MAX_SHARED_MEMORY_PER_BLOCK_OPTIN = 97
};
enum LibreCuFunctionAttribute {
    CU_FUNC_ATTRIBUTE_MAX_DYNAMIC_SHARED_SIZE_BYTES = 8,
};

LIBRECUDA_EXPORT libreCudaStatus_t libreCuInit(int flags);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuDeviceGetCount(int *pDeviceCount);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuDeviceGet(LibreCUdevice *device, int deviceOrdinal);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuGetErrorString(libreCudaStatus_t status, const char **pStatusName);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuCtxCreate_v2(LibreCUcontext *pCtx, int flags, LibreCUdevice device);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuCtxDestroy(LibreCUcontext ctx);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuCtxSetCurrent(LibreCUcontext ctx);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuCtxGetCurrent(LibreCUcontext *pCtxOut);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuMemAlloc(void **pDevicePointer, size_t bytesize, bool mapToCpu = false);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuMemCpy(void *dst, void *src, size_t byteCount, LibreCUstream stream, bool async = false);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuMemFree(void *devicePointer);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuStreamCreate(LibreCUstream *pStreamOut, uint32_t flags);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuStreamDestroy(LibreCUstream stream);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuDeviceGetName(char *pDeviceName, int length, LibreCUdevice device);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuDeviceGetAttribute(int *pValOut, LibreCuDeviceAttribute attribute, LibreCUdevice device);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuFuncSetAttribute(LibreCUFunction function, LibreCuFunctionAttribute attribute, int value);

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
                                                       void **extra,
                                                       bool async=false);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuEventCreate(LibreCUEvent *pEventOut, uint32_t flags);
LIBRECUDA_EXPORT libreCudaStatus_t libreCuEventRecord(LibreCUEvent event, LibreCUstream stream);
LIBRECUDA_EXPORT libreCudaStatus_t libreCuEventSynchronize(LibreCUEvent event);
LIBRECUDA_EXPORT libreCudaStatus_t libreCuEventElapsedTime(float *pMillisecondsOut, LibreCUEvent start, LibreCUEvent end);
LIBRECUDA_EXPORT libreCudaStatus_t libreCuEventDestroy(LibreCUEvent event);

/**
 * Submits the built up command buffer to the gpu.
 * Operations performed on streams fall into two types: "compute" (eg. launch kernel) and "dma".
 * "dma" operations are eg. host to device / device to host memcpy operations.
 * NOTE: Device to device copies are implemented as a compute kernel, so they DO NOT COUNT AS DMA!
 * If only operations of a single type are performed, commence will not block and return near instantly.
 * To correctly interleave compute and dma operations, not all operations can be dispatched immediately and cpu-side
 * waiting is required to dispatch operations in order.
 * The more context switches between "compute" and "dma" mode you encounter, the more often cpu-side synchronization can
 * slow down execution of the timeline.
 * libreCuStreamCommence will block for the same amount of time as if libreCuStreamAwait was called, however
 * this behavior should not be relied upon. Always call libreCuStreamAwait for safety.
 */
LIBRECUDA_EXPORT libreCudaStatus_t libreCuStreamCommence(LibreCUstream stream);
LIBRECUDA_EXPORT libreCudaStatus_t libreCuStreamAwait(LibreCUstream stream);

LIBRECUDA_EXPORT libreCudaStatus_t libreCuModuleUnload(LibreCUmodule module);

#endif //LIBRECUDA_H_