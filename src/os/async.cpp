#include "os/async.h"

#include <eekernel.h>
#include <libcdvd.h>
#include <list>
#include <sifdev.h>
#include <string.h>

#include "os/arkfile.h"
#include "os/asynccallback.h"
#include "os/cycles.h"
#include "os/hostmode.h"
#include "os/loadfile.h"
#include "os/log.h"
#include "os/mem.h"
#include "os/seccache.h"
#include "os/zone.h"

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

// States the one chunk transfer the drive performs moves through. Idle means the record describes
// no transfer at all.
enum AsyncOpStatus { kAsyncOpIdle = 0, kAsyncOpSeeking = 1, kAsyncOpReading = 2, kAsyncOpDone = 3 };

// Milliseconds an issued command is given before AsyncCheck starts consulting the drive itself.
constexpr int kAsyncOpTimeoutMs = 3000;

// Milliseconds an issued command may take before the wait is reported.
constexpr int kAsyncOpWarnMs = 10000;

// How long the wait report asks to stay on screen. The unit is not determined.
constexpr int kAsyncWarnMessageDuration = 300;

// Attempts one chunk is given before the failure is fatal.
constexpr int kAsyncOpMaxAttempts = 3;

// Reported by PickNextAsyncFetch while it has found nothing to fetch.
constexpr int kAsyncNoSectorPending = 9999999;

// Recorded as the file of a request whose path could not be opened. It is not a valid handle. The
// record's flags stay clear, and nothing ever tries to close it.
constexpr int kAsyncNoFile = 9999;

// Bytes of the inflated size a gzip member ends with.
constexpr int kGzInflatedSizeFieldLength = 4;

/**
 * The one chunk transfer the media is performing right now.
 *
 * Every field name is attested. The dump line reads `current op:  id: %d,
 * sector: %d, buffer: %p, status: %d, retry: %d (%d)`, and the fatal report in
 * TakeFinishedAsyncOp reads `FAILED TRYING TO READ SECTOR: %d (id: %d,
 * baseSector: %d)`. Only the second count the dump prints has no title of its
 * own, and it is the attempt counter the fatal report is raised from.
 *
 * mId is the resolved file rather than a request identifier, which
 * AsyncQueueCachedSector proves by passing it straight to SectorCacheGetLru().
 */
struct AsyncOp {
    int mId;         // +0x00
    int mSector;     // +0x04
    int mBaseSector; // +0x08 the archive's own start sector on the disc
    void *mBuffer;   // +0x0c the cache row the chunk is read into
    int mStatus;     // +0x10 one of AsyncOpStatus
    int mRetry;      // +0x14 set when the command has to be issued again
    int mRetryCount; // +0x18
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
AsyncOp g_asyncCurrentOp;

// 0x006e915c
int g_bAsyncThreaded;

// 0x006e9160. Raised by the drive callback and consumed by AsyncCheck.
int g_nAsyncOpFinished;

// 0x006e9168. Nothing in the image writes this, so every field stays at its zero: no retry limit,
// no spindle override, and 2048-byte sectors.
sceCdRMode g_asyncOpReadMode;

// 0x006e9170. Cleared once the read finishes, which is what makes a zero here mean "no command in
// flight" to AsyncCheck.
long long g_llAsyncOpDeadline;

// 0x006e9178
long long g_llAsyncOpStartTime;

// 0x006e91d0
AsyncJob *g_pAsyncFreeJobs;

// 0x006e91d4
int g_nAsyncCallbackThread;

// 0x006e91d8. Nothing restores it.
sceCdCBFunc g_pfnAsyncPrevCdCallback;

// 0x006e91dc. The drive error code the last callback report latched.
int g_nAsyncOpError;

// 0x00892590. The absolute disc sector the seek moves to and the read then starts at.
int g_nAsyncOpLsn;

// 0x008925a0
char g_abAsyncCallbackStack[kAsyncCallbackStackSize];

} // namespace

