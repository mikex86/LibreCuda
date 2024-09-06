#ifndef LIBRECUDA_CMDQUEUE_H
#define LIBRECUDA_CMDQUEUE_H

#include <initializer_list>
#include <cstddef>
#include <vector>
#include <deque>
#include <cstdint>
#include <mutex>
#include <condition_variable>

#include "librecuda.h"
#include "librecuda_internal.h"

#include "nvidia/nvtypes.h"

typedef NvU64 NvMethod;

static inline NvMethod makeNvMethod(int subcommand, int method, int size, int typ = 2);

struct NvSignal {
public:
    volatile NvU64 value{};
    volatile NvU64 time_stamp{};
};

enum QueueType {
    COMPUTE, DMA
};

struct CommandQueuePage {

    /**
     * CPU mapped GPU memory for the command queue.
     * Command queue is copied to this page for submission.
     */
    NvU32 *commandQueueSpace{};

    /**
     * Pointer into commandQueuePage. Commands are mem-copied in appending fashion into the command queue page and
     * commandWritePtr is used to keep track of the current offset from the base pointer.
     */
    uint64_t commandWriteIdx = 0;

};

/**
 * Represents a region of the command buffer attributable to a particular QueueType in sequential time space.
 * These splits are necessary to interleave COMPUTE and DMA calls while preserving sequentiality
 */
struct CommandBufSplit {
public:
    std::vector<NvU32> commandBuffer{};
    QueueType queueType;
    NvU32 timelineCtr;
    bool timelineNotifyPending;
};

class NvCommandQueue {
private:
    /**
     * The parent cuda context
     */
    LibreCUcontext ctx;

    /**
     * State whether the queue has been initialized.
     * @code initializeQueue() @endcode must be called before queue methods can be called
     */
    bool initialized = false;

    /**
     * Growing list of bytes that represent the commands enqueued in the queue.
     * Once built, the command buffers contents will be copied over to the memory mapped command queue page.
     *
     * Format:
     * - NvMethod (NvU32) nvMethod
     * - vararg NvU32 expected by method (tightly packed arguments) ; size is encoded in the NvMethod bit structure
     */
    std::vector<NvU32> computeCommandBuffer{}, dmaCommandBuffer{};


    /**
     * compute queue page & stack ptr
     */
    CommandQueuePage computeQueuePage{};

    /**
     * DMA (copy) queue page & stack ptr
     */
    CommandQueuePage dmaQueuePage{};

    /**
     * CPU mapped GPU buffer of NvSignals, which can be claimed and freed again
     */
    NvSignal *signalPool{};

    /**
     * Vector of free signal handles (index into signalPool)
     */
    std::deque<NvU32> freeSignals{};

    /**
     * Primary signal used for synchronization
     */
    NvSignal *timelineSignal{};

    /**
     * Incrementing counter used for synchronization.
     * Synchronization employs signals whose values have to be greater or equal to some set value.
     * As long as the condition is false, the queue will wait.
     * Thus we have a counter value to derive new "waiting targets".
     *
     * The intuition here is that the timelineCtr advances first, as the queue is built, and the signal's value advances
     * to meet the timelineCtr. If they are equal, there is no async operation pending.
     */
    NvU32 timelineCtr = 0;

    /**
     * State whether the last command that incremented the timeline also issued a signalNotify() command.
     * If this flag is true and the stream is commenced, a trailing signalNotify() has to be inserted.
     * Otherwise this is not necessary as the last COMPUTE/DMA command already issued it.
     */
    bool timelineNotifyPending = false;

    // TODO: To my knowledge there is no way to interleave COMPUTE and DMA queues with synchronization primitives.
    //  You can release semaphores on a DMA queue, but not acquire it. You need both for bi-directional sync.
    /**
     * The current queue mode.
     * When a kernel is launched, the queue mode is switched over to COMPUTE.
     * When eg. a memcpy is performed, the queue mode is switched to DMA.
     * Whenever you switch between COMPUTE and DMA, we have to split into multiple command queues that we wait for to complete on the cpu.
     * It is an unfortunate necessity. This is why interleaving DMA and compute operations should be avoided with librecuda in
     * highly performance critical applications. (it won't be the end of the world, but its not nice...)
     * (I also highly doubt real cuda does this any better)
     */
    QueueType currentQueueType;

    /**
     * Backlog of command buffer splits for interleaving queue types
     */
    std::vector<CommandBufSplit> commandBufBacklog{};

    /**
     * Current shader local memory per thread. NOT equal to the allocated size of localMemoryVa, but correlates.
     */
    size_t currentSlmPerThread = 0;

    /**
     * GPU virtual address pointing to a page of memory used for storing the arguments to launched kernels
     */
    NvU8 *kernArgsPageVa;

    /**
     * Byte offset into kernArgsPageVa to bump allocate memory inside the page.
     */
    size_t kernArgsWriteIdx{};
private:
    NvU64 local_mem_window = 0xff000000;
    NvU64 shared_mem_window = 0xfe000000;

public:
    explicit NvCommandQueue(LibreCUcontext ctx);

    /**
     * Must be called before the queue is usable
     * @return status
     */
    libreCudaStatus_t initializeQueue();

    libreCudaStatus_t startExecution(QueueType queueType);

    libreCudaStatus_t startExecution();

    /**
     * Waits for the pending operations in the currently executing command queue to complete
     */
    libreCudaStatus_t awaitExecution();

    ~NvCommandQueue();

    libreCudaStatus_t ensureEnoughLocalMem(LibreCUFunction function);

    libreCudaStatus_t
    launchFunction(LibreCUFunction function,
                   uint32_t gridDimX, uint32_t gridDimY, uint32_t gridDimZ,
                   uint32_t blockDimX, uint32_t blockDimY, uint32_t blockDimZ,
                   uint32_t sharedMemBytes,
                   void **params, size_t numParams, bool async);

    libreCudaStatus_t gpuMemcpy(void *dst, void *src, size_t numBytes, bool async);

    libreCudaStatus_t recordEvent(LibreCUEvent event, LibreCUstream_ *pStream);

    libreCudaStatus_t waitForEvent(LibreCUEvent event);

    libreCudaStatus_t getEventTimestamp(LibreCUEvent event, uint64_t *pTimestampOut);

private:

    libreCudaStatus_t enqueue(NvMethod method, std::initializer_list<NvU32> arguments, QueueType type);

    libreCudaStatus_t obtainSignal(NvSignal **pSignalPtr);

    libreCudaStatus_t releaseSignal(NvSignal *signal);

    libreCudaStatus_t signalNotify(NvSignal *pSignal, NvU32 signalTarget, QueueType type);

    libreCudaStatus_t signalWaitCpu(NvSignal *pSignal, NvU32 signalTarget);

    /**
     * waits on the gpu compute queue until the signal reaches the specified target
     * @param pSignal the signal
     * @param signalTarget the target value
     * @return status
     */
    libreCudaStatus_t signalWaitGpu(NvSignal *pSignal, NvU32 signalTarget);

    libreCudaStatus_t submitToFifo(QueueType type);

    libreCudaStatus_t allocKernArgs(NvU64 *pMemOut, size_t size);

    libreCudaStatus_t backlogCurrentCmdBuffer(QueueType queueType);
};

#endif //LIBRECUDA_CMDQUEUE_H
