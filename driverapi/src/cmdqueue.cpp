#include "cmdqueue.h"

#include <algorithm>
#include <thread>
#include <cstring>

#include "nvidia/clc6c0.h"
#include "nvidia/clc6b5.h"
#include "nvidia/g_allclasses.h"

#include "librecuda_internal.h"

#include "nvidia/clc6c0qmd.h"
#include "nvidia/nvmisc.h"

#define ENSURE_QUEUE_INITIALIZED() LIBRECUDA_VALIDATE(initialized, LIBRECUDA_ERROR_NOT_INITIALIZED)


#define MAX_CMD_QUEUE_PAGE_SIZE 0x200000
#define MAX_SIGNAL_POOL_SIZE 65536

#define KERNARGS_PAGE_SIZE (16 << 20)

static inline NvU32 hi_32(NvU64 value) {
    return static_cast<NvV32>(value >> 32);
}

static inline NvU32 lo_32(NvU64 value) {
    return static_cast<NvV32>(value & 0xFFFFFFFF);
}

#define U64_HI_32_BITS(value) hi_32(reinterpret_cast<NvU64>(value))
#define U64_LO_32_BITS(value) lo_32(reinterpret_cast<NvU64>(value))

struct LibreCUEvent_ {
    LibreCUstream_ *stream;
    NvSignal *computeSignal{};
    NvSignal *dmaSignal{};
};

LibreCUEvent_ *NewEvent() {
    return new LibreCUEvent_{};
}

LibreCUstream_ *EventGetStream(LibreCUEvent_ *pEvent) {
    return pEvent->stream;
}

void DeleteEvent(LibreCUEvent_ *pEvent) {
    delete pEvent;
}

static NvMethod makeNvMethod(int subcommand, int method, int size, int typ) {
    return (typ << 28) | (size << 16) | (subcommand << 13) | (method >> 2);
}

NvCommandQueue::NvCommandQueue(LibreCUcontext ctx) : ctx(ctx) {

}

