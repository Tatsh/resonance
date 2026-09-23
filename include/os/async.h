#pragma once

class AsyncCallback;

/** The number of job records the ring is built from. */
constexpr int kAsyncJobCount = 512;

/** Bit of a file handle that identifies a stream inside a mounted ark rather than a loose file. */
constexpr int kFileHandleArkStream = 0x4000;

/** Origins FileSeek() and SeekArkStream() accept. */
enum FileSeekOrigin {
    kFileSeekSet = 0, /*!< Measure the offset from the start. */
    kFileSeekCur = 1, /*!< Measure the offset from the current position. */
    kFileSeekEnd = 2  /*!< Measure the offset from the end. */
};

/** Bit of AsyncRequest::mFlags that makes completion close the request's file. */
constexpr unsigned kAsyncRequestCloseFile = 1;

/**
 * Bit of AsyncRequest::mFlags that makes completion inflate what the read delivered.
 *
 * The inflate runs forward from mReadBuffer over mBuffer, which is why AsyncLoadFileByPath() reads
 * a compressed file into the tail of its buffer.
 */
constexpr unsigned kAsyncRequestInflate = 2;

/** Stored in AsyncRequest::mStatus while the request has not completed. */
constexpr int kAsyncStatusPending = -1;

/** Stored in AsyncRequest::mStatus once the data is in place. */
constexpr int kAsyncStatusOk = 0;

/** Stored in AsyncRequest::mStatus when the path could not be opened. */
constexpr int kAsyncStatusOpenFailed = 2;

/** Stored in AsyncRequest::mStatus when the caller's buffer is smaller than the file. */
constexpr int kAsyncStatusBufferTooSmall = 3;

/** Stored in AsyncRequest::mStatus when the read itself failed. */
constexpr int kAsyncStatusReadFailed = 4;

/** Stored in AsyncRequest::mStatus when the read succeeded and the inflate did not. */
constexpr int kAsyncStatusInflateFailed = 5;

/**
 * One queued asynchronous read.
 *
 * The type name comes from the log strings, which write of pending jobs,
 * completed jobs, and free job chains. A job is 28 bytes and lives in one
 * preallocated ring, so no job is ever allocated on its own. `mNext` threads a
 * job onto exactly one of three lists at a time, which are the free list, a
 * stream's pending list, and the completed list.
 *
 * A job covers one chunk of kSectorCacheRowSize bytes. `mSector` therefore indexes 64 KiB chunks
 * of the file rather than drive sectors, and DeliverAsyncJobData() reconstructs the byte offset as
 * `mSector * kSectorCacheRowSize + mSectorOffset`.
 */
struct AsyncJob {
    AsyncJob *mNext;   /*!< The next job, null at the end of a chain. +0x00 */
    AsyncJob *mPrev;   /*!< The previous job, null at the head of a chain. +0x04 */
    int mSector;       /*!< The 64 KiB chunk of the file the job transfers. +0x08 */
    int mUnknown0c;    /*!< Always 32, the drive sectors a chunk occupies. No reader. +0x0c */
    void *mBuffer;     /*!< The destination the copy writes to. +0x10 */
    int mSectorOffset; /*!< Byte offset inside the chunk the transfer starts at. +0x14 */
    int mLength;       /*!< Bytes to copy. +0x18 */
};

/**
 * One queued or completed asynchronous request.
 *
 * The record is 48 bytes and lives inline in the pending and completed lists. A list node is
 * therefore the eight-byte node header followed by this.
 *
 * Two windows are recorded rather than one. mBuffer and mLength describe what the caller receives
 * and are what AsyncPollComplete() reports, while mReadBuffer and mReadLength describe where the
 * drive data lands and how much of it there is. The two agree for an ordinary read, and they differ
 * for a compressed one, where the stored bytes are read into the tail of the buffer and then
 * inflated forward over the whole of it.
 *
 * Every member is public because AsyncSubmitRequest() and AsyncLoadFileByPath() build the record
 * field by field with no accessor anywhere in the image, and the record has no behaviour of its
 * own.
 */
