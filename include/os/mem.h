#pragma once

#include <stddef.h>

#ifdef __cplusplus
/**
 * The number of per-tag accounting records.
 *
 * Record 0 is the overflow bucket. Its name is never written, and it receives
 * the byte count of every allocation whose tag did not fit in records 1
 * through 23.
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
 * Close the memory report and print a summary of it.
 *
 * Closing is skipped when the report was never opened. The routine then clears the logging flag,
 * formats a summary, and writes it through LogPrintf(). Fatal() is the only caller, which makes
 * this the last thing the machine does with its allocation record before it stops.
 *
 * Only the closing half is recovered. The summary builds its text by scanning for a character and
 * differencing two pointers, and neither the format string nor the quantity it reports has been
 * determined, so the body is not reconstructed.
 *
 * @ghidraAddress 0x004a7d30
 */
void MemCloseLogAndReport();

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
