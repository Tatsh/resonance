#pragma once

/** The number of job records the ring is built from. */
constexpr int kAsyncJobCount = 512;

/**
 * One queued asynchronous read.
 *
 * The type name comes from the log strings, which write of pending jobs,
 * completed jobs, and free job chains. A job is 28 bytes and lives in one
 * preallocated ring, so no job is ever allocated on its own. `mNext` threads a
 * job onto exactly one of three lists at a time, which are the free list, a
 * stream's pending list, and the completed list.
 *
 * Only the fields the queue routines touch have been recovered.
 */
struct AsyncJob {
    AsyncJob *mNext;   /*!< The next job, null at the end of a chain. +0x00 */
    AsyncJob *mPrev;   /*!< The previous job, null at the head of a chain. +0x04 */
    int mSector;       /*!< The media sector the read starts at. +0x08 */
    int mUnknown0c;    /*!< Undetermined. +0x0c */
    void *mBuffer;     /*!< The destination the copy writes to. +0x10 */
    int mSectorOffset; /*!< Added to mSector scaled by the sector size. +0x14 */
    int mLength;       /*!< Bytes to copy. +0x18 */
};

/**
 * Bring up the asynchronous file-I/O layer.
 *
 * Allocates the 512-job ring, threads it into one doubly linked free list, and
 * clears the current operation. When the layer runs threaded, a semaphore and a
 * worker thread are also created. The routine is idempotent through the
 * initialised flag, and every queue entry point calls it first.
 *
 * @ghidraAddress 0x0045f000
 */
void InitAsync();

/**
 * Release the job ring.
 *
 * Performs no work when the layer was never brought up. The initialised flag is
 * not cleared, so the layer cannot be brought up again afterwards.
 *
 * @ghidraAddress 0x00460b98
 */
void ShutdownAsync();

/**
 * One queued or completed asynchronous request.
 *
 * The record is 48 bytes and lives inline in the pending and completed lists, so a list node is
 * the sixteen-byte node header followed by this. Only the fields the poll and cancel paths touch
 * have been recovered, and the submit path builds the same 48-byte shape before handing it over.
 */
struct AsyncRequest {
    int mId;            /*!< Identifier the poll and cancel paths match on. +0x00 */
    int mFile;          /*!< The file, closed on cancel when bit 0 of mFlags is set. +0x04 */
    void *mBuffer;      /*!< Destination, released on cancel when mOwnsBuffer is set. +0x08 */
    int mUnknown0c;     /*!< Undetermined. +0x0c */
    int mUnknown10;     /*!< Undetermined. +0x10 */
    unsigned mFlags;    /*!< Bit 0 makes cancel close mFile. +0x14 */
    int mUnknown18;     /*!< Undetermined. +0x18 */
    int mUnknown1c;     /*!< Undetermined. +0x1c */
    AsyncJob *mJobs;    /*!< Job chain, released whenever the request leaves a list. +0x20 */
    int mOwnsBuffer;    /*!< Non-zero when cancel must release mBuffer. +0x24 */
    int mStatus;        /*!< What AsyncPollComplete reports. +0x28 */
    int mUnknown2c;     /*!< Undetermined. +0x2c */
};

/**
 * Advance the operation the media is servicing.
 *
 * The name is attested by the routine's own report, `AsyncCheck: unexpected op status: %d`. The
 * body returns at once unless the operation state at 0x006e9150 is 1 or 2. Otherwise it polls the
 * media, reports `HEY - 10 SECONDS SINCE ASYNC OP` once the wait passes 10001 ticks, treats a CD
 * error as fatal through `CD ERROR: %d on sector %d, NOT retrying...`, and on completion advances
 * the state through AsyncIssueOp. Both call sites, ArkFile::Open and the ark reader, pass 1 and
 * sit immediately before a synchronous read, which is what stops a queued read from racing it.
 *
 * @param nBlocking Non-zero to keep polling until the operation settles.
 * @ghidraAddress 0x00460590
 */
void AsyncCheck(int nBlocking);

/**
 * Report a finished request and take it off the completed list.
 *
 * The request's job chain is released and its node erased. Either output pointer may be null.
 *
 * @param nId The request identifier.
 * @param pnOut1 Receives the request's +0x08 field, or null.
 * @param pnOut2 Receives the request's +0x14 field, or null.
 * @return The request's status, or -1 when no completed request has that identifier.
 * @ghidraAddress 0x0045f658
 */
int AsyncPollComplete(int nId, int *pnOut1, int *pnOut2);

/**
 * Abandon a request wherever it sits.
 *
 * Both lists are searched. A matching request has its buffer released when it owns it, its file
 * closed when bit 0 of its flags is set, its job chain released, and its node erased. async.cpp
 * lines 481 and 499.
 *
 * @param nId The request identifier.
 * @ghidraAddress 0x0045f738
 */
void AsyncCancelRequest(int nId);

/**
 * Report the queue to the log.
 *
 * @ghidraAddress 0x0045faf0
 */
void AsyncDump();

/**
 * Take the next job off the free list.
 *
 * Exhausting the free list is fatal.
 *
 * @return The job, unlinked from the free list.
 * @ghidraAddress 0x00460d90
 */
AsyncJob *AsyncGetFreeJobChain();

/**
 * Put a whole chain of jobs back on the free list.
 *
 * The chain is walked to its end, which is then spliced onto the current free
 * list, and the chain head becomes the new free list head. A null argument
 * produces no work.
 *
 * @param pChain The head of the chain to release.
 * @ghidraAddress 0x00460dd8
 */
void AsyncReleaseJobChain(AsyncJob *pChain);

/**
 * Collect a finished read by handle.
 *
 * Walks the completed list for the job whose identifier matches, reports its result and status
 * through whichever out-parameters are supplied, then releases the job chain and unlinks it. Both
 * out-parameters are optional and a null one is skipped.
 *
 * @param nHandle The identifier the submit returned.
 * @param ppResult Receives the job's result, or null to discard it.
 * @param pnStatus Receives the job's status, or null to discard it.
 * @return Non-zero when a matching job was collected.
 * @ghidraAddress 0x0045f658
 */
int AsyncPollComplete(int nHandle, void **ppResult, int *pnStatus);