void InitAsync() {
    g_nAsyncHostMedia = (UsingCdMedia() == 0);
    g_asyncPendingJobs.clear();
    g_asyncCompletedJobs.clear();
    g_nAsyncNextJobId = 1;

    g_asyncCurrentOp.mId = -1;
    g_asyncCurrentOp.mBaseSector = -1;
    g_asyncCurrentOp.mRetryCount = 0;
    g_asyncCurrentOp.mSector = -1;
    g_asyncCurrentOp.mBuffer = nullptr;
    g_asyncCurrentOp.mStatus = kAsyncOpIdle;
    g_asyncCurrentOp.mRetry = 0;

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

// 0x00460b20
int IsMediaReady() {
    return 1;
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

namespace {

// 0x004604c8
// Issue the command the current transfer's state calls for.
//
// The name is attested by the routine's own report. A state the routine does not recognise is
// reported and nothing is issued, and the timing stamps are then not written either.
void AsyncIssueOp() {
    switch (g_asyncCurrentOp.mStatus) {
    case kAsyncOpSeeking:
        g_nAsyncOpLsn =
            g_asyncCurrentOp.mBaseSector +
            ArkfileLogicalToPhysicalSector(g_asyncCurrentOp.mId, g_asyncCurrentOp.mSector) *
                kAsyncJobChunkSectors;
        sceCdSeek(g_nAsyncOpLsn);
        break;
    case kAsyncOpReading:
        // Both command results are discarded. A command the drive refuses outright is therefore
        // noticed only once the deadline below expires.
        sceCdRead(
            g_nAsyncOpLsn, kAsyncJobChunkSectors, g_asyncCurrentOp.mBuffer, &g_asyncOpReadMode);
        break;
    default:
        LogPrintf("AsyncIssueOp: unexpected op status: %d\n", g_asyncCurrentOp.mStatus);
        return;
    }

    g_llAsyncOpStartTime = GetElapsedMilliseconds();
    g_llAsyncOpDeadline = g_llAsyncOpStartTime + kAsyncOpTimeoutMs;
}

} // namespace

void AsyncCheck(int nBlocking) {
    if ((g_asyncCurrentOp.mStatus != kAsyncOpSeeking) &&
        (g_asyncCurrentOp.mStatus != kAsyncOpReading)) {
        return;
    }

    while (true) {
        if (g_llAsyncOpDeadline != 0) {
            const long long llNow = GetElapsedMilliseconds();
            if (llNow >= g_llAsyncOpDeadline) {
                if (sceCdDiskReady(SCECdNonblock) == SCECdNotReady) {
                    sceCdDiskReady(SCECdBlock);
                    g_asyncCurrentOp.mRetry = 1;
                } else if ((llNow - g_llAsyncOpStartTime) > kAsyncOpWarnMs) {
                    ShowScreenMessage("HEY - 10 SECONDS SINCE ASYNC OP\n",
                                      kAsyncWarnMessageDuration);
                }
            }
        }

        int nBusy = 1;
        int nError = 0;
        if (g_bAsyncThreaded != 0) {
            if (g_nAsyncOpFinished != 0) {
                nBusy = 0;
                nError = g_nAsyncOpError;
                g_nAsyncOpFinished = 0;
            } else if (nBlocking != 0) {
                continue;
            }
        } else {
            nBusy = sceCdSync((nBlocking != 0) ? SCECdBlock : SCECdNonblock);
            nError = sceCdGetError();
        }

        if (nError != 0) {
            if (nError != SCECdErTRMOPN) {
                Fatal("CD ERROR: %d on sector %d, NOT retrying...\n",
                      nError,
                      g_asyncCurrentOp.mSector);
            }
            sceCdDiskReady(SCECdBlock);
            g_asyncCurrentOp.mRetry = 1;
            if (nBlocking == 0) {
                return;
            }
            continue;
        }

        if (nBusy != 0) {
            return;
        }

        switch (g_asyncCurrentOp.mStatus) {
        case kAsyncOpSeeking:
            g_asyncCurrentOp.mStatus = kAsyncOpReading;
            AsyncIssueOp();
            if (nBlocking == 0) {
                return;
            }
            continue;
        case kAsyncOpReading:
            g_asyncCurrentOp.mStatus = kAsyncOpDone;
            g_llAsyncOpDeadline = 0;
            return;
        default:
            // A state AsyncIssueOp only reports is fatal here. The entry test above admits no
            // other state, so the branch is unreachable from the one call path.
            Fatal("AsyncCheck: unexpected op status: %d\n", g_asyncCurrentOp.mStatus);
        }
    }
}

namespace {

// 0x004603d8
// Advance the current transfer and take its results once it has finished.
//
// Reports non-zero only on the pass that finds the data in place, and the record is idle again
// afterwards. A pending retry is counted and the command reissued instead.
int TakeFinishedAsyncOp(int *pnFile, int *pnSector, void **ppBuffer) {
    if (g_asyncCurrentOp.mStatus == kAsyncOpIdle) {
        return 0;
    }

    if (g_asyncCurrentOp.mRetry != 0) {
        ++g_asyncCurrentOp.mRetryCount;
        if (g_asyncCurrentOp.mRetryCount == kAsyncOpMaxAttempts) {
            Fatal("FAILED TRYING TO READ SECTOR: %d (id: %d, baseSector: %d)\n",
                  g_asyncCurrentOp.mSector,
                  g_asyncCurrentOp.mId,
                  g_asyncCurrentOp.mBaseSector);
        }
        g_asyncCurrentOp.mRetry = 0;
        AsyncIssueOp();
        return 0;
    }

    AsyncCheck(0);
    if (g_asyncCurrentOp.mStatus != kAsyncOpDone) {
        return 0;
    }

    *pnFile = g_asyncCurrentOp.mId;
    *pnSector = g_asyncCurrentOp.mSector;
    *ppBuffer = g_asyncCurrentOp.mBuffer;

    g_asyncCurrentOp.mId = -1;
    g_asyncCurrentOp.mSector = -1;
    g_asyncCurrentOp.mRetryCount = 0;
    g_asyncCurrentOp.mBuffer = nullptr;
    g_asyncCurrentOp.mStatus = kAsyncOpIdle;
    // mBaseSector is not reset here. AsyncQueueCachedSector always rewrites it.
    g_asyncCurrentOp.mRetry = 0;
    return 1;
}

// Take one finished job out of its request's chain and put it back on the free list.
inline void UnlinkAsyncJob(AsyncRequest *pRequest, AsyncJob *pJob) {
    if (pJob->mNext != nullptr) {
        pJob->mNext->mPrev = pJob->mPrev;
    }
    if (pJob->mPrev != nullptr) {
        pJob->mPrev->mNext = pJob->mNext;
    } else {
        pRequest->mJobs = pJob->mNext;
    }

    pJob->mPrev = nullptr;
    pJob->mNext = nullptr;
    if (pJob != nullptr) { // The test cannot fire from either caller. The binary performs it.
        pJob->mNext = g_pAsyncFreeJobs;
        g_pAsyncFreeJobs = pJob;
    }
}

// 0x00460e10
// Perform one job at once rather than through the sector cache.
//
// This is the path a request on a loose file takes. The cache is keyed by 64 KiB chunks of an
// archive, and a loose file has no archive to key it by. The job therefore reads straight from the
// file through the SDK primitives.
void DeliverAsyncJobData(AsyncRequest *pRequest, AsyncJob *pJob) {
    const int nFile = ((pRequest->mFile & kFileHandleArkStream) != 0) ?
                          GetArkStreamArkId(pRequest->mFile & ~kFileHandleArkStream) :
                          pRequest->mFile;

    sceLseek(nFile, pJob->mSector * kSectorCacheRowSize + pJob->mSectorOffset, SCE_SEEK_SET);
    sceRead(nFile, pJob->mBuffer, pJob->mLength);
    UnlinkAsyncJob(pRequest, pJob);
}

// 0x00460120
// Report the work the drive should do next.
//
// A pending request on a loose file is serviced in place and ends the scan. The first pending
// ark-stream request is copied out instead, and the chunk its first job wants is reported for the
// caller to queue.
int PickNextAsyncFetch(AsyncRequest *pRequest) {
    int nSector = kAsyncNoSectorPending;
    for (auto it = g_asyncPendingJobs.begin(); it != g_asyncPendingJobs.end(); ++it) {
        if ((it->mFile & kFileHandleArkStream) == 0) {
            DeliverAsyncJobData(&*it, it->mJobs);
            break;
        }
        if (nSector == kAsyncNoSectorPending) {
            *pRequest = *it;
            nSector = it->mJobs->mSector;
        }
    }

    return (nSector != kAsyncNoSectorPending) ? nSector : -1;
}

// 0x00460238
// Hand a freshly read chunk to every pending request that wants it.
//
// A request whose last job is satisfied here completes immediately. The data a caller asked for is
// therefore in place before the caller is told about it.
void DistributeAsyncSectorData(int nFile, int nSector, const void *pSectorData) {
    for (auto it = g_asyncPendingJobs.begin(); it != g_asyncPendingJobs.end();) {
        if (it->mStreamFile != nFile) {
            ++it;
            continue;
        }

        AsyncJob *pJob = it->mJobs;
        while (pJob != nullptr) {
            AsyncJob *pNext = pJob->mNext;
            if (pJob->mSector == nSector) {
                memcpy(pJob->mBuffer,
                       static_cast<const char *>(pSectorData) + pJob->mSectorOffset,
                       pJob->mLength);
                UnlinkAsyncJob(&*it, pJob);
            }
            pJob = pNext;
        }

        if (it->mJobs != nullptr) {
            ++it;
            continue;
        }

        AsyncJobComplete(&*it, kAsyncStatusOk);
        it = g_asyncPendingJobs.erase(it);
    }
}

// 0x00460ee0
// Start the transfer of one chunk into the cache.
//
// The name is attested by the routine's own report. The row the transfer will fill is locked for
// the whole of it, which is what stops AsyncQueueRequest() copying a half-filled row out. The
// result is the constant 1 and the one caller discards it.
int AsyncQueueCachedSector(int nFile, int nSector, int nBaseSector) {
    SectorCacheRow *pRow = SectorCacheGetLru(nFile, nSector);
    if (pRow == nullptr) {
        Fatal("AsyncQueueCachedSector: internal error!!\n");
    }
    SetSectorRowLocked(pRow);

    g_asyncCurrentOp.mId = nFile;
    g_asyncCurrentOp.mSector = nSector;
    g_asyncCurrentOp.mBaseSector = nBaseSector;
    g_asyncCurrentOp.mStatus = kAsyncOpSeeking;
    g_asyncCurrentOp.mBuffer = pRow->mBuffer;
    g_asyncCurrentOp.mRetryCount = 0;
    g_asyncCurrentOp.mRetry = 0;
    AsyncIssueOp();
    return 1;
}

} // namespace

void AsyncPumpCompletedRequests() {
    if (g_nAsyncHostMedia == 0) {
        int nFile;
        int nSector;
        void *pSectorData;
        if (TakeFinishedAsyncOp(&nFile, &nSector, &pSectorData) != 0) {
            DistributeAsyncSectorData(nFile, nSector, pSectorData);
            UnlockCachedSector(nFile, nSector);
        }

        if (g_asyncCurrentOp.mStatus == kAsyncOpIdle) {
            AsyncRequest request;
            const int nFetchSector = PickNextAsyncFetch(&request);
            if (nFetchSector >= 0) {
                AsyncQueueCachedSector(
                    request.mStreamFile, nFetchSector, ArkfileGetBaseSector(request.mStreamFile));
            }
        }
    }

    for (auto it = g_asyncCompletedJobs.begin(); it != g_asyncCompletedJobs.end();) {
        if (it->mCallback != nullptr) {
            it->mCallback->Done(it->mId, it->mFile, it->mBuffer, it->mLength, it->mStatus);
        }
        if (it->mJobs != nullptr) {
            AsyncReleaseJobChain(it->mJobs);
        }
        it = g_asyncCompletedJobs.erase(it);
    }
}

int ResolveAsyncStreamFile(int nFile) {
    if ((nFile & kFileHandleArkStream) == 0) {
        return nFile;
    }

    return GetArkStreamArkId(nFile & ~kFileHandleArkStream);
}

int MatchesCurrentAsyncOp(int nFile, int nSector) {
    if (g_asyncCurrentOp.mId != nFile) {
        return 0;
    }

    return (g_asyncCurrentOp.mSector == nSector) ? 1 : 0;
}

void AsyncJobComplete(AsyncRequest *pRequest, int nStatus) {
    if (nStatus > 0) {
        LogPrintf("AsyncJobComplete: job %d has error: %d\n", pRequest->mId, nStatus);
    } else if ((pRequest->mFlags & kAsyncRequestInflate) != 0) {
        if (InflateGzBuffer(pRequest->mReadBuffer, pRequest->mReadLength, pRequest->mBuffer) <= 0) {
            nStatus = kAsyncStatusInflateFailed;
        }
    }

    if ((pRequest->mFlags & kAsyncRequestCloseFile) != 0) {
        FileClose(pRequest->mFile);
    }
    pRequest->mStatus = nStatus;
    g_asyncCompletedJobs.push_back(*pRequest);
}

int AsyncPollComplete(int nHandle, void **ppBuffer, int *pnLength) {
    for (auto it = g_asyncCompletedJobs.begin(); it != g_asyncCompletedJobs.end(); ++it) {
        if (it->mId != nHandle) {
            continue;
        }

        const int nStatus = it->mStatus;
        if (ppBuffer != nullptr) {
            *ppBuffer = it->mBuffer;
        }
        if (pnLength != nullptr) {
            *pnLength = it->mLength;
        }
        if (it->mJobs != nullptr) {
            AsyncReleaseJobChain(it->mJobs);
        }
        g_asyncCompletedJobs.erase(it);
        return nStatus;
    }

    return -1;
}

void AsyncCancelRequest(int nHandle) {
    for (auto it = g_asyncPendingJobs.begin(); it != g_asyncPendingJobs.end(); ++it) {
        if (it->mId != nHandle) {
            continue;
        }

        if (it->mOwnsBuffer != 0) {
            MemFreeTagged(it->mBuffer, __FILE__, __LINE__);
        }
        if ((it->mFlags & kAsyncRequestCloseFile) != 0) {
            FileClose(it->mFile);
        }
        if (it->mJobs != nullptr) {
            AsyncReleaseJobChain(it->mJobs);
        }
        g_asyncPendingJobs.erase(it);
        return;
    }

    for (auto it = g_asyncCompletedJobs.begin(); it != g_asyncCompletedJobs.end(); ++it) {
        if (it->mId != nHandle) {
            continue;
        }

        if (it->mOwnsBuffer != 0) {
            MemFreeTagged(it->mBuffer, __FILE__, __LINE__);
        }
        // A finished request's file is not closed here, unlike the pending case above. Nothing
        // reopens it either.
        if (it->mJobs != nullptr) {
            AsyncReleaseJobChain(it->mJobs);
        }
        g_asyncCompletedJobs.erase(it);
        return;
    }
}

void AsyncDump() {
    LogPrintf("\nASYNC DUMP\n\n");
    LogPrintf("current op:  id: %d, sector: %d, buffer: %p, status: %d, retry: %d (%d)\n",
              g_asyncCurrentOp.mId,
              g_asyncCurrentOp.mSector,
              g_asyncCurrentOp.mBuffer,
              g_asyncCurrentOp.mStatus,
              g_asyncCurrentOp.mRetry,
              g_asyncCurrentOp.mRetryCount);

    LogPrintf("num Pending Jobs: %d\n", static_cast<int>(g_asyncPendingJobs.size()));
    for (auto it = g_asyncPendingJobs.begin(); it != g_asyncPendingJobs.end(); ++it) {
        LogPrintf("   h: %d\n", it->mId);
    }

    LogPrintf("num Completed Jobs: %d\n", static_cast<int>(g_asyncCompletedJobs.size()));
    for (auto it = g_asyncCompletedJobs.begin(); it != g_asyncCompletedJobs.end(); ++it) {
        LogPrintf("   h: %d\n", it->mId);
    }

    int nFreeJobs = 0;
    for (const AsyncJob *pJob = g_pAsyncFreeJobs; pJob != nullptr; pJob = pJob->mNext) {
        ++nFreeJobs;
    }
    LogPrintf("num Free Job Chains: %d\n", nFreeJobs);
}

int AsyncLoadFileByPath(const char *pszPath,
                        void *pBuffer,
                        unsigned nLength,
                        AsyncCallback *pCallback) {
    if (g_bAsyncInitialised == 0) {
        InitAsync();
    }

    const char *pszExtension = strrchr(pszPath, '.');
    const int bGzipped = ((pszExtension != nullptr) && (pszExtension[1] == 'g') &&
                          (pszExtension[2] == 'z') && (pszExtension[3] == '\0')) ?
                             1 :
                             0;

    const int nFile = FileOpen(pszPath, 0);
    if (nFile < 0) {
        AsyncRequest request;
        memset(&request, 0, sizeof(request));
        request.mId = g_nAsyncNextJobId;
        request.mFile = kAsyncNoFile;
        request.mCallback = pCallback;
        request.mStatus = kAsyncStatusPending;
        AsyncJobComplete(&request, kAsyncStatusOpenFailed);
        ++g_nAsyncNextJobId;
        return request.mId;
    }

    const int bArkStream = (nFile & kFileHandleArkStream);
    int nStoredLength;
    int nInflatedLength;
    if (bArkStream != 0) {
        const ArkFileEntry *pEntry = GetArkStreamFileEntry(nFile & ~kFileHandleArkStream);
        nStoredLength = pEntry->mLength;
        nInflatedLength = pEntry->mSize;
    } else {
        nStoredLength = FileSeek(nFile, 0, kFileSeekEnd);
        if (bGzipped != 0) {
            // The inflated size is the last four bytes of a gzip member.
            FileSeek(nFile, -kGzInflatedSizeFieldLength, kFileSeekEnd);
            FileRead(nFile, &nInflatedLength, kGzInflatedSizeFieldLength);
        } else {
            nInflatedLength = nStoredLength;
        }
        FileSeek(nFile, 0, kFileSeekSet);
    }

    const int nBufferLength = (nInflatedLength < nStoredLength) ? nStoredLength : nInflatedLength;
    int bOwnsBuffer = 0;
    if (pBuffer == nullptr) {
        if (ZoneGetCurrent() == kNoZone) {
            pBuffer = MemAllocTagged(nBufferLength, __FILE__, __LINE__);
            bOwnsBuffer = 1;
        } else {
            pBuffer = ZoneAlloc(nBufferLength);
        }
        nLength = nBufferLength;
    } else if (nLength < static_cast<unsigned>(nBufferLength)) {
        AsyncRequest request;
        memset(&request, 0, sizeof(request));
        request.mId = g_nAsyncNextJobId;
        request.mFile = nFile;
        request.mCallback = pCallback;
        request.mStatus = kAsyncStatusPending;
        // The file stays open. No flag is set to close it, and the handle is not stored anywhere.
        AsyncJobComplete(&request, kAsyncStatusBufferTooSmall);
        ++g_nAsyncNextJobId;
        return request.mId;
    }

    if (bGzipped != 0) {
        AsyncRequest request;
        memset(&request, 0, sizeof(request));
        request.mId = g_nAsyncNextJobId;
        request.mFile = nFile;
        request.mBuffer = pBuffer;
        // The stored bytes land against the end of the buffer. That is what makes room for the
        // inflate to run forward over the whole of it.
        request.mReadBuffer = static_cast<char *>(pBuffer) + (nBufferLength - nStoredLength);
        request.mReadLength = nStoredLength;
        request.mLength = nBufferLength;
        request.mStreamFile = ResolveAsyncStreamFile(nFile);
        request.mFlags = kAsyncRequestCloseFile | kAsyncRequestInflate;
        request.mCallback = pCallback;
        request.mStatus = kAsyncStatusPending;
        request.mOwnsBuffer = bOwnsBuffer;
        AsyncQueueRequest(request);
        ++g_nAsyncNextJobId;
        return request.mId;
    }

    // The layer was already brought up at the top of the routine, and this path tests it again.
    if (g_bAsyncInitialised == 0) {
        InitAsync();
    }

    AsyncRequest request;
    memset(&request, 0, sizeof(request));
    request.mId = g_nAsyncNextJobId;
    request.mFile = nFile;
    request.mBuffer = pBuffer;
    request.mReadBuffer = pBuffer;
    request.mReadLength = nLength;
    request.mLength = nLength;
    request.mStreamFile = ResolveAsyncStreamFile(nFile);
    request.mFlags = kAsyncRequestCloseFile;
    request.mCallback = pCallback;
    request.mStatus = kAsyncStatusPending;
    request.mOwnsBuffer = bOwnsBuffer;
    AsyncQueueRequest(request);

    if ((bArkStream != 0) && (g_nAsyncHostMedia == 0)) {
        SeekArkStream(nFile, nLength, kFileSeekCur);
    }
    ++g_nAsyncNextJobId;
    // The file is closed here and kAsyncRequestCloseFile closes it a second time once the request
    // completes. On disc media that first close lands while the read is still queued.
    FileClose(nFile);
    return request.mId;
}

void CountAsyncQueues(int *pnPending, int *pnCompleted, int *pnFreeJobs) {
    *pnPending = static_cast<int>(g_asyncPendingJobs.size());
    *pnCompleted = static_cast<int>(g_asyncCompletedJobs.size());

    *pnFreeJobs = 0;
    for (const AsyncJob *pJob = g_pAsyncFreeJobs; pJob != nullptr; pJob = pJob->mNext) {
        ++*pnFreeJobs;
    }
}