libreCudaStatus_t NvCommandQueue::submitToFifo(QueueType queueType) {
    auto &commandBuffer = queueType == COMPUTE ? computeCommandBuffer : dmaCommandBuffer;

    if (commandBuffer.empty()) {
        return LIBRECUDA_SUCCESS; // don't do anything when command buffer is emtpy
    }

    auto &page = queueType == COMPUTE ? computeQueuePage : dmaQueuePage;
    auto &gpfifo = queueType == COMPUTE ? ctx->compute_gpfifo : ctx->dma_gpfifo;

    if ((page.commandWriteIdx + commandBuffer.size()) > MAX_CMD_QUEUE_PAGE_SIZE) {
        LIBRECUDA_VALIDATE(NvU64(gpfifo.ring[gpfifo.controls->GPGet] & 0xFFFFFFFFFC) >=
                           NvU64(page.commandQueueSpace + commandBuffer.size()) ||
                           gpfifo.controls->GPGet == gpfifo.controls->GPPut, LIBRECUDA_ERROR_UNKNOWN);
        page.commandWriteIdx = 0;
    }
    memcpy(page.commandQueueSpace + page.commandWriteIdx, commandBuffer.data(), commandBuffer.size() * sizeof(NvU32));

    gpfifo.ring[gpfifo.put_value % gpfifo.entries_count] =
            ((NvU64(page.commandQueueSpace + page.commandWriteIdx) / sizeof(NvU32)) << 2) |
            (commandBuffer.size() << 42) | (1l << 41);
    page.commandWriteIdx += commandBuffer.size();

    gpfifo.controls->GPPut = (gpfifo.put_value + 1) % gpfifo.entries_count;
    ctx->device->gpu_mmio[0x90 / 4] = gpfifo.token;
    gpfifo.put_value++;

    commandBuffer.clear();

    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t NvCommandQueue::initializeQueue() {
    // allocate compute command queue page
    {
        NvU64 cmdq_va{};
        LIBRECUDA_ERR_PROPAGATE(
                gpuAlloc(ctx, MAX_CMD_QUEUE_PAGE_SIZE * sizeof(NvU32), false, false, true, 0, &cmdq_va));
        computeQueuePage.commandQueueSpace = reinterpret_cast<NvU32 *>(cmdq_va);
    }

    // allocate dma command queue page
    {
        NvU64 cmdq_va{};
        LIBRECUDA_ERR_PROPAGATE(
                gpuAlloc(ctx, MAX_CMD_QUEUE_PAGE_SIZE * sizeof(NvU32), false, false, true, 0, &cmdq_va));
        dmaQueuePage.commandQueueSpace = reinterpret_cast<NvU32 *>(cmdq_va);
    }

    // allocate signal pool
    {
        NvU64 signal_pool_va{};
        LIBRECUDA_ERR_PROPAGATE(
                gpuSystemAlloc(ctx, sizeof(NvSignal) * MAX_SIGNAL_POOL_SIZE, true, 0, &signal_pool_va)
        );
        signalPool = reinterpret_cast<NvSignal *>(signal_pool_va);

        // consider all signals free
        {
            for (int i = 0; i < MAX_SIGNAL_POOL_SIZE; i++) {
                freeSignals.push_back(i);
            }
        }
    }

    // create timeline signal
    {
        LIBRECUDA_ERR_PROPAGATE(obtainSignal(&timelineSignal));
    }

    // setup compute queue
    {
        LIBRECUDA_ERR_PROPAGATE(
                enqueue(makeNvMethod(1, NVC6C0_SET_OBJECT, 1), {ctx->device->compute_class}, COMPUTE)
        );
        LIBRECUDA_ERR_PROPAGATE(
                enqueue(
                        makeNvMethod(1, NVC6C0_SET_SHADER_LOCAL_MEMORY_WINDOW_A, 2),
                        {U64_HI_32_BITS(local_mem_window), U64_LO_32_BITS(local_mem_window)},
                        COMPUTE
                )
        );
        LIBRECUDA_ERR_PROPAGATE(
                enqueue(
                        makeNvMethod(1, NVC6C0_SET_SHADER_SHARED_MEMORY_WINDOW_A, 2),
                        {U64_HI_32_BITS(shared_mem_window), U64_LO_32_BITS(shared_mem_window)},
                        COMPUTE
                )
        );
        timelineCtr++;
        LIBRECUDA_ERR_PROPAGATE(signalNotify(timelineSignal, timelineCtr, COMPUTE));
    }
    LIBRECUDA_ERR_PROPAGATE(startExecution(COMPUTE));
    LIBRECUDA_ERR_PROPAGATE(awaitExecution());

    // setup copy queue
    {
        LIBRECUDA_ERR_PROPAGATE(
                enqueue(makeNvMethod(4, NVC6C0_SET_OBJECT, 1), {AMPERE_DMA_COPY_B}, DMA)
        );
        timelineCtr++;
        LIBRECUDA_ERR_PROPAGATE(signalNotify(timelineSignal, timelineCtr, DMA));
    }

    LIBRECUDA_ERR_PROPAGATE(startExecution(DMA));
    LIBRECUDA_ERR_PROPAGATE(awaitExecution());

    // allocate kernargs page
    {
        LIBRECUDA_ERR_PROPAGATE(
                gpuAlloc(
                        ctx,
                        KERNARGS_PAGE_SIZE,
                        false,
                        true,
                        true,
                        0,
                        reinterpret_cast<NvU64 *>(&kernArgsPageVa)
                )
        )
    }

    initialized = true;

    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t NvCommandQueue::allocKernArgs(NvU64 *pMemOut, size_t size) {
    LIBRECUDA_VALIDATE(pMemOut != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    if ((kernArgsWriteIdx + size) > KERNARGS_PAGE_SIZE) {
        LIBRECUDA_FAIL(LIBRECUDA_ERROR_LAUNCH_OUT_OF_RESOURCES);
    }
    NvU8 *ptr = kernArgsPageVa + kernArgsWriteIdx;
    kernArgsWriteIdx += size;
    *pMemOut = reinterpret_cast<NvU64>(ptr);

    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t NvCommandQueue::enqueue(NvMethod method, std::initializer_list<NvU32> arguments, QueueType type) {
    auto &commandBuffer = type == COMPUTE ? computeCommandBuffer : dmaCommandBuffer;
    // copy method bytes
    {
        commandBuffer.push_back(method);
    }

    // copy argument bytes
    {
        size_t n_arguments = method >> 16 & 0xFFF;
        LIBRECUDA_VALIDATE(n_arguments == arguments.size(), LIBRECUDA_ERROR_INVALID_VALUE);

        for (NvU32 arg: arguments) {
            commandBuffer.push_back(arg);
        }
    }

    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t NvCommandQueue::obtainSignal(NvSignal **pSignalPtr) {
    LIBRECUDA_VALIDATE(pSignalPtr != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(signalPool != nullptr, LIBRECUDA_ERROR_NOT_INITIALIZED);

    if (freeSignals.empty()) {
        LIBRECUDA_FAIL(LIBRECUDA_ERROR_UNKNOWN);
    }

    NvU32 signalIdx = freeSignals[0];
    freeSignals.pop_front();

    NvSignal *signalPointer = signalPool + signalIdx;
    *pSignalPtr = signalPointer;

    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t NvCommandQueue::releaseSignal(NvSignal *signal) {
    LIBRECUDA_VALIDATE(signal != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);

    // compute signal index from pointer
    NvU32 signalIdx = signal - signalPool;

    // ensure pointer was within signal pool and thus valid
    if (signalIdx >= MAX_SIGNAL_POOL_SIZE) {
        LIBRECUDA_FAIL(LIBRECUDA_ERROR_INVALID_VALUE);
    }

    // check if the signal has already been released
    if (std::find(freeSignals.begin(), freeSignals.end(), signalIdx) != freeSignals.end()) {
        LIBRECUDA_FAIL(LIBRECUDA_ERROR_INVALID_VALUE);
    }

    freeSignals.push_back(signalIdx);
    LIBRECUDA_SUCCEED();
}

NvCommandQueue::~NvCommandQueue() {
    if (!initialized) {
        return;
    }
    // we unfortunately can't validate errors in the destructor,
    // but this is fairly safe and shouldn't fail unless somebody
    // frees the pool or command page
    releaseSignal(timelineSignal);

    gpuFree(ctx, reinterpret_cast<NvU64>(signalPool));
    gpuFree(ctx, reinterpret_cast<NvU64>(computeQueuePage.commandQueueSpace));
    gpuFree(ctx, reinterpret_cast<NvU64>(dmaQueuePage.commandQueueSpace));
    gpuFree(ctx, reinterpret_cast<NvU64>(kernArgsPageVa));
}

libreCudaStatus_t NvCommandQueue::awaitExecution() {
    LIBRECUDA_VALIDATE(timelineSignal != nullptr, LIBRECUDA_ERROR_NOT_INITIALIZED);
    LIBRECUDA_ERR_PROPAGATE(signalWaitCpu(timelineSignal, timelineCtr));
    kernArgsWriteIdx = 0; // reset kern args write idx
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t NvCommandQueue::signalNotify(NvSignal *pSignal, NvU32 signalTarget, QueueType type) {
    LIBRECUDA_VALIDATE(pSignal != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);

    switch (type) {
        case COMPUTE: {
            LIBRECUDA_ERR_PROPAGATE(enqueue(
                    makeNvMethod(0, NVC56F_SEM_ADDR_LO, 5),
                    {
                            // little endian
                            U64_LO_32_BITS(pSignal),
                            U64_HI_32_BITS(pSignal),

                            // little endian
                            signalTarget,
                            0,

                            (NVC56F_SEM_EXECUTE_OPERATION_RELEASE) | (NVC56F_SEMAPHORED_RELEASE_WFI_DIS << 20) |
                            (NVC56F_SEM_EXECUTE_PAYLOAD_SIZE_64BIT << 24) |
                            (NVC56F_SEM_EXECUTE_RELEASE_TIMESTAMP_EN
                                    << 25)
                    },
                    COMPUTE
            ));
            LIBRECUDA_ERR_PROPAGATE(
                    enqueue(
                            makeNvMethod(4, NVC56F_NON_STALL_INTERRUPT, 1),
                            {0x0},
                            COMPUTE
                    )
            );
            break;
        }
        case DMA: {
            LIBRECUDA_ERR_PROPAGATE(enqueue(
                    makeNvMethod(4, NVC6B5_SET_SEMAPHORE_A, 3),
                    {
                            U64_HI_32_BITS(pSignal),
                            U64_LO_32_BITS(pSignal),

                            signalTarget
                    },
                    DMA
            ));
            LIBRECUDA_ERR_PROPAGATE(enqueue(
                    makeNvMethod(4, NVC6B5_LAUNCH_DMA, 1),
                    {0x14}, // also don't know what this means
                    DMA
            ));
            break;
        }
    }

    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t NvCommandQueue::signalWaitCpu(NvSignal *pSignal, NvU32 signalTarget) {
    LIBRECUDA_VALIDATE(pSignal != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);

    // handle default case if none specified
    {
        if ((ctx->flags & CU_CTX_SCHED_SPIN) == 0
            && (ctx->flags & CU_CTX_SCHED_YIELD) == 0
            && (ctx->flags & CU_CTX_SCHED_BLOCKING_SYNC) == 0) {
            ctx->flags |= CU_CTX_SCHED_SPIN;
        }
    }

    if (ctx->flags & CU_CTX_SCHED_SPIN) {
        while (pSignal->value < signalTarget) {
        }
    } else if (ctx->flags & CU_CTX_SCHED_YIELD) {
        while (pSignal->value < signalTarget) {
            std::this_thread::yield();
        }
    } else if (ctx->flags & CU_CTX_SCHED_BLOCKING_SYNC) {
        throw std::runtime_error("TODO: not yet implemented");
    }
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t NvCommandQueue::startExecution(QueueType queueType) {
    LIBRECUDA_ERR_PROPAGATE(submitToFifo(
            queueType
    ));
    LIBRECUDA_SUCCEED();
}

static inline NvU64 ceilDiv(NvU64 a, NvU64 b) {
    return (a + b - 1) / b;
}

static inline int maxOf(int a, int b) {
    return (a > b) ? a : b;
}

static inline int ceilDiv(int a, int b) {
    return (a + b - 1) / b;
}

static inline int roundUp(int a, int b) {
    return ceilDiv(a, b) * b;
}

static inline NvU64 roundUp(NvU64 a, NvU64 b) {
    return ceilDiv(a, b) * b;
}

static inline NvU32 maxOf(NvU32 a, NvU32 b) {
    return (a > b) ? a : b;
}

static inline NvU32 minOf(NvU32 a, NvU32 b) {
    return (a < b) ? a : b;
}

static inline NvU32 ceilDiv(NvU32 a, NvU32 b) {
    return (a + b - 1) / b;
}

static inline NvU32 roundUp(NvU32 a, NvU32 b) {
    return ceilDiv(a, b) * b;
}


libreCudaStatus_t NvCommandQueue::ensureEnoughLocalMem(LibreCUFunction function) {
    NvU32 localMemReq = function->local_mem_req;
    if (localMemReq <= currentSlmPerThread) {
        return LIBRECUDA_SUCCESS; // no action required, local memory is enough
    }

    if (function->shader_local_memory_va != 0) {
        LIBRECUDA_ERR_PROPAGATE(gpuFree(ctx, function->shader_local_memory_va));
    }

    currentSlmPerThread = ceilDiv(localMemReq, 32u) * 32; // round up
    NvU64 bytes_per_warp = ceilDiv(currentSlmPerThread * 32, 0x200u) * 0x200; // round up
    NvU64 bytes_per_tpc = ceilDiv(bytes_per_warp * 48 * 2, 0x8000ul) * 0x8000; // round up

    size_t nSlmBytes = ceilDiv(bytes_per_tpc * 64, 0x20000ul) * 0x20000; // round up
    LIBRECUDA_ERR_PROPAGATE(
            gpuAlloc(
                    ctx,
                    nSlmBytes,
                    true,
                    true,
                    false,
                    0,
                    &function->shader_local_memory_va
            )
    );

    {
        // set shader local memory pointer
        LIBRECUDA_ERR_PROPAGATE(enqueue(
                makeNvMethod(1, NVC6C0_SET_SHADER_LOCAL_MEMORY_A, 2),
                {
                        U64_HI_32_BITS(function->shader_local_memory_va),
                        U64_LO_32_BITS(function->shader_local_memory_va)
                },
                COMPUTE
        ));
        LIBRECUDA_ERR_PROPAGATE(enqueue(
                makeNvMethod(1, NVC6C0_SET_SHADER_LOCAL_MEMORY_NON_THROTTLED_A, 3),

                {
                        U64_HI_32_BITS(bytes_per_tpc),
                        U64_LO_32_BITS(bytes_per_tpc),

                        0x40 // whatever this means...
                },
                COMPUTE
        ));
    }

    LIBRECUDA_SUCCEED();
}


typedef uint32_t qmd_cmd_t[0x40];

libreCudaStatus_t
NvCommandQueue::launchFunction(LibreCUFunction function,
                               uint32_t gridDimX, uint32_t gridDimY, uint32_t gridDimZ,
                               uint32_t blockDimX, uint32_t blockDimY, uint32_t blockDimZ,
                               uint32_t sharedMemBytes,
                               void **params, size_t numParams,
                               bool async) {
    LIBRECUDA_VALIDATE(function != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(numParams == function->param_info.size(), LIBRECUDA_ERROR_INVALID_VALUE);

    if (dmaCommandBuffer.empty()) {
        currentQueueType = COMPUTE;
    }
    if (currentQueueType == DMA) {
        backlogCurrentCmdBuffer(DMA);
        currentQueueType = COMPUTE;
    }

    LIBRECUDA_ERR_PROPAGATE(ensureEnoughLocalMem(function));


    if (!async && timelineNotifyPending) {
        LIBRECUDA_ERR_PROPAGATE(signalNotify(timelineSignal, timelineCtr, COMPUTE));
        timelineNotifyPending = false;
    }

    if (!async) {
        LIBRECUDA_ERR_PROPAGATE(signalWaitGpu(timelineSignal, timelineCtr));
    }

    // prepare constbuf0
    NvU32 constbuf0_data[88] = {};
    {
        // populate ntid registers (blockDim.x, ...)
        constbuf0_data[0] = blockDimX;
        constbuf0_data[1] = blockDimY;
        constbuf0_data[2] = blockDimZ;

        // populate nctaid registers (gridDim.x, ...)
        constbuf0_data[3] = gridDimX;
        constbuf0_data[4] = gridDimY;
        constbuf0_data[5] = gridDimZ;

        // little endian
        constbuf0_data[6] = U64_LO_32_BITS(shared_mem_window);
        constbuf0_data[7] = U64_HI_32_BITS(shared_mem_window);

        constbuf0_data[8] = U64_LO_32_BITS(local_mem_window);
        constbuf0_data[9] = U64_HI_32_BITS(local_mem_window);

        constbuf0_data[10] = 0xfffdc0;
        constbuf0_data[11] = 0;
    }

    // validate const0 exists
    LIBRECUDA_VALIDATE(!function->constants.empty(), LIBRECUDA_ERROR_INVALID_IMAGE);

    // prepare param pointer
    NvU64 kernargs_pointer{};
    size_t kernargs_size = roundUp(function->constants[0].size, NvU64(1 << 8)) + (8 << 8);
    {
        LIBRECUDA_ERR_PROPAGATE(allocKernArgs(&kernargs_pointer, kernargs_size));

        {
            auto kernargs_buf = reinterpret_cast<void *>(kernargs_pointer);
            LIBRECUDA_VALIDATE(sizeof(constbuf0_data) < kernargs_size, LIBRECUDA_ERROR_INVALID_IMAGE);
            memcpy(kernargs_buf, constbuf0_data, sizeof(constbuf0_data));
        }
        {
            auto kernargs_buf = reinterpret_cast<NvU32 *>(kernargs_pointer + sizeof(constbuf0_data));
            LIBRECUDA_VALIDATE((sizeof(constbuf0_data) + (numParams * 2 * sizeof(NvU32))) < kernargs_size,
                               LIBRECUDA_ERROR_INVALID_IMAGE);

            size_t j = 0;
            for (size_t i = 0; i < numParams; i++) {
                size_t param_size = function->param_info[i].param_size;
                switch (param_size) {
                    case 8: {
                        auto *param_ptr = reinterpret_cast<NvU64 *>(params[i]);
                        auto param_value = *param_ptr;
                        kernargs_buf[j++] = U64_LO_32_BITS(param_value);
                        kernargs_buf[j++] = U64_HI_32_BITS(param_value);
                        break;
                    }
                    case 4: {
                        auto *param_ptr = reinterpret_cast<NvU32 *>(params[i]);
                        auto param_value = *param_ptr;
                        kernargs_buf[j++] = param_value;
                        break;
                    }
                    case 2: {
                        auto *param_ptr = reinterpret_cast<NvU16 *>(params[i]);
                        auto param_value = *param_ptr;
                        kernargs_buf[j++] = param_value;
                        break;
                    }
                    case 1: {
                        auto *param_ptr = reinterpret_cast<NvU8 *>(params[i]);
                        auto param_value = *param_ptr;
                        kernargs_buf[j++] = param_value;
                        break;
                    }
                    default: {
                        if (param_size % sizeof(NvU32) != 0) {
                            // cuda encodes everything with these 32-bit words. The fact that this would be allowed is highly
                            // implausible given that even most c compilers pad struct lengths to multiples of 4 anyway,
                            // so cuda doing it any different would be highly implausible
                            LIBRECUDA_DEBUG(
                                    "Encountered kernel with array parameter with size % 4 != 0! This should not be possible");
                            LIBRECUDA_FAIL(LIBRECUDA_ERROR_INVALID_VALUE);
                        }
                        auto *param_ptr = reinterpret_cast<NvU32 *>(params[i]);
                        size_t num_words = param_size / sizeof(NvU32);
                        for (size_t k = 0; k < num_words; k++) {
                            kernargs_buf[j++] = param_ptr[k];
                        }
                        break;
                    }
                }
            }
        }
    }

    // check launch dimensions
    NvU32 max_threads = ((65536 / roundUp(maxOf(1u, function->num_registers) * 32, 256u)) / 4) * 4 * 32;

    uint32_t shmem_usage = maxOf(function->shared_mem, sharedMemBytes);

    NvU32 blockProd = blockDimX * blockDimY * blockDimZ;
    if ((shmem_usage > sharedMemBytes) && (blockProd > 1024 || max_threads < blockProd)) {
        LIBRECUDA_FAIL(LIBRECUDA_ERROR_LAUNCH_OUT_OF_RESOURCES);
    }

    LIBRECUDA_VALIDATE(blockDimX < 1024, LIBRECUDA_ERROR_LAUNCH_OUT_OF_RESOURCES);
    LIBRECUDA_VALIDATE(blockDimY < 1024, LIBRECUDA_ERROR_LAUNCH_OUT_OF_RESOURCES);
    LIBRECUDA_VALIDATE(blockDimZ < 64, LIBRECUDA_ERROR_LAUNCH_OUT_OF_RESOURCES);

    LIBRECUDA_VALIDATE(gridDimX < 2147483647, LIBRECUDA_ERROR_LAUNCH_OUT_OF_RESOURCES);
    LIBRECUDA_VALIDATE(gridDimY < 65535, LIBRECUDA_ERROR_LAUNCH_OUT_OF_RESOURCES);
    LIBRECUDA_VALIDATE(gridDimZ < 65535, LIBRECUDA_ERROR_LAUNCH_OUT_OF_RESOURCES);


    qmd_cmd_t qmd_data{};
    {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-branch-clone"

        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_QMD_GROUP_ID, , , 0x3F, qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_SM_GLOBAL_CACHING_ENABLE, , , 1, qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_INVALIDATE_TEXTURE_HEADER_CACHE, , , 1, qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_INVALIDATE_TEXTURE_SAMPLER_CACHE, , , 1, qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_INVALIDATE_TEXTURE_DATA_CACHE, , , 1, qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_INVALIDATE_SHADER_DATA_CACHE, , , 1, qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_API_VISIBLE_CALL_LIMIT, , , 1, qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_SAMPLER_INDEX, , , 1, qmd_data);

        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_CWD_MEMBAR_TYPE, , , NVC6C0_QMDV03_00_CWD_MEMBAR_TYPE_L1_SYSMEMBAR, qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_QMD_MAJOR_VERSION, , , 3, qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_CONSTANT_BUFFER_INVALIDATE(0), , , 1, qmd_data);

        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_SHARED_MEMORY_SIZE, , , maxOf(0x400u, roundUp(shmem_usage, 0x100u)),
                           qmd_data);

        // get smem config
        uint32_t smem_config;
        {
            uint32_t min = UINT32_MAX;
            for (auto shmem_conf: {32, 64, 100}) {
                if ((shmem_conf * 1024) >= shmem_usage) {
                    min = minOf(shmem_conf * 1024, min);
                }
            }
            smem_config = (min / 4096) + 1;
        }
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_MIN_SM_CONFIG_SHARED_MEM_SIZE, , , smem_config, qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_MAX_SM_CONFIG_SHARED_MEM_SIZE, , , 0x1A, qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_REGISTER_COUNT_V, , , function->num_registers, qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_TARGET_SM_CONFIG_SHARED_MEM_SIZE, , , smem_config, qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_BARRIER_COUNT, , , 1, qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_SHADER_LOCAL_MEMORY_HIGH_SIZE, , , currentSlmPerThread, qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_PROGRAM_PREFETCH_SIZE, , , function->function_size >> 8, qmd_data);

        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_PROGRAM_ADDRESS_LOWER, , , U64_LO_32_BITS(function->func_va_addr), qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_PROGRAM_ADDRESS_UPPER, , , U64_HI_32_BITS(function->func_va_addr), qmd_data);

        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_SASS_VERSION, , , 0x89,
                           qmd_data); // TODO: HARDCODING THIS SEEMS DESTINED FOR DISASTER
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_PROGRAM_PREFETCH_ADDR_LOWER_SHIFTED, , , function->func_va_addr >> 8,
                           qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_PROGRAM_PREFETCH_ADDR_UPPER_SHIFTED, , , function->func_va_addr >> 40,
                           qmd_data);

        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_CTA_RASTER_WIDTH, , , gridDimX, qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_CTA_RASTER_HEIGHT, , , gridDimY, qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_CTA_RASTER_DEPTH, , , gridDimZ, qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_CTA_THREAD_DIMENSION0, , , blockDimX, qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_CTA_THREAD_DIMENSION1, , , blockDimY, qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_CTA_THREAD_DIMENSION2, , , blockDimZ, qmd_data);

        // setup constant buffers
        const auto &constants = function->constants;
        for (auto constant_info: constants) {
            FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_CONSTANT_BUFFER_ADDR_UPPER(constant_info.const_nr), , ,
                               U64_HI_32_BITS(constant_info.address),
                               qmd_data);
            FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_CONSTANT_BUFFER_ADDR_LOWER(constant_info.const_nr), , ,
                               U64_LO_32_BITS(constant_info.address),
                               qmd_data);
            FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_CONSTANT_BUFFER_SIZE_SHIFTED4(constant_info.const_nr), , ,
                               constant_info.size, qmd_data);
            FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_CONSTANT_BUFFER_VALID(constant_info.const_nr), , ,
                               NVC6C0_QMDV03_00_CONSTANT_BUFFER_VALID_TRUE,
                               qmd_data);
        }

        // setup param pointer
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_CONSTANT_BUFFER_ADDR_UPPER(0), , , U64_HI_32_BITS(kernargs_pointer),
                           qmd_data);
        FLD_SET_DRF_NUM_MW(C6C0_QMDV03_00_CONSTANT_BUFFER_ADDR_LOWER(0), , , U64_LO_32_BITS(kernargs_pointer),
                           qmd_data);