struct AsyncRequest {
    int mId;                  /*!< Identifier the poll and cancel paths match on. +0x00 */
    int mFile;                /*!< The file, with kFileHandleArkStream for an ark stream. +0x04 */
    void *mBuffer;            /*!< Destination the caller receives. +0x08 */
    void *mReadBuffer;        /*!< Destination the drive data lands in. +0x0c */
    int mReadLength;          /*!< Bytes to transfer from the file. +0x10 */
    int mLength;              /*!< Bytes the caller receives. +0x14 */
    int mStreamFile;          /*!< mFile resolved to the file the sector cache is keyed by. +0x18 */
    unsigned mFlags;          /*!< kAsyncRequestCloseFile and kAsyncRequestInflate. +0x1c */
    AsyncJob *mJobs;          /*!< Job chain, released whenever the request exits a list. +0x20 */
    AsyncCallback *mCallback; /*!< Receiver the pump reports completion to, or null. +0x24 */
    int mStatus;              /*!< What AsyncPollComplete() reports. +0x28 */
    int mOwnsBuffer;          /*!< Non-zero when cancel must release mBuffer. +0x2c */
};

/**
 * Bring up the asynchronous file-I/O layer.
 *
 * Allocates the 512-job ring, threads it into one doubly linked free list, and clears the current
 * operation. On disc media the drive callback thread is created and AsyncMediaEventCallback() is
 * installed as the drive completion callback. The routine is idempotent through the initialised
 * flag, and every queue entry point calls it first.
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
 * Queue one asynchronous read and report its identifier.
 *
 * Brings the layer up on first use, then builds a request whose caller window and read window both
 * describe the whole transfer. For an ark stream on disc media the stream position is advanced past
 * the data the read will deliver once the request is queued. A following request therefore
 * continues where this one ends.
 *
 * @param nFile The file to read, positioned where the read should start.
 * @param pBuffer The destination.
 * @param nLength The number of bytes to read.
 * @param bCloseOnComplete Non-zero to close nFile once the request completes.
 * @param pCallback Receiver notified once the request completes, or null.
 * @param bOwnsBuffer Non-zero to have a cancelled request release pBuffer.
 * @return The request identifier, which AsyncPollComplete() and AsyncCancelRequest() match on.
 * @ghidraAddress 0x00460bd0
 */
int AsyncSubmitRequest(int nFile,
                       void *pBuffer,
                       int nLength,
                       int bCloseOnComplete,
                       AsyncCallback *pCallback,
                       int bOwnsBuffer);

/**
 * Open a path and queue the whole file as one asynchronous read.
 *
 * A path ending in `.gz` is inflated in place once the read completes. The buffer is sized to the
 * larger of the stored and inflated sizes, the stored bytes are read into its tail, and
 * kAsyncRequestInflate makes completion inflate them forward over the buffer. With no buffer
 * supplied one is allocated, from the selected zone when there is one. An ark stream reports both
 * sizes from its directory entry, and a loose gzip file reports the inflated size from its own
 * last four bytes.
 *
 * Two failures are reported as a completed request rather than through the return value. A path
 * that will not open completes with kAsyncStatusOpenFailed, and a buffer smaller than the file
 * completes with kAsyncStatusBufferTooSmall. Either way the identifier is a real one a caller can
 * poll.
 *
 * @param pszPath The file to read.
 * @param pBuffer The destination, or null to have one allocated.
 * @param nLength The destination size, ignored when pBuffer is null. The unsigned type is proven by
 *                the unsigned comparison against the file size and by the zero extension at the
 *                stream-advance call.
 * @param pCallback Receiver notified once the request completes, or null.
 * @return The request identifier.
 * @ghidraAddress 0x0045f148
 */
int AsyncLoadFileByPath(const char *pszPath,
                        void *pBuffer,
                        unsigned nLength,
                        AsyncCallback *pCallback);

/**
 * Split a request into chunk jobs and put it on the pending list.
 *
 * The request is passed by value, which is what the image does. Every caller copies the 48-byte
 * record into its outgoing argument area and passes the address of the copy.
 *
 * On host media the read runs at once through FileRead() and the request completes in place. On
 * disc media the file position is taken, the transfer is divided at kSectorCacheRowSize boundaries,
 * and for an ark stream every chunk the sector cache already buffers is copied straight out of its
 * row. Each remaining chunk takes a job off the free chain. A request with no job at all completes
 * immediately.
 *
 * @param request The request to queue.
 * @ghidraAddress 0x0045fc90
 */
void AsyncQueueRequest(AsyncRequest request);

