#pragma once

#include <stddef.h>

/**
 * Allocate a block.
 *
 * @param nSize The block size in bytes.
 * @return The block, or null when the request cannot be met.
 * @ghidraAddress 0x004a8380
 */
void *MemAlloc(size_t nSize);

/**
 * Allocate a block and record the request against a tag.
 *
 * The tag is normally the caller's `__FILE__` or the class name. Totals per tag are accumulated
 * into a 24-entry table for the memory report.
 *
 * @param nSize The block size in bytes.
 * @param pszTag The tag to bill the allocation to.
 * @param nLine The caller's line number.
 * @return The block, or null when the request cannot be met.
 * @ghidraAddress 0x004a8520
 */
void *MemAllocTagged(size_t nSize, const char *pszTag, int nLine);

/**
 * Release a block.
 *
 * @param pBlock The block to release.
 * @ghidraAddress 0x004a92c8
 */
void MemFree(void *pBlock);

/**
 * Release a block and record the release against a tag.
 *
 * Halts when the block does not belong to any zone.
 *
 * @param pBlock The block to release.
 * @param pszTag The tag the allocation was billed to.
 * @param nLine The caller's line number.
 * @ghidraAddress 0x004a94e8
 */
void MemFreeTagged(void *pBlock, const char *pszTag, int nLine);