#pragma clang diagnostic pop
    }


    // memory barrier
    {
        LIBRECUDA_ERR_PROPAGATE(enqueue(
                makeNvMethod(1, NVC6C0_INVALIDATE_SHADER_CACHES_NO_WFI, 1),
                {
                        (NVC6C0_INVALIDATE_SHADER_CACHES_NO_WFI_CONSTANT_TRUE << 12) |
                        (NVC6C0_INVALIDATE_SHADER_CACHES_NO_WFI_GLOBAL_DATA_TRUE << 4) |
                        (NVC6C0_INVALIDATE_SHADER_CACHES_NO_WFI_INSTRUCTION_TRUE << 0)
                },
                COMPUTE
        ));
    }

    // commence launch
    {
        NvU64 qmd_addr = kernargs_pointer + roundUp(function->constants[0].size, 1ul << 8);

        // the qmd struct needs to be copied after kernel args
        memcpy(reinterpret_cast<void *>(qmd_addr), qmd_data, sizeof(qmd_data));

        LIBRECUDA_ERR_PROPAGATE(enqueue(
                makeNvMethod(1, NVC6C0_SEND_PCAS_A, 0x1),
                {NvU32(qmd_addr >> 8)},
                COMPUTE
        ));

        LIBRECUDA_ERR_PROPAGATE(enqueue(
                makeNvMethod(1, NVC6C0_SEND_SIGNALING_PCAS2_B, 0x1),
                {0x9}, // TODO: whatever this means...
                COMPUTE
        ));
    }
    timelineCtr++;
    if (!async) {
        LIBRECUDA_ERR_PROPAGATE(signalNotify(timelineSignal, timelineCtr, COMPUTE));
        timelineNotifyPending = false;
    } else {
        // when async, we cannot do a signalNotify! This prevents parallelism, so we only do one
        // signalNotify at the end to advance the timelineSignal to the timelineCtr at once, which
        // may have been incremented multiple times.
        // timelineNotifyPending tells startExecution that we have to issue a signalNotify at the end
        // because a previous async kernel launch will not have issued this.
        // Without this, we would wait forever because the gpu would never modify the timelineSignal at all.
        timelineNotifyPending = true;
    }
    LIBRECUDA_SUCCEED();
}


