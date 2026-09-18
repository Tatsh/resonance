#include "os/async.h"

#include <list>

#include "os/asyncthread.h"
#include "os/loadfile.h"
#include "os/log.h"
#include "os/mem.h"

namespace {

// The worker thread's stack, which lives in .bss rather than on the kernel
// heap.
constexpr int kAsyncWorkerStackSize = 0x2000;

// The priority the worker semaphore runs at.
constexpr int kAsyncWorkerPriority = 2;

/**
 * The read the media is servicing right now.
 *
 * The field names come from the dump line, which reads `current op:  id: %d,
 * sector: %d, buffer: %p, status: %d, retry: %d (%d)`.
 */
struct AsyncOp {
    int mId;        // +0x00
    int mSector;    // +0x04
    int mUnknown08; // +0x08 cleared to -1 alongside mId and mSector
    void *mBuffer;  // +0x0c
    int mStatus;    // +0x10
    int mRetry;     // +0x14
    int mUnknown18; // +0x18 the second count the dump prints after the retry
                    // count
};

// 0x006e9128
std::list<AsyncJob *> g_asyncPendingJobs;

// 0x006e9130
std::list<AsyncJob *> g_asyncCompletedJobs;

// 0x006e9134
int g_nAsyncNextJobId;

// 0x006e9138
int g_bAsyncInitialised;

// 0x006e913c
int g_bAsyncHostMedia;

// 0x006e9140
AsyncOp g_asyncOp;

// 0x006e915c
int g_bAsyncThreaded;

// 0x006e91d0
AsyncJob *g_pAsyncFreeJobs;

// 0x006e91d4
int g_nAsyncSemaphore;

// 0x006e91d8
int g_nAsyncWorkerThread;

// 0x008925a0
char g_abAsyncWorkerStack[kAsyncWorkerStackSize];

} // namespace

void InitAsync() {
    g_bAsyncHostMedia = (UsingCdMedia() == 0);
    g_asyncPendingJobs.clear();
    g_asyncCompletedJobs.clear();
    g_nAsyncNextJobId = 1;

    g_asyncOp.mId = -1;
    g_asyncOp.mUnknown08 = -1;
    g_asyncOp.mUnknown18 = 0;
    g_asyncOp.mSector = -1;
    g_asyncOp.mBuffer = nullptr;
    g_asyncOp.mStatus = 0;
    g_asyncOp.mRetry = 0;

    g_pAsyncFreeJobs = static_cast<AsyncJob *>(
        MemAllocTagged(kAsyncJobCount * sizeof(AsyncJob), __FILE__, __LINE__));

    AsyncJob *pJob = g_pAsyncFreeJobs;
    for (int i = 0; i < kAsyncJobCount; ++i) {
        pJob->mPrev = (i != 0) ? pJob - 1 : nullptr;
        pJob->mNext = (i != kAsyncJobCount - 1) ? pJob + 1 : nullptr;
        ++pJob;
    }

    if (UsingCdMedia() != 0) {
        RegisterWorkerStack(1, g_abAsyncWorkerStack, kAsyncWorkerStackSize);
        g_nAsyncSemaphore = CreateWorkerSemaphore();
        SetWorkerSemaphorePriority(g_nAsyncSemaphore, kAsyncWorkerPriority);
        g_nAsyncWorkerThread = StartWorkerThread(AsyncWorkerMain);
        g_bAsyncThreaded = 1;
    }

    g_bAsyncInitialised = 1;
}

void ShutdownAsync() {
    if (g_bAsyncInitialised != 0) {
        MemFreeTagged(g_pAsyncFreeJobs, __FILE__, __LINE__);
    }
}

AsyncJob *AsyncGetFreeJobChain() {
    AsyncJob *pJob = g_pAsyncFreeJobs;
    AsyncJob *pNext = pJob->mNext;
    // The head is advanced before the check, so an exhausted list leaves a null
    // head behind.
    g_pAsyncFreeJobs = pNext;
    if (pNext == nullptr || pJob == nullptr) {
        Fatal("ASYNC_GET_FREE_JOB_CHAIN FAILURE!\n");
    }
    return pJob;
}

void AsyncReleaseJobChain(AsyncJob *pChain) {
    if (pChain == nullptr) {
        return;
    }

    AsyncJob *pTail = pChain;
    while (pTail->mNext != nullptr) {
        pTail = pTail->mNext;
    }
    pTail->mNext = g_pAsyncFreeJobs;
    g_pAsyncFreeJobs = pChain;
}