/**
 * Finish a request and put it on the completed list.
 *
 * A positive status is reported through the log as `AsyncJobComplete: job %d has error: %d` and
 * skips the inflate. kAsyncRequestInflate inflates mReadLength bytes from mReadBuffer over
 * mBuffer, and a failed inflate is reported as status 5. kAsyncRequestCloseFile then closes mFile.
 *
 * @param pRequest The request to finish.
 * @param nStatus The status to record.
 * @ghidraAddress 0x0045ffa8
 */
void AsyncJobComplete(AsyncRequest *pRequest, int nStatus);

/**
 * Resolve a file handle to the file the sector cache is keyed by.
 *
 * An ark stream handle resolves to the archive's own file. Two streams inside one archive therefore
 * share its cache rows. Any other handle resolves to itself.
 *
 * @param nFile The file handle.
 * @return The resolved file, or -1 when no stream record has that handle.
 * @ghidraAddress 0x00460d58
 */
int ResolveAsyncStreamFile(int nFile);

/**
 * Report whether the operation the drive is servicing right now covers a chunk.
 *
 * A row the in-flight operation is still filling must not be read out of the cache, which is the
 * one use of this test.
 *
 * @param nFile The resolved file.
 * @param nSector The 64 KiB chunk index.
 * @return Non-zero when the current operation is filling that chunk.
 * @ghidraAddress 0x00460f78
 */
int MatchesCurrentAsyncOp(int nFile, int nSector);

/**
 * Advance the operation the media is servicing.
 *
 * The name is attested by the routine's own report, `AsyncCheck: unexpected op status: %d`. The
 * body returns at once unless the current chunk transfer has a command in flight. Otherwise it
 * waits for that command, then advances the transfer: a finished seek issues the read, and a
 * finished read marks the data ready for AsyncPumpCompletedRequests() to distribute.
 *
 * How the wait is performed depends on the media. On disc media the drive callback thread raises a
 * flag the routine consumes, and without that thread the routine calls sceCdSync() instead. Either
 * way a drive error of SCECdErTRMOPN waits for the tray and requests a retry, and every other drive
 * error is fatal through `CD ERROR: %d on sector %d, NOT retrying...`.
 *
 * A command that has been in flight for more than three seconds also consults the drive directly. A
 * drive reporting SCECdNotReady requests a retry, and a ready drive more than ten seconds in
 * reports `HEY - 10 SECONDS SINCE ASYNC OP` through a routine whose body is empty in this build.
 *
 * Thirteen call sites, most of them immediately before a synchronous read. That is what stops a
 * queued read from racing the synchronous one.
 *
 * @param nBlocking Non-zero to keep waiting until the command settles, zero to return as soon as
 *                  the drive reports that it is still busy.
 * @ghidraAddress 0x00460590
 */
void AsyncCheck(int nBlocking);

/**
 * Service the queue once.
 *
 * On disc media the routine advances the chunk transfer the drive is performing. A finished chunk
 * is distributed to every pending request covering it and its cache row is unlocked, and an idle
 * drive is then given the next chunk any pending request still needs. Nothing here waits for the
 * drive. A caller that wants the data now calls AsyncCheck() instead.
 *
 * Every completed request is then reported to its callback, its jobs are released, and its record
 * is erased. A request with no callback is erased in the same pass. A caller that wants the buffer
 * back through AsyncPollComplete() therefore has to poll before the next pump.
 *
 * @ghidraAddress 0x0045f8d8
 */
void AsyncPumpCompletedRequests();

/**
 * Report whether the disc is ready to read.
 *
 * This build always reports 1. MetRenderer::OnUnknownSlot7() is the one caller, and it shows
 * `met_disc_prob.view` while the report is 0. The name is inferred from that view.
 *
 * @return 1.
 * @ghidraAddress 0x00460b20
 */
int IsMediaReady();

/**
 * Drive completion callback the async layer installs on disc media.
 *
 * The shape is libcdvd's `sceCdCBFunc`, and the argument is the function code of the command that
 * just finished. The drive error code is latched on every report, and a finished read or seek
 * raises the flag AsyncCheck() waits on. Every other code only latches the error.
 *
 * @param nFunction The libcdvd function code of the finished command.
 * @ghidraAddress 0x00460b28
 */
void AsyncMediaEventCallback(int nFunction);