libreCudaStatus_t NvCommandQueue::gpuMemcpy(void *dst, void *src, size_t numBytes, bool async) {
    LIBRECUDA_VALIDATE(dst != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(src != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(numBytes < UINT32_MAX, LIBRECUDA_ERROR_INVALID_VALUE);

    if (computeCommandBuffer.empty()) {
        currentQueueType = DMA;
    }

    if (currentQueueType == COMPUTE) {
        backlogCurrentCmdBuffer(COMPUTE);
        currentQueueType = DMA;
    }

    LIBRECUDA_ERR_PROPAGATE(enqueue(
            makeNvMethod(4, NVC6B5_OFFSET_IN_UPPER, 4),
            {
                    U64_HI_32_BITS(src),
                    U64_LO_32_BITS(src),

                    U64_HI_32_BITS(dst),
                    U64_LO_32_BITS(dst),
            },
            DMA
    ));

    LIBRECUDA_ERR_PROPAGATE(enqueue(
            makeNvMethod(4, NVC6B5_LINE_LENGTH_IN, 1),
            {
                    static_cast<NvU32>(numBytes)
            },
            DMA
    ));

    LIBRECUDA_ERR_PROPAGATE(enqueue(
            makeNvMethod(4, NVC6B5_LAUNCH_DMA, 1),
            {
                    (NVC6B5_LAUNCH_DMA_DATA_TRANSFER_TYPE_NON_PIPELINED << 0) |
                    (NVC6B5_LAUNCH_DMA_SRC_MEMORY_LAYOUT_PITCH << 7) |
                    (NVC6B5_LAUNCH_DMA_DST_MEMORY_LAYOUT_PITCH << 8)
            },
            DMA
    ));
    timelineCtr++;

    // same logic as for COMPUTE applies to DMA.
    if (!async) {
        LIBRECUDA_ERR_PROPAGATE(signalNotify(timelineSignal, timelineCtr, DMA));
        timelineNotifyPending = false;
    } else {
        timelineNotifyPending = true;
    }

    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t NvCommandQueue::backlogCurrentCmdBuffer(QueueType queueType) {
    switch (queueType) {
        case COMPUTE: {
            if (computeCommandBuffer.empty()) {
                return LIBRECUDA_SUCCESS;
            }
            commandBufBacklog.push_back(CommandBufSplit{
                    .commandBuffer=computeCommandBuffer,
                    .queueType=COMPUTE,
                    .timelineCtr=timelineCtr,
                    .timelineNotifyPending=timelineNotifyPending
            });
            computeCommandBuffer.clear();
            break;
        }
        case DMA: {
            if (dmaCommandBuffer.empty()) {
                return LIBRECUDA_SUCCESS;
            }
            commandBufBacklog.push_back(CommandBufSplit{
                    .commandBuffer=dmaCommandBuffer,
                    .queueType=DMA,
                    .timelineCtr=timelineCtr,
                    .timelineNotifyPending=timelineNotifyPending
            });
            dmaCommandBuffer.clear();
            break;
        }
    }
    timelineNotifyPending = false;
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t NvCommandQueue::startExecution() {
    if (!commandBufBacklog.empty()) {
        backlogCurrentCmdBuffer(COMPUTE);
        backlogCurrentCmdBuffer(DMA);

        for (const auto &backlog_entry: commandBufBacklog) {
            switch (backlog_entry.queueType) {
                case COMPUTE: {
                    computeCommandBuffer = backlog_entry.commandBuffer;
                    break;
                }
                case DMA: {
                    dmaCommandBuffer = backlog_entry.commandBuffer;
                    break;
                }
            }
            if (backlog_entry.timelineNotifyPending) {
                LIBRECUDA_ERR_PROPAGATE(
                        signalNotify(timelineSignal, backlog_entry.timelineCtr, backlog_entry.queueType)
                );
            }
            LIBRECUDA_ERR_PROPAGATE(submitToFifo(backlog_entry.queueType));
            LIBRECUDA_ERR_PROPAGATE(signalWaitCpu(timelineSignal, backlog_entry.timelineCtr));
        }
        commandBufBacklog.clear();
    } else {
        if (currentQueueType == COMPUTE) {
            LIBRECUDA_VALIDATE(dmaCommandBuffer.empty(), LIBRECUDA_ERROR_UNKNOWN);

            // only issue signalNotify if last command didn't already do that
            if (timelineNotifyPending) {
                LIBRECUDA_ERR_PROPAGATE(signalNotify(timelineSignal, timelineCtr, COMPUTE));
            }
            LIBRECUDA_ERR_PROPAGATE(startExecution(COMPUTE));
        }
        if (currentQueueType == DMA) {
            LIBRECUDA_VALIDATE(computeCommandBuffer.empty(), LIBRECUDA_ERROR_UNKNOWN);

            // only issue signalNotify if last command didn't already do that
            if (timelineNotifyPending) {
                LIBRECUDA_ERR_PROPAGATE(signalNotify(timelineSignal, timelineCtr, DMA));
            }
            LIBRECUDA_ERR_PROPAGATE(startExecution(DMA));
        }
        timelineNotifyPending = false;
    }
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t NvCommandQueue::signalWaitGpu(NvSignal *pSignal, NvU32 signalTarget) {
    if (pSignal->value == signalTarget) {
        // No need to wait, if cpu can confirm.
        // if this change is visible, this means it was done on this stream by operations
        // that were already commenced and have already executed on the GPU.
        return LIBRECUDA_SUCCESS;
    }
    LIBRECUDA_ERR_PROPAGATE(enqueue(
            makeNvMethod(0, NVC56F_SEM_ADDR_LO, 5),
            {
                    // little endian
                    U64_LO_32_BITS(pSignal),
                    U64_HI_32_BITS(pSignal),

                    // little endian
                    signalTarget,
                    0,

                    (NVC56F_SEM_EXECUTE_OPERATION_ACQUIRE) |
                    (NVC56F_SEM_EXECUTE_PAYLOAD_SIZE_64BIT << 24)
            },
            COMPUTE
    ));
    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t NvCommandQueue::recordEvent(LibreCUEvent event, LibreCUstream_ *pStream) {
    LIBRECUDA_VALIDATE(event != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);

    event->stream = pStream;
    switch (currentQueueType) {
        case COMPUTE: {
            if (event->computeSignal == nullptr) {
                LIBRECUDA_ERR_PROPAGATE(obtainSignal(&event->computeSignal));
            }
            LIBRECUDA_ERR_PROPAGATE(signalNotify(event->computeSignal, 1, COMPUTE));
            break;
        }
        case DMA: {
            if (event->dmaSignal == nullptr) {
                LIBRECUDA_ERR_PROPAGATE(obtainSignal(&event->dmaSignal));
            }
            LIBRECUDA_ERR_PROPAGATE(signalNotify(event->dmaSignal, 1, DMA));
            break;
        }
        default: {
            // if the queue type isn't determined yet, we just force compute mode.
            if (event->computeSignal == nullptr) {
                LIBRECUDA_ERR_PROPAGATE(obtainSignal(&event->computeSignal));
            }
            LIBRECUDA_ERR_PROPAGATE(signalNotify(event->computeSignal, 1, COMPUTE));
            currentQueueType = COMPUTE;
        }
    }

    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t NvCommandQueue::waitForEvent(LibreCUEvent event) {
    LIBRECUDA_VALIDATE(event != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(event->stream != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(event->computeSignal != nullptr || event->dmaSignal != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);

    if (event->computeSignal != nullptr) {
        signalWaitCpu(event->computeSignal, 1);
    } else if (event->dmaSignal != nullptr) {
        signalWaitCpu(event->dmaSignal, 1);
    }

    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t NvCommandQueue::getEventTimestamp(LibreCUEvent event, uint64_t *pTimestampOut) {
    LIBRECUDA_VALIDATE(event != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(pTimestampOut != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(event->stream != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    LIBRECUDA_VALIDATE(event->computeSignal != nullptr || event->dmaSignal != nullptr, LIBRECUDA_ERROR_INVALID_VALUE);
    if (event->computeSignal != nullptr) {
        LIBRECUDA_VALIDATE(event->computeSignal->value == 1, LIBRECUDA_ERROR_NOT_READY); // event was not hit yet
        *pTimestampOut = event->computeSignal->time_stamp;
    } else if (event->dmaSignal != nullptr) {
        LIBRECUDA_VALIDATE(event->dmaSignal->value == 1, LIBRECUDA_ERROR_NOT_READY); // event was not hit yet
        *pTimestampOut = event->dmaSignal->time_stamp;
    }
    LIBRECUDA_SUCCEED();
}
