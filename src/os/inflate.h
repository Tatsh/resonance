#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** Results that inflate() and the routines under it report. */
enum InflateResult {
    kInflateOk = 0,          /*!< The data decoded. */
    kInflateError = 1,       /*!< Corrupt data, or an incomplete code set. */
    kInflateBadCodes = 2,    /*!< Over-subscribed code lengths, or an unknown block type. */
    kInflateOutOfMemory = 3, /*!< The table pool is exhausted. */
};

struct huft;

/**
 * Decode a whole deflate stream into the window, flushing it to the output.
 *
 * The table pool is rewound before each block. On success any whole bytes read ahead into the bit
 * buffer are returned to the staging buffer. The gzip trailer is neither read nor checked.
 *
 * @return kInflateOk, or the failure of the block decoder.
 * @ghidraAddress NTSC-U/C: 0x0063e0b0
 * @ghidraAddress PAL: 0x0067ec40
 */
int inflate(void);

/**
 * Record the pool high-water mark and rewind the pool to its start.
 *
 * inflate() calls it before each block.
 *
 * @ghidraAddress NTSC-U/C: 0x0063e1a8
 * @ghidraAddress PAL: 0x0067ed38
 */
void HuftReset(void);

/**
 * Carve a decoding table from the pool, in place of the heap.
 *
 * The cursor is advanced even when the allocation fails, and an allocation that ends exactly at
 * the end of the pool fails.
 *
 * @param nEntries The number of entries, including the link entry.
 * @return The table, or null after logging when the pool is exhausted.
 * @ghidraAddress NTSC-U/C: 0x0063e1e0
 * @ghidraAddress PAL: 0x0067ed70
 */
struct huft *HuftAlloc(unsigned nEntries);

#ifdef __cplusplus
}
#endif
