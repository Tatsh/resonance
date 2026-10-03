#pragma once

#include <stddef.h>
#include <sys/types.h>

class AsyncCallback;

/** The number of job records the ring is built from. */
constexpr int kAsyncJobCount = 512;

/** Bit of a file handle that identifies a stream inside a mounted ark rather than a loose file. */
constexpr int kFileHandleArkStream = 0x4000;

/** Bit of a file handle that identifies a file the host or disc file service opened. */
constexpr int kFileHandleSceFile = 0x2000;

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
 * Its name comes from the debugging symbols of the North American demo release. A job is 28 bytes
 * and is stored in one preallocated ring. No job is ever allocated separately. `mNext` threads a
 * job onto exactly one of three lists at a time (the free list, a stream's pending list, and the
 * completed list).
 *
 * A job covers one chunk of kSectorCacheRowSize bytes. `mSector` therefore indexes 64 KiB chunks
 * of the file rather than drive sectors, and DeliverAsyncJobData() reconstructs the byte offset as
 * `mSector * kSectorCacheRowSize + mSectorOffset`.
 */
struct AsyncJobInfo {
    AsyncJobInfo *mNext; /*!< The next job, null at the end of a chain. +0x00 */
    AsyncJobInfo *mPrev; /*!< The previous job, null at the head of a chain. +0x04 */
    int mSector;         /*!< The 64 KiB chunk of the file the job transfers. +0x08 */
    int mSectorCount;    /*!< Always 32, the drive sectors a chunk occupies. No reader. +0x0c */
    void *mBuffer;       /*!< The destination the copy writes to. +0x10 */
    int mSectorOffset;   /*!< Byte offset inside the chunk the transfer starts at. +0x14 */
    int mLength;         /*!< Bytes to copy. +0x18 */
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
    AsyncJobInfo *mJobs;      /*!< Job chain, released whenever the request exits a list. +0x20 */
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
 * @ghidraAddress NTSC-U/C: 0x0045f000
 * @ghidraAddress PAL: 0x0049c6c0
 */
void InitAsync();

/**
 * Release the job ring.
 *
 * Performs no work when the layer was never brought up. The initialised flag is
 * not cleared, so the layer cannot be brought up again afterwards.
 *
 * @ghidraAddress NTSC-U/C: 0x00460b98
 * @ghidraAddress PAL: 0x0049e258
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
 * @ghidraAddress NTSC-U/C: 0x00460bd0
 * @ghidraAddress PAL: 0x0049e290
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
 * @ghidraAddress NTSC-U/C: 0x0045f148
 * @ghidraAddress PAL: 0x0049c808
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
 * On host media the read runs at once through read() and the request completes in place. On
 * disc media the file position is taken, the transfer is divided at kSectorCacheRowSize boundaries,
 * and for an ark stream every chunk the sector cache already buffers is copied straight out of its
 * row. Each remaining chunk takes a job off the free chain. A request with no job at all completes
 * immediately.
 *
 * @param request The request to queue.
 * @ghidraAddress NTSC-U/C: 0x0045fc90
 * @ghidraAddress PAL: 0x0049d350
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
 * @ghidraAddress NTSC-U/C: 0x0045ffa8
 * @ghidraAddress PAL: 0x0049d668
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
 * @ghidraAddress NTSC-U/C: 0x00460d58
 * @ghidraAddress PAL: 0x0049e418
 */
int AsyncGetIdFromFd(int nFile);

/**
 * Report whether the operation the drive is servicing right now covers a chunk.
 *
 * A row the in-flight operation is still filling must not be read out of the cache, which is the
 * one use of this test.
 *
 * @param nFile The resolved file.
 * @param nSector The 64 KiB chunk index.
 * @return Non-zero when the current operation is filling that chunk.
 * @ghidraAddress NTSC-U/C: 0x00460f78
 * @ghidraAddress PAL: 0x0049e638
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
 * @ghidraAddress NTSC-U/C: 0x00460590
 * @ghidraAddress PAL: 0x0049dc50
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
 * @ghidraAddress NTSC-U/C: 0x0045f8d8
 * @ghidraAddress PAL: 0x0049cf98
 */
void AsyncPumpCompletedRequests();

/**
 * Report whether the disc is ready to read.
 *
 * This build always reports 1. MetRenderer::Update() is the one caller, and it shows
 * `met_disc_prob.view` while the report is 0. The name is inferred from that view.
 *
 * @return 1.
 * @ghidraAddress NTSC-U/C: 0x00460b20
 * @ghidraAddress PAL: 0x0049e1e0
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
 * @ghidraAddress NTSC-U/C: 0x00460b28
 * @ghidraAddress PAL: 0x0049e1e8
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
 * @ghidraAddress NTSC-U/C: 0x0045f658
 * @ghidraAddress PAL: 0x0049cd18
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
 * @ghidraAddress NTSC-U/C: 0x0045f738
 * @ghidraAddress PAL: 0x0049cdf8
 */
void AsyncCancelRequest(int nHandle);

/**
 * Report the queue to the log.
 *
 * @ghidraAddress NTSC-U/C: 0x0045faf0
 * @ghidraAddress PAL: 0x0049d1b0
 */
void AsyncDump();

/**
 * Report how much work the queue is holding.
 *
 * Every count is walked rather than stored. AsyncDump() reports the same three quantities through
 * its own copies of these loops rather than through this routine, and the names here come from the
 * messages it prints them with. The one caller is a debug reader outside this subsystem.
 *
 * @param nPending Receives the number of queued requests.
 * @param nCompleted Receives the number of finished requests no caller has taken yet.
 * @param nFreeJobs Receives the number of job records still free.
 * @ghidraAddress NTSC-U/C: 0x0045fa38
 * @ghidraAddress PAL: 0x0049d0f8
 */
void AsyncStatus(int &nPending, int &nCompleted, int &nFreeJobs);

/**
 * Take the next job off the free list.
 *
 * Exhausting the free list is fatal.
 *
 * @return The job, unlinked from the free list.
 * @ghidraAddress NTSC-U/C: 0x00460d90
 * @ghidraAddress PAL: 0x0049e450
 */
AsyncJobInfo *AsyncGetFreeJobChain();

/**
 * Put a whole chain of jobs back on the free list.
 *
 * The chain is walked to its end, which is then spliced onto the current free
 * list, and the chain head becomes the new free list head. A null argument
 * produces no work.
 *
 * @param pChain The head of the chain to release.
 * @ghidraAddress NTSC-U/C: 0x00460dd8
 * @ghidraAddress PAL: 0x0049e498
 */
void AsyncReturnJobChain(AsyncJobInfo *pChain);

/**
 * Open a file by path.
 *
 * The game's replacement for the C library's `open()`, which its trace line `open(%s) at t:%f`
 * names. The newlib access mode and the append, create, and truncate bits are translated to the
 * file service's `SCE_*` flags.
 *
 * A request to write goes to `host0:`. On a CD-only boot, a write to a `.py`, `.pyc`, or `.gz`
 * path is refused with -1 instead.
 *
 * A read first tries a mounted archive when UsingArkFiles() reports ark use, which yields an ark
 * stream with kFileHandleArkStream set. Otherwise, and on a CD-only boot for a path that is not
 * `.py`, `.pyc`, or `.gz`, the path is opened on the disc as `cdrom0:` plus
 * FilenameToISO9660() for either CD boot mode, or on `host0:` for a host-only boot. A disc open
 * that fails falls back to `host0:` when the boot mode permits both media. A file-service handle
 * has kFileHandleSceFile set.
 *
 * Each open other than the log's own is traced to the file log with the handle it produced.
 *
 * @param pszPath The file to open.
 * @param nFlags The newlib open flags.
 * @return The file. A negative result reports that the open failed.
 * @ghidraAddress NTSC-U/C: 0x0047c9c0
 * @ghidraAddress PAL: 0x004ba650
 */
int FileOpen(const char *pszPath, int nFlags, ...);

/**
 * Read one run of bytes from a file.
 *
 * The game's own `read()`, in place of the C library's. An ark stream goes to
 * ReadArkStreamThroughCache(). A file-service handle waits for the async layer and the drive and
 * goes to `sceRead()` with kFileHandleSceFile cleared. Any other handle goes to the console reader.
 * Every read is traced to the file log.
 *
 * @param nFile The file to read.
 * @param pBuffer The destination.
 * @param nLength The number of bytes to read.
 * @return The number of bytes transferred, which is not positive on failure.
 * @ghidraAddress NTSC-U/C: 0x0047e060
 * @ghidraAddress PAL: 0x004bbd38
 */
extern "C" ssize_t read(int nFile, void *pBuffer, size_t nLength);

/**
 * Move a file's read position.
 *
 * The game's replacement for the C library's `lseek()`, the routine its trace line identifies. It
 * dispatches on the handle in the same way as read(), to SeekArkStream(), to `sceLseek()`, or to
 * the console stub. The console stub reports -1. Every seek is traced to the file log.
 *
 * @param nFile The file to move.
 * @param nOffset The offset to move by.
 * @param nOrigin One of FileSeekOrigin.
 * @return The resulting position.
 * @ghidraAddress NTSC-U/C: 0x0047e1c8
 * @ghidraAddress PAL: 0x004bbea0
 */
int FileSeek(int nFile, int nOffset, int nOrigin);

/**
 * Close a file.
 *
 * The game's own `close()`, in place of the C library's. The close is traced to the file log
 * first. An ark stream's record is erased, a file-service handle goes to `sceClose()`, and
 * any other handle goes to the console stub. The console stub reports -1.
 *
 * @param nFile The file to close.
 * @return The result of the close.
 * @ghidraAddress NTSC-U/C: 0x0047dfb0
 * @ghidraAddress PAL: 0x004bbc88
 */
extern "C" int close(int nFile);

/**
 * Write to a file.
 *
 * The game's own `write()`, in place of the C library's. An ark stream,
 * kFileHandleArkStream, cannot be written and reports -1. A file-service handle goes to
 * `sceWrite()` with kFileHandleSceFile cleared, and any other handle goes to the console writer.
 * Writes are not traced. The C library's write path and the embedded interpreter's `posix.write`
 * call it.
 *
 * @param nFile The file to write.
 * @param pBuffer The source.
 * @param nLength The number of bytes to write.
 * @return The number of bytes transferred, or -1 on failure.
 * @ghidraAddress NTSC-U/C: 0x0047e178
 * @ghidraAddress PAL: 0x004bbe50
 */
extern "C" ssize_t write(int nFile, const void *pBuffer, size_t nLength);

/**
 * Report whether a file is an interactive terminal.
 *
 * The game's own `isatty()`, in place of the C library's. An ark stream or a file-service handle
 * is never a terminal, and any other handle goes to the console stub. The console stub reports 1.
 * The embedded interpreter's `raw_input` and its interactive-input test call it.
 *
 * @param nFile The file to test.
 * @return Non-zero for a terminal.
 * @ghidraAddress NTSC-U/C: 0x0047e2f0
 * @ghidraAddress PAL: 0x004bbfc8
 */
extern "C" int isatty(int nFile);

/**
 * Report an ark stream's read position.
 *
 * The position reported is ArkStream::mArkPosition, measured inside the whole archive rather than
 * inside the stream's file.
 *
 * @param nStream The ark stream handle, kFileHandleArkStream included.
 * @return The position, or -1 when no stream record has that handle.
 * @ghidraAddress NTSC-U/C: 0x0055c028
 * @ghidraAddress PAL: 0x0059d248
 */
int ArkfileGetCurrAbsOffset(int nStream);

/**
 * Move an ark stream's read position.
 *
 * A resulting position before the start of the stream is clamped back to the start. A stream whose
 * archive is no longer mounted, and an origin outside FileSeekOrigin, both report -1.
 *
 * @param nStream The ark stream handle, kFileHandleArkStream included.
 * @param nOffset The offset to move by.
 * @param nOrigin One of FileSeekOrigin.
 * @return The resulting position, or -1 when no stream record has that handle.
 * @ghidraAddress NTSC-U/C: 0x0055bd38
 * @ghidraAddress PAL: 0x0059cf58
 */
int SeekArkStream(int nStream, int nOffset, int nOrigin);
