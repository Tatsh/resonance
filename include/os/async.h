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
