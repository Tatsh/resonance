#pragma once

#include <stddef.h>

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

/**
 * Allocate a block.
 *
 * The request is raised to one byte when it is zero. The allocation is billed
 * to the tag `UNK[]`, and a failure is fatal.
 *
 * @param nSize The block size in bytes.
 * @return The block.
 * @ghidraAddress 0x004a8380
 */
void *MemAlloc(size_t nSize);

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
void *MemAllocTagged(size_t nSize, const char *pszTag, int nLine);

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
void MemFreeTagged(void *pBlock, const char *pszTag, int nLine);

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
