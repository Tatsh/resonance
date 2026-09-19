#include "os/async.h"

#include <eekernel.h>
#include <libcdvd.h>
#include <list>
#include <string.h>

#include "os/loadfile.h"
#include "os/log.h"
#include "os/mem.h"
#include "os/seccache.h"

namespace {

// The drive callback thread's stack, which lives in .bss rather than on the
// kernel heap.
constexpr int kAsyncCallbackStackSize = 0x2000;

// The priority sceCdInitEeCB gives the drive callback thread.
constexpr int kAsyncCallbackPriority = 1;

// The priority InitAsync raises the calling thread to.
constexpr int kAsyncCallerPriority = 2;

// Written into every job's mUnknown0c. Thirty-two drive sectors of 2048 bytes
// are one kSectorCacheRowSize chunk.
constexpr int kAsyncJobChunkSectors = 32;

// Function code libcdvd reports for a finished read.
constexpr int kCdFunctionRead = 1;

// Function code libcdvd reports for a finished seek.
constexpr int kCdFunctionSeek = 4;

/**
 * The read the media is servicing right now.
 *
 * The field names come from the dump line, which reads `current op:  id: %d,
 * sector: %d, buffer: %p, status: %d, retry: %d (%d)`. mId is the resolved file
 * rather than a request identifier, which BeginAsyncOp proves by passing it
 * straight to SectorCacheGetLru().
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

// 0x006e9128. The element is the 48-byte request inline, not a pointer to one.
std::list<AsyncRequest> g_asyncPendingJobs;

// 0x006e9130
std::list<AsyncRequest> g_asyncCompletedJobs;

// 0x006e9134
int g_nAsyncNextJobId;

// 0x006e9138
int g_bAsyncInitialised;

// 0x006e913c
int g_nAsyncHostMedia;

// 0x006e9140
AsyncOp g_asyncOp;

// 0x006e915c
int g_bAsyncThreaded;

// 0x006e9160. Raised by the drive callback and consumed by AsyncCheck.
int g_nAsyncOpFinished;

// 0x006e91d0
AsyncJob *g_pAsyncFreeJobs;

// 0x006e91d4
int g_nAsyncCallbackThread;

// 0x006e91d8. Nothing restores it.
sceCdCBFunc g_pfnAsyncPrevCdCallback;

// 0x006e91dc. The drive error code the last callback report latched.
int g_nAsyncOpError;

// 0x008925a0
char g_abAsyncCallbackStack[kAsyncCallbackStackSize];

} // namespace

void InitAsync() {
    g_nAsyncHostMedia = (UsingCdMedia() == 0);
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
        sceCdInitEeCB(kAsyncCallbackPriority, g_abAsyncCallbackStack, kAsyncCallbackStackSize);
        g_nAsyncCallbackThread = GetThreadId();
        // The priority change applies to the calling thread, not to the callback thread.
        ChangeThreadPriority(g_nAsyncCallbackThread, kAsyncCallerPriority);
        g_pfnAsyncPrevCdCallback = sceCdCallback(AsyncMediaEventCallback);
        g_bAsyncThreaded = 1;
    }

    g_bAsyncInitialised = 1;
}

void AsyncQueueRequest(AsyncRequest request) {
    if (g_nAsyncHostMedia != 0) {
        const int nRead = FileRead(request.mFile, request.mReadBuffer, request.mReadLength);
        AsyncJobComplete(&request, (nRead > 0) ? kAsyncStatusOk : kAsyncStatusReadFailed);
        if ((request.mFlags & kAsyncRequestCloseFile) != 0) {
            // AsyncJobComplete has already closed the file for the same flag.
            FileClose(request.mFile);
        }
        return;
    }

    const int nOffset = ((request.mFile & kFileHandleArkStream) != 0) ?
                            GetArkStreamPosition(request.mFile) :
                            FileSeek(request.mFile, 0, kFileSeekCur);

    const int nFirstChunk = nOffset / kSectorCacheRowSize;
    const int nFirstOffset = nOffset - (nFirstChunk * kSectorCacheRowSize);
    // The count is one too many when the transfer ends exactly on a chunk boundary, and the extra
    // pass then transfers nothing.
    const int nChunkCount = static_cast<int>(static_cast<unsigned>(nOffset + request.mReadLength) /
                                             kSectorCacheRowSize) -
                            nFirstChunk + 1;

    char *pDest = static_cast<char *>(request.mReadBuffer);
    int nRemaining = request.mReadLength;
    AsyncJob *pPrev = nullptr;

    for (int i = 0; i < nChunkCount; ++i) {
        int nChunkOffset;
        int nChunkLength;
        if (i == 0) {
            nChunkOffset = nFirstOffset;
            nChunkLength = kSectorCacheRowSize - nFirstOffset;
        } else {
            nChunkOffset = 0;
            nChunkLength = (nRemaining < kSectorCacheRowSize) ? nRemaining : kSectorCacheRowSize;
        }
        if (nRemaining < nChunkLength) {
            nChunkLength = nRemaining;
        }

        const int nChunk = nFirstChunk + i;
        bool bNeedJob = true;
        if (((request.mFile & kFileHandleArkStream) != 0) &&
            (MatchesCurrentAsyncOp(request.mStreamFile, nChunk) == 0)) {
            const SectorCacheRow *pRow = SectorCacheFind(request.mStreamFile, nChunk);
            if (pRow != nullptr) {
                bNeedJob = false;
                memcpy(
                    pDest, static_cast<const char *>(pRow->mBuffer) + nChunkOffset, nChunkLength);
            }
        }

        if (bNeedJob) {
            AsyncJob *pJob = AsyncGetFreeJobChain();
            if (request.mJobs == nullptr) {
                request.mJobs = pJob;
            } else {
                pPrev->mNext = pJob;
            }
            pJob->mPrev = pPrev;
            pPrev = pJob;
            pJob->mNext = nullptr;
            pJob->mSector = nChunk;
            pJob->mSectorOffset = nChunkOffset;
            pJob->mUnknown0c = kAsyncJobChunkSectors;
            pJob->mBuffer = pDest;
            pJob->mLength = nChunkLength;
        }

        nRemaining -= nChunkLength;
        pDest += nChunkLength;
    }

    if (request.mJobs == nullptr) {
        AsyncJobComplete(&request, kAsyncStatusOk);
        return;
    }

    g_asyncPendingJobs.push_back(request);
}

void AsyncMediaEventCallback(int nFunction) {
    g_nAsyncOpError = sceCdGetError();

    switch (nFunction) {
    case kCdFunctionRead:
    case kCdFunctionSeek:
        g_nAsyncOpFinished = 1;
        break;
    }
}

void ShutdownAsync() {
    if (g_bAsyncInitialised != 0) {
        MemFreeTagged(g_pAsyncFreeJobs, __FILE__, __LINE__);
    }
}

int AsyncSubmitRequest(int nFile,
                       void *pBuffer,
                       int nLength,
                       int bCloseOnComplete,
                       AsyncCallback *pCallback,
                       int bOwnsBuffer) {
    if (g_bAsyncInitialised == 0) {
        InitAsync();
    }

    const int nStreamFile = ResolveAsyncStreamFile(nFile);

    AsyncRequest request;
    // mJobs is the one field the clear alone establishes.
    memset(&request, 0, sizeof(request));
    request.mId = g_nAsyncNextJobId;
    request.mFile = nFile;
    request.mBuffer = pBuffer;
    request.mReadBuffer = pBuffer;
    request.mReadLength = nLength;
    request.mLength = nLength;
    request.mStreamFile = nStreamFile;
    request.mFlags = (bCloseOnComplete != 0) ? kAsyncRequestCloseFile : 0;
    request.mCallback = pCallback;
    request.mStatus = kAsyncStatusPending;
    request.mOwnsBuffer = bOwnsBuffer;

    AsyncQueueRequest(request);

    if (((nFile & kFileHandleArkStream) != 0) && (g_nAsyncHostMedia == 0)) {
        SeekArkStream(nFile, nLength, kFileSeekCur);
    }

    ++g_nAsyncNextJobId;
    return request.mId;
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
