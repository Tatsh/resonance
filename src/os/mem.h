#pragma once

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
/**
 * The number of per-tag accounting records.
 *
 * Record 0 is the overflow bucket. It receives the byte count of every allocation whose tag did
 * not fit in records 1 through 23, and MemLogBeginCount() labels it `Other_Sources`.
 */
constexpr int kMemTagCount = 24;

/** Bytes a tag may occupy in an accounting record, including the terminator. */
constexpr int kMemTagNameSize = 124;

/**
 * One row of the memory report.
 *
 * The name is the tag string the allocation was billed to, which is normally a
 * `__FILE__` or a class name. Records are claimed in order as new tags appear,
 * and the table is never cleared.
 */
struct MemTagTotal {
    char mName[kMemTagNameSize]; /*!< The tag this record bills, empty while unclaimed. +0x00 */
    int mBytes;                  /*!< Bytes allocated against the tag so far. +0x7c */
};

/** Bytes the tag the STL allocator hook formats may occupy, including the terminator. */
constexpr int kMemStlTagSize = 128;
#endif

#ifdef __cplusplus
/**
 * Allocate a block for an array, the game's replacement global `operator new[]`.
 *
 * The request is raised to one byte when it is zero. The allocation is billed
 * to the tag `UNK[]`, and a failure is fatal. Its exception tables carry the
 * `throw(std::bad_alloc)` specification.
 *
 * @param nSize The block size in bytes.
 * @return The block.
 * @ghidraAddress NTSC-U/C: 0x004a8380
 * @ghidraAddress PAL: 0x004e6490
 */
void *operator new[](size_t nSize);

/**
 * Allocate a block for a single object, the game's replacement global `operator new`.
 *
 * The request is raised to one byte when it is zero. The allocation is billed
 * to the tag `UNK`, and a failure is fatal. The log line and the failure
 * message both omit a tag, and the message reads
 * `NEW ALLOCATION FAILURE, size: %d`. Its exception tables carry the
 * `throw(std::bad_alloc)` specification.
 *
 * @param nSize The block size in bytes.
 * @return The block.
 * @ghidraAddress NTSC-U/C: 0x004a81e0
 * @ghidraAddress PAL: 0x004e62f0
 */
void *operator new(size_t nSize);
#endif

/**
 * Allocate a block for one object of a named class.
 *
 * This is the allocator every class-specific `operator new` in the image
 * forwards to. Each of those is eight instructions that pass the request
 * through and supply the class name as the second argument. 637 call sites
 * exist and at least 41 distinct class names appear among them, including the
 * qualified forms `Rnd::Mesh` and `Rnd::Cam`. The tag reaches the accounting
 * table unreduced, and the failure message reads
 * `NEW ALLOCATION FAILURE, class: %s, size: %d`. The request is raised to one
 * byte when it is zero, and a failure is fatal.
 *
 * @param nSize The block size in bytes.
 * @param pszClass The class name to bill the allocation to.
 * @return The block.
 * @ghidraAddress NTSC-U/C: 0x004a90a0
 * @ghidraAddress PAL: 0x004e71b0
 */
void *AllocateTaggedMemory(size_t nSize, const char *pszClass);

/**
 * Release a block that AllocateTaggedMemory() handed out.
 *
 * The class name reaches the log line unreduced. Unlike MemFreeTagged() this
 * path does not test the block against the zones.
 *
 * @param pBlock The block to release.
 * @param pszClass The class name the allocation was billed to.
 * @ghidraAddress NTSC-U/C: 0x004a91e0
 * @ghidraAddress PAL: 0x004e72f0
 */
void OperatorDeleteOverride(void *pBlock, const char *pszClass);

/**
 * Address the buffer the STL allocator hook bills its allocations to.
 *
 * The buffer is the one MemSetStlTag() formats into, and MemAllocTagged()
 * rewinds it to `stl_unk` after every tagged allocation. Callers pass the
 * result straight on as a tag.
 *
 * @return The tag buffer.
 * @ghidraAddress NTSC-U/C: 0x004a9090
 * @ghidraAddress PAL: 0x004e71a0
 */
char *MemGetCurrentTag();

/**
 * Bill the next STL allocation to a container kind and element size.
 *
 * The two arguments are formatted as `%s.%d`. That produces a tag such as
 * `stl_vector.8`. Every STL allocation hook calls this and then passes
 * MemGetCurrentTag() to MemAllocTagged().
 *
 * @param pszKind The container kind, for example `stl_vector`.
 * @param nElemSize The element size in bytes.
 * @ghidraAddress NTSC-U/C: 0x004a9048
 * @ghidraAddress PAL: 0x004e7158
 */
void MemSetStlTag(const char *pszKind, int nElemSize);

/**
 * Allocate a block and record the request against a tag.
 *
 * The tag is normally the caller's `__FILE__` or the class name. Totals per tag
 * accumulate into the accounting table, and the log line uses only the tag's
 * basename. A failure is fatal.
 *
 * @param nSize The block size in bytes.
 * @param pszTag The tag to bill the allocation to.
 * @param nLine The caller's line number.
 * @return The block.
 * @ghidraAddress NTSC-U/C: 0x004a8520
 * @ghidraAddress PAL: 0x004e6630
 */
#ifdef __cplusplus
extern "C" {
#endif
void *MemAllocTagged(size_t nSize, const char *pszTag, int nLine);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/**
 * Release an array block, the game's replacement global `operator delete[]`.
 *
 * The log line identifies the path as `del(UNK[],%p)`. Its exception tables include the empty
 * `throw()` specification.
 *
 * @param pBlock The block to release.
 * @ghidraAddress NTSC-U/C: 0x004a92c8
 * @ghidraAddress PAL: 0x004e73d8
 */
void operator delete[](void *pBlock) noexcept;

/**
 * Release a single object, the game's replacement global `operator delete`.
 *
 * The log line identifies the path as `del(UNK,%p)`. HxStr::Alloc() is the one
 * caller inside the string class, in the NTSC-U/C build only. The PAL build's HxStr::Alloc()
 * calls `operator delete[]`. Its exception tables include the empty `throw()` specification.
 *
 * @param pBlock The block to release.
 * @ghidraAddress NTSC-U/C: 0x004a9230
 * @ghidraAddress PAL: 0x004e7340
 */
void operator delete(void *pBlock) noexcept;
#endif

/**
 * Release a block and record the release against a tag.
 *
 * Releasing a block that belongs to a zone is fatal, because a zone hands out
 * bump-pointer slices that cannot be reclaimed individually.
 *
 * @param pBlock The block to release.
 * @param pszTag The tag the allocation was billed to.
 * @param nLine The caller's line number.
 * @ghidraAddress NTSC-U/C: 0x004a94e8
 * @ghidraAddress PAL: 0x004e75f8
 */
#ifdef __cplusplus
extern "C" {
#endif
void MemFreeTagged(void *pBlock, const char *pszTag, int nLine);
#ifdef __cplusplus
}
#endif

/**
 * Find or claim the report row for a source name.
 *
 * The name is reduced to its basename, then matched against the interned source
 * table of 128 rows. A name of 40 characters or more, and an exhausted table,
 * are both fatal. The table is separate from the per-tag accounting table.
 *
 * @param pszName The source name, normally a `__FILE__`.
 * @return The row index.
 * @ghidraAddress NTSC-U/C: 0x004a86c0
 * @ghidraAddress PAL: 0x004e67d0
 */
int MemLogFindSource(const char *pszName);

/**
 * Resize a block and record the move against a tag.
 *
 * Resizing a block that belongs to a zone is fatal, for the same reason releasing one is. On
 * failure the report uses the tag and line unchanged.
 *
 * @param pBlock The block to resize.
 * @param nSize The new size in bytes.
 * @param pszTag The tag the allocation was billed to.
 * @param nLine The caller's line number.
 * @return The block, which differs from pBlock only when it moved.
 * @ghidraAddress NTSC-U/C: 0x004a93b8
 * @ghidraAddress PAL: 0x004e74c8
 */
void *MemReallocTagged(void *pBlock, size_t nSize, const char *pszTag, int nLine);

/**
 * Write a marker line into the memory report.
 *
 * Performs no work while logging is off. The frame loop and HHeapLogBasicStats() are the callers.
 *
 * @param pszText The text to mark.
 * @ghidraAddress NTSC-U/C: 0x004a8e18
 * @ghidraAddress PAL: 0x004e6f28
 */
void MemLogWriteMarker(const char *pszText);

/**
 * Close the memory report and print a summary of the heap.
 *
 * Closing is skipped when the report was never opened, and otherwise also clears the logging
 * flag. The summary is the ten fields of the C library's `mallinfo()` and, once MemOpenLog() has
 * painted the stack, the stack depth reached. DumpHeapMemoryLog(0) runs last. Fatal() calls this,
 * and MemOpenLog() registers it with atexit().
 *
 * @ghidraAddress NTSC-U/C: 0x004a7d30
 * @ghidraAddress PAL: 0x004e5e40
 */
void MemCloseLogAndReport();

/**
 * Open the memory report and paint the stack.
 *
 * A path opens the report for writing and turns logging on when the file opens. Either way the
 * three linker symbols `_stack`, `_stack_size`, and `_end` are logged, every byte of the stack
 * below its top 0x2000 bytes is set to `u` so that a later report can measure the depth reached,
 * and MemCloseLogAndReport() is registered with atexit().
 *
 * The name is inferred.
 *
 * @param pszPath The report path, or null to paint the stack only.
 * @ghidraAddress NTSC-U/C: 0x004a7c40
 * @ghidraAddress PAL: 0x004e5d50
 */
void MemOpenLog(const char *pszPath);

/**
 * Start a new report file that retains everything written so far.
 *
 * The open report is closed, its content is copied into a file named after the original path with
 * `_N` before the extension, and writing continues there. The mallinfo summary and the stack depth
 * are logged afterwards whether or not a report was open. The title is the one its log line
 * gives.
 *
 * @ghidraAddress NTSC-U/C: 0x004a7ef8
 * @ghidraAddress PAL: 0x004e6008
 */
void MemLogCloseAndContinue();

/**
 * Write text to the memory report while logging is on.
 *
 * The image has no caller. The name is inferred.
 *
 * @param pszText The text.
 * @ghidraAddress NTSC-U/C: 0x004a8e50
 * @ghidraAddress PAL: 0x004e6f60
 */
void MemLogPrint(const char *pszText);

/**
 * Clear the per-tag accounting table and start charging it.
 *
 * Record 0 is labelled `Other_Sources`. Rnd::AsyncLoader's poll brackets a load with this and
 * MemLogEndCount().
 *
 * @ghidraAddress NTSC-U/C: 0x004a8e88
 * @ghidraAddress PAL: 0x004e6f98
 */
void MemLogBeginCount();

/**
 * Stop charging the accounting table and format its totals.
 *
 * The report begins `Memory Allocated: %d` and adds one line per labelled record. Record 0 appears
 * only when it has been charged. A line that would leave less than 0x40 bytes of the buffer is
 * replaced by `...REPORT TOO LONG FOR BUFFER!` and ends the report.
 *
 * @param pszReport Receives the report.
 * @param nReportSize The size of the buffer.
 * @return The total bytes charged since MemLogBeginCount().
 * @ghidraAddress NTSC-U/C: 0x004a8ef8
 * @ghidraAddress PAL: 0x004e7008
 */
int MemLogEndCount(char *pszReport, int nReportSize);

/**
 * Allocate the per-block tracking table and clear the per-source table.
 *
 * The block table is 32 MB, 0x200000 slots of 16 bytes. The image has no caller, so block
 * tracking never runs in the shipped build. The name is inferred.
 *
 * @ghidraAddress NTSC-U/C: 0x004a95c8
 * @ghidraAddress PAL: 0x004e76d8
 */
void MemLogSourceInit();

/**
 * Move a tracked block to its new address after a resize.
 *
 * An untracked old block is reported and recorded as a new allocation. Otherwise the entry takes
 * the new address and size and its source's byte totals move by the difference. The title is the
 * one its log line gives.
 *
 * @param pszSource The source the block is billed to.
 * @param pNew The block after the resize.
 * @param pOld The block before the resize.
 * @param nSize The new size in bytes.
 * @ghidraAddress NTSC-U/C: 0x004a87d0
 * @ghidraAddress PAL: 0x004e68e0
 */
void MemLogSourceTrackRealloc(const char *pszSource, void *pNew, void *pOld, int nSize);

/**
 * Print the per-source table, sorted by name.
 *
 * Performs no work before MemLogSourceInit(). DumpHeapMemoryLog() is the one caller. The name is
 * inferred.
 *
 * @param pszTitle The heading line.
 * @param pFile The stream to write, or null for standard output.
 * @ghidraAddress NTSC-U/C: 0x004a8a68
 * @ghidraAddress PAL: 0x004e6b78
 */
void MemLogSourceReport(const char *pszTitle, FILE *pFile);

/**
 * Write the memory statistics files and probe the largest possible allocation.
 *
 * It writes `memdump_%d.txt` through MemLogSourceReport(). It then writes `memstat_%d.txt` with the
 * largest single allocation, probed downward from 128 megabytes in tenths, and the number of
 * 2048-byte and 128-byte blocks that fit, each counted to at most 65536 and released again. Each
 * line also goes to the log. Neither file open is checked.
 *
 * @param nIndex The number the file names carry.
 * @ghidraAddress NTSC-U/C: 0x0054b348
 * @ghidraAddress PAL: 0x0058b878
 */
void DumpHeapMemoryLog(int nIndex);

/**
 * Take a block from the backing allocator.
 *
 * The routine behaves as newlib's `malloc`, toolchain C library linked as shipped. It loads the
 * reentrancy structure at 0x007819cc and calls `_malloc_r` at 0x0059b528. It is not the Heap class
 * in `os/heap.h`, the arena the embedded Python allocates from.
 *
 * @param nSize The block size in bytes.
 * @return The block, or null when the request cannot be met.
 * @ghidraAddress NTSC-U/C: 0x004bfd48
 * @ghidraAddress PAL: 0x004fdde8
 */
#ifdef __cplusplus
extern "C" {
#endif
void *HeapAlloc(size_t nSize);
#ifdef __cplusplus
}
#endif

/**
 * Give a block back to the backing allocator.
 *
 * The routine behaves as newlib's `free`, toolchain C library linked as shipped. It loads the
 * same reentrancy structure and tail-calls `_free_r` at 0x005da3b0.
 *
 * @param pBlock The block to release.
 * @ghidraAddress NTSC-U/C: 0x004bfd70
 * @ghidraAddress PAL: 0x004fde10
 */
#ifdef __cplusplus
extern "C" {
#endif
void HeapFree(void *pBlock);
#ifdef __cplusplus
}
#endif

/**
 * Resize a block through the backing allocator.
 *
 * The toolchain routine underneath is at 0x00589278. Unlike the other two wrappers this one has no
 * out-of-line copy, because MemReallocTagged() is its only caller and inlines it.
 *
 * @param pBlock The block to resize.
 * @param nSize The new size in bytes.
 * @return The block, which differs from pBlock only when it moved.
 * @ghidraAddress NTSC-U/C: 0x00589278
 * @ghidraAddress PAL: 0x005cc4f0
 */
#ifdef __cplusplus
extern "C" {
#endif
void *HeapRealloc(void *pBlock, size_t nSize);
#ifdef __cplusplus
}
#endif

/**
 * Log how much of the backing allocator is free, then give it all back.
 *
 * Blocks of 2048 bytes are taken through HeapAlloc() until a request fails, the count and the
 * total are logged, and every block is released through HeapFree(). The name is inferred. The
 * binary places the routine at the head of the TexturePairRecord unit, and it has no caller.
 *
 * @ghidraAddress NTSC-U/C: 0x00246c18
 * @ghidraAddress PAL: 0x0025bd58
 */
void ReportHeapCapacity();
