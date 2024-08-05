#include "cmdqueue.h"

#include <algorithm>
#include <mutex>
#include <thread>
#include <cstring>

#include "nvidia/clc6c0.h"
#include "nvidia/clc6b5.h"
#include "nvidia/g_allclasses.h"
#include "nvidia/nvos.h"

#include "librecuda_internal.h"

#define ENSURE_QUEUE_INITIALIZED() LIBRECUDA_VALIDATE(initialized, LIBRECUDA_ERROR_NOT_INITIALIZED)


#define MAX_CMD_QUEUE_PAGE_SIZE 0x200000
#define MAX_SIGNAL_POOL_SIZE 65536

static inline NvU32 hi_32(NvU64 value) {
    return static_cast<NvV32>(value >> 32);
}

static inline NvU32 lo_32(NvU64 value) {
    return static_cast<NvV32>(value & 0xFFFFFFFF);
}

#define U64_HI_32_BITS(value) hi_32(reinterpret_cast<NvU64>(value))
#define U64_LO_32_BITS(value) lo_32(reinterpret_cast<NvU64>(value))


static NvMethod makeNvMethod(int subcommand, int method, int size, int typ) {
    return (typ << 28) | (size << 16) | (subcommand << 13) | (method >> 2);
}

NvCommandQueue::NvCommandQueue(LibreCUcontext ctx) : ctx(ctx) {
}

libreCudaStatus_t NvCommandQueue::submitToFifo(GPFifo &gpfifo, CommandQueuePage &page) {
    LIBRECUDA_VALIDATE(page.commandQueueSpace != nullptr, LIBRECUDA_ERROR_NOT_INITIALIZED);

    if (commandBuffer.empty()) {
        return LIBRECUDA_SUCCESS; // don't do anything when command buffer is emtpy
    }

    if ((page.commandWritePtr + (commandBuffer.size() * sizeof(NvU32))) > (MAX_CMD_QUEUE_PAGE_SIZE * sizeof(NvU32))) {
        LIBRECUDA_VALIDATE(NvU64(gpfifo.ring[gpfifo.controls->GPGet] & 0xFFFFFFFFFC) >=
                           NvU64(page.commandQueueSpace + commandBuffer.size()) ||
                           gpfifo.controls->GPGet == gpfifo.controls->GPPut, LIBRECUDA_ERROR_UNKNOWN);
        page.commandWritePtr = 0;
    }
    memcpy(page.commandQueueSpace + page.commandWritePtr, commandBuffer.data(), commandBuffer.size() * sizeof(NvU32));
    page.commandWritePtr += commandBuffer.size();

    gpfifo.ring[gpfifo.put_value % gpfifo.entries_count] =
            ((NvU64(page.commandQueueSpace) / sizeof(NvU32)) << 2) | (commandBuffer.size() << 42) | (1l << 41);
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
        NvU64 local_mem_window = 0xff000000;
        NvU64 shared_mem_window = 0xfe000000;

        LIBRECUDA_ERR_PROPAGATE(
                enqueue(makeNvMethod(1, NVC6C0_SET_OBJECT, 1), {ctx->device->compute_class})
        );
        LIBRECUDA_ERR_PROPAGATE(
                enqueue(
                        makeNvMethod(1, NVC6C0_SET_SHADER_LOCAL_MEMORY_WINDOW_A, 2),
                        {U64_HI_32_BITS(local_mem_window), U64_LO_32_BITS(local_mem_window)}
                )
        );
        LIBRECUDA_ERR_PROPAGATE(
                enqueue(
                        makeNvMethod(1, NVC6C0_SET_SHADER_SHARED_MEMORY_WINDOW_A, 2),
                        {U64_HI_32_BITS(shared_mem_window), U64_LO_32_BITS(shared_mem_window)}
                )
        );
        timelineCtr++;
    }
    LIBRECUDA_ERR_PROPAGATE(startExecution(COMPUTE));
    LIBRECUDA_ERR_PROPAGATE(awaitExecution());

    // setup copy queue
    {
        LIBRECUDA_ERR_PROPAGATE(
                enqueue(makeNvMethod(4, NVC6C0_SET_OBJECT, 1), {AMPERE_DMA_COPY_B})
        );
        timelineCtr++;
    }

    LIBRECUDA_ERR_PROPAGATE(startExecution(DMA));
    LIBRECUDA_ERR_PROPAGATE(awaitExecution());

    initialized = true;

    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t NvCommandQueue::enqueue(NvMethod method, std::initializer_list<NvU32> arguments) {
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
    if (signalIdx > MAX_SIGNAL_POOL_SIZE) {
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
}

libreCudaStatus_t NvCommandQueue::awaitExecution() {
    LIBRECUDA_VALIDATE(timelineSignal != nullptr, LIBRECUDA_ERROR_NOT_INITIALIZED);
    LIBRECUDA_ERR_PROPAGATE(signalWait(timelineSignal, timelineCtr));
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

                            (NVC56F_SEM_EXECUTE_OPERATION_RELEASE) | (NVC56F_SEMAPHORED_RELEASE_WFI_EN << 20) |
                            (NVC56F_SEM_EXECUTE_PAYLOAD_SIZE_64BIT << 24) |
                            (NVC56F_SEM_EXECUTE_RELEASE_TIMESTAMP_EN
                                    << 25)
                    }
            ));
            LIBRECUDA_ERR_PROPAGATE(
                    enqueue(
                            makeNvMethod(4, NVC56F_NON_STALL_INTERRUPT, 1),
                            {0x0}
                    )
            );
            break;
        }
        case DMA: {
            LIBRECUDA_ERR_PROPAGATE(enqueue(
                    makeNvMethod(4, NVC6B5_SET_SEMAPHORE_A, 4),
                    {
                            // little endian inside NvU32s but big endian across NvU32s for some reason...
                            // don't question nvidia's autism...
                            U64_HI_32_BITS(pSignal),
                            U64_LO_32_BITS(pSignal),

                            signalTarget,

                            // 4 means what?
                            4
                    }
            ));
            LIBRECUDA_ERR_PROPAGATE(enqueue(
                    makeNvMethod(4, NVC6B5_LAUNCH_DMA, 1),
                    {0x14} // also don't know what this means
            ))
            break;
        }
    }

    LIBRECUDA_SUCCEED();
}

libreCudaStatus_t NvCommandQueue::signalWait(NvSignal *pSignal, NvU32 signalTarget) {
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
    LIBRECUDA_ERR_PROPAGATE(signalNotify(timelineSignal, timelineCtr, queueType));
    LIBRECUDA_ERR_PROPAGATE(submitToFifo(
            queueType == COMPUTE ? ctx->compute_gpfifo : ctx->dma_gpfifo,
            queueType == COMPUTE ? computeQueuePage : dmaQueuePage
    ));
    LIBRECUDA_SUCCEED();
}

