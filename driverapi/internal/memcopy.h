#pragma once

#include <librecuda.h>
#include <cstddef>

libreCudaStatus_t loadMemcpyKernelsIfNeeded(LibreCUdevice device);

libreCudaStatus_t memcpyD2D(void *dst, void *src, size_t size, LibreCUstream stream, bool async);