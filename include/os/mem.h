#pragma once

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
/**
 * The number of per-tag accounting records.
 *
 * Record 0 is the overflow bucket. It receives the byte count of every
 * allocation whose tag did not fit in records 1 through 23, and
 * MemBeginAccounting() titles it `Other_Sources`.
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

/**
 * Allocate a block.
 *
 * The request is raised to one byte when it is zero. The allocation is billed
 * to the tag `UNK[]`, and a failure is fatal. This is the array form, and
 * MemAllocScalar() is the single-object form.
 *
 * @param nSize The block size in bytes.
 * @return The block.
 * @ghidraAddress 0x004a8380
 */
void *MemAlloc(size_t nSize);

/**
 * Allocate a block for a single object.
 *
 * The request is raised to one byte when it is zero. The allocation is billed
 * to the tag `UNK`, and a failure is fatal. The log line and the failure
 * message both omit a tag, and the message reads
 * `NEW ALLOCATION FAILURE, size: %d`.
 *
 * @param nSize The block size in bytes.
 * @return The block.
 * @ghidraAddress 0x004a81e0
 */
void *MemAllocScalar(size_t nSize);

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
 * @ghidraAddress 0x004a90a0
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
 * @ghidraAddress 0x004a91e0
 */
void FreeTaggedMemory(void *pBlock, const char *pszClass);

/**
 * Address the buffer the STL allocator hook bills its allocations to.
 *
 * The buffer is the one MemSetStlTag() formats into, and MemAllocTagged()
 * rewinds it to `stl_unk` after every tagged allocation. Callers pass the
 * result straight on as a tag.
 *
 * @return The tag buffer.
 * @ghidraAddress 0x004a9090
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
 * @ghidraAddress 0x004a9048
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
 * @ghidraAddress 0x004a8520
 */
#ifdef __cplusplus
extern "C" {
#endif
void *MemAllocTagged(size_t nSize, const char *pszTag, int nLine);
#ifdef __cplusplus
}
#endif

/**
 * Release a block.
 *
 * This is the array release path, which the log line identifies as
 * `del(UNK[],%p)`.
 *
 * @param pBlock The block to release.
 * @ghidraAddress 0x004a92c8
 */
void MemFree(void *pBlock);

/**
 * Release a single object rather than an array.
 *
 * The log line identifies the path as `del(UNK,%p)`. HxStr::Alloc() is the one
 * caller inside the string class.
 *
 * @param pBlock The block to release.
 * @ghidraAddress 0x004a9230
 */
void MemFreeScalar(void *pBlock);

/**
 * Release a block and record the release against a tag.
 *
 * Releasing a block that belongs to a zone is fatal, because a zone hands out
 * bump-pointer slices that cannot be reclaimed individually.
 *
 * @param pBlock The block to release.
 * @param pszTag The tag the allocation was billed to.
 * @param nLine The caller's line number.
 * @ghidraAddress 0x004a94e8
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
 * @ghidraAddress 0x004a86c0
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
 * @ghidraAddress 0x004a93b8
 */
void *MemReallocTagged(void *pBlock, size_t nSize, const char *pszTag, int nLine);

/**
 * Write a marker line into the memory report.
 *
 * Performs no work while logging is off. The frame loop and Heap::DumpStats() are the callers.
 *
 * @param pszText The text to mark.
 * @ghidraAddress 0x004a8e18
 */
void MemLogWrite(const char *pszText);

/**
 * Close the memory report and print a summary of the heap.
 *
 * Closing is skipped when the report was never opened, and otherwise also clears the logging
 * flag. The summary is the ten fields of the C library's `mallinfo()` and, once MemOpenLog() has
 * painted the stack, the stack depth reached. DumpHeapMemoryLog(0) runs last. Fatal() calls this,
 * and MemOpenLog() registers it with atexit().
 *
 * @ghidraAddress 0x004a7d30
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
 * @ghidraAddress 0x004a7c40
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
 * @ghidraAddress 0x004a7ef8
 */
void MemLogCloseAndContinue();

/**
 * Write text to the memory report while logging is on.
 *
 * The image has no caller. The name is inferred.
 *
 * @param pszText The text.
 * @ghidraAddress 0x004a8e50
 */
void MemLogPrint(const char *pszText);

/**
 * Clear the per-tag accounting table and start charging it.
 *
 * Record 0 is titled `Other_Sources`. Rnd::AsyncLoader's poll brackets a load with this and
 * MemEndAccounting(). The name is inferred.
 *
 * @ghidraAddress 0x004a8e88
 */
void MemBeginAccounting();

/**
 * Stop charging the accounting table and format its totals.
 *
 * The report begins `Memory Allocated: %d` and adds one line per titled record. Record 0 appears
 * only when it has been charged. A line that would leave less than 0x40 bytes of the buffer is
 * replaced by `...REPORT TOO LONG FOR BUFFER!` and ends the report. The name is inferred.
 *
 * @param pszReport Receives the report.
 * @param nReportSize The size of the buffer.
 * @return The total bytes charged since MemBeginAccounting().
 * @ghidraAddress 0x004a8ef8
 */
int MemEndAccounting(char *pszReport, int nReportSize);

/**
 * Allocate the per-block tracking table and clear the per-source table.
 *
 * The block table is 32 MB, 0x200000 slots of 16 bytes. The image has no caller, so block
 * tracking never runs in the shipped build. The name is inferred.
 *
 * @ghidraAddress 0x004a95c8
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
 * @ghidraAddress 0x004a87d0
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
 * @ghidraAddress 0x004a8a68
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
 * @ghidraAddress 0x0054b348
 */
void DumpHeapMemoryLog(int nIndex);

/**
 * Take a block from the backing allocator.
 *
 * The allocator underneath is the toolchain's own, whose state lives at 0x007819cc and whose
 * allocate and release routines are at 0x0059b528 and 0x005da3b0. It is not the Heap class in
 * `os/heap.h`, which is the arena the embedded Python allocates from. Both wrappers here are two
 * instructions that load the allocator state and tail-call it.
 *
 * @param nSize The block size in bytes.
 * @return The block, or null when the request cannot be met.
 * @ghidraAddress 0x004bfd48
 */
void *HeapAlloc(size_t nSize);

/**
 * Give a block back to the backing allocator.
 *
 * @param pBlock The block to release.
 * @ghidraAddress 0x004bfd70
 */
void HeapFree(void *pBlock);

/**
 * Resize a block through the backing allocator.
 *
 * The toolchain routine underneath is at 0x00589278. Unlike the other two wrappers this one has no
 * out-of-line copy, because MemReallocTagged() is its only caller and inlines it.
 *
 * @param pBlock The block to resize.
 * @param nSize The new size in bytes.
 * @return The block, which differs from pBlock only when it moved.
 */
void *HeapRealloc(void *pBlock, size_t nSize);

/**
 * Log how much of the backing allocator is free, then give it all back.
 *
 * Blocks of 2048 bytes are taken through HeapAlloc() until a request fails, the count and the
 * total are logged, and every block is released through HeapFree(). The name is inferred. The
 * binary places the routine at the head of the TexturePairRecord unit, and it has no caller.
 *
 * @ghidraAddress 0x00246c18
 */
void ReportHeapCapacity();