/**
 * Report a finished request and take it off the completed list.
 *
 * The request's job chain is released and its node erased. Either output pointer may be null.
 *
 * @param nHandle The identifier AsyncSubmitRequest() reported.
 * @param ppBuffer Receives the request's buffer, or null to discard it.
 * @param pnLength Receives the number of bytes the request covers, or null to discard it.
 * @return The request's status, or -1 when no completed request has that identifier.
 * @ghidraAddress 0x0045f658
 */
int AsyncPollComplete(int nHandle, void **ppBuffer, int *pnLength);

/**
 * Abandon a request wherever it sits.
 *
 * Both lists are searched. A matching request has its buffer released when mOwnsBuffer is set, its
 * file closed when kAsyncRequestCloseFile is set, its job chain released, and its node erased.
 * async.cpp lines 481 and 499.
 *
 * @param nHandle The identifier AsyncSubmitRequest() reported.
 * @ghidraAddress 0x0045f738
 */
void AsyncCancelRequest(int nHandle);

/**
 * Report the queue to the log.
 *
 * @ghidraAddress 0x0045faf0
 */
void AsyncDump();

/**
 * Report how much work the queue is holding.
 *
 * Every count is walked rather than stored. AsyncDump() reports the same three quantities through
 * its own copies of these loops rather than through this routine, and the names here come from the
 * messages it prints them with. The one caller is a debug reader outside this subsystem.
 *
 * @param pnPending Receives the number of queued requests.
 * @param pnCompleted Receives the number of finished requests no caller has taken yet.
 * @param pnFreeJobs Receives the number of job records still free.
 * @ghidraAddress 0x0045fa38
 */
void CountAsyncQueues(int *pnPending, int *pnCompleted, int *pnFreeJobs);

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
 * Open a file by path.
 *
 * The routine belongs to another agent's subsystem and is declared here so async.cpp can call it.
 * A path inside a mounted archive resolves to an ark stream handle, with kFileHandleArkStream set.
 *
 * @param pszPath The file to open.
 * @param nMode Zero at every call site in this subsystem.
 * @return The file. A negative result reports that the open failed.
 * @ghidraAddress 0x0047c9c0
 */
int FileOpen(const char *pszPath, int nMode);

/**
 * Read one run of bytes from a file.
 *
 * The routine belongs to another agent's subsystem and is declared here so async.cpp can call it.
 * It dispatches on the handle, to the ark reader for an ark stream and to the host or disc reader
 * otherwise.
 *
 * @param nFile The file to read.
 * @param pBuffer The destination.
 * @param nLength The number of bytes to read.
 * @return The number of bytes transferred, which is not positive on failure.
 * @ghidraAddress 0x0047e060
 */
int FileRead(int nFile, void *pBuffer, int nLength);

/**
 * Move a file's read position.
 *
 * The routine belongs to another agent's subsystem and is declared here so async.cpp can call it.
 *
 * @param nFile The file to move.
 * @param nOffset The offset to move by.
 * @param nOrigin One of FileSeekOrigin.
 * @return The resulting position.
 * @ghidraAddress 0x0047e1c8
 */
int FileSeek(int nFile, int nOffset, int nOrigin);

/**
 * Close a file.
 *
 * The routine belongs to another agent's subsystem and is declared here so async.cpp can call it.
 *
 * @param nFile The file to close.
 * @ghidraAddress 0x0047dfb0
 */
void FileClose(int nFile);

/**
 * Report an ark stream's read position.
 *
 * The routine belongs to another agent's subsystem and is declared here so async.cpp can call it.
 * The position is measured inside the stream rather than inside the archive.
 *
 * @param nStream The ark stream handle, kFileHandleArkStream included.
 * @return The position, or -1 when no stream record has that handle.
 * @ghidraAddress 0x0055c028
 */
int GetArkStreamPosition(int nStream);

/**
 * Move an ark stream's read position.
 *
 * The routine belongs to another agent's subsystem and is declared here so async.cpp can call it.
 * A resulting position before the start of the stream is clamped back to the start.
 *
 * @param nStream The ark stream handle, kFileHandleArkStream included.
 * @param nOffset The offset to move by.
 * @param nOrigin One of FileSeekOrigin.
 * @return The resulting position, or -1 when no stream record has that handle.
 * @ghidraAddress 0x0055bd38
 */
int SeekArkStream(int nStream, int nOffset, int nOrigin);
