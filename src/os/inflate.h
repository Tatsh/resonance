#pragma once

/** Results the inflate routines report. */
enum InflateResult {
    kInflateOk = 0,          /*!< The data decoded, or huft_build() built a complete table. */
    kInflateError = 1,       /*!< Corrupt data, or from huft_build() an incomplete code set. */
    kInflateBadCodes = 2,    /*!< Over-subscribed code lengths, or an unknown block type. */
    kInflateOutOfMemory = 3, /*!< The table pool is exhausted. */
};

/**
 * One entry of a decoding table that huft_build() creates.
 *
 * The first entry of each table allocation is a link that huft_build() threads the tables of one
 * build through, and the table proper starts after it.
 */
struct Huft {
    unsigned char mExtra; /*!< The extra bit count, 16 plus the subtable bits, 15, 16, or 99. */
    unsigned char mBits;  /*!< The bits this code consumes. */
    union {
        unsigned short mBase; /*!< The literal, or the length or distance base. */
        Huft *mpTable;        /*!< The subtable, or on a link entry the next table. */
    } mValue;                 /*!< The value the code decodes to. */
};

/**
 * Build a decoding table from a list of code lengths.
 *
 * The tables are carved from the fixed pool by HuftAlloc() rather than from the heap.
 *
 * @param pLengths The code lengths, none above 16.
 * @param nCodes The number of code lengths, at least one.
 * @param nSimple The number of codes that decode to their own value.
 * @param pBase The base values of the other codes, indexed from nSimple.
 * @param pExtra The extra bit counts of the other codes, indexed from nSimple.
 * @param ppTable Receives the first table, or null when every length is zero.
 * @param pnLookupBits The preferred lookup bits on entry, and the bits used on return.
 * @return kInflateOk, kInflateError for an incomplete code set that is not a single code,
 *     kInflateBadCodes for over-subscribed lengths, or kInflateOutOfMemory.
 * @ghidraAddress NTSC-U/C: 0x0063c748
 * @ghidraAddress PAL: 0x0067d2d8
 */
int huft_build(const unsigned *pLengths,
               unsigned nCodes,
               unsigned nSimple,
               const unsigned short *pBase,
               const unsigned short *pExtra,
               Huft **ppTable,
               int *pnLookupBits);

/**
 * Decode the literals and matches of one block into the window.
 *
 * @param pLiteralTable The literal and length table.
 * @param pDistanceTable The distance table.
 * @param nLiteralBits The lookup bits of the literal and length table.
 * @param nDistanceBits The lookup bits of the distance table.
 * @return kInflateOk at the end-of-block code, or kInflateError for an invalid code.
 * @ghidraAddress NTSC-U/C: 0x0063cd50
 * @ghidraAddress PAL: 0x0067d8e0
 */
int inflate_codes(Huft *pLiteralTable, Huft *pDistanceTable, int nLiteralBits, int nDistanceBits);

/**
 * Copy a stored block into the window.
 *
 * @return kInflateOk, or kInflateError when the length does not match its complement.
 * @ghidraAddress NTSC-U/C: 0x0063d348
 * @ghidraAddress PAL: 0x0067ded8
 */
int inflate_stored();

/**
 * Decode a block that uses the fixed codes.
 *
 * @return kInflateOk, or the failure of huft_build() or inflate_codes().
 * @ghidraAddress NTSC-U/C: 0x0063d5c8
 * @ghidraAddress PAL: 0x0067e158
 */
int inflate_fixed();

/**
 * Decode a block that includes its own codes.
 *
 * An incomplete literal or distance code set is reported on the standard error stream.
 *
 * @return kInflateOk, or kInflateError for bad counts or corrupt data, or the failure of
 *     huft_build().
 * @ghidraAddress NTSC-U/C: 0x0063d720
 * @ghidraAddress PAL: 0x0067e2b0
 */
int inflate_dynamic();

/**
 * Decode one block of any type.
 *
 * @param pnLast Receives one when this block is the last of the stream.
 * @return kInflateOk, the failure of the block decoder, or kInflateBadCodes for block type 3.
 * @ghidraAddress NTSC-U/C: 0x0063def8
 * @ghidraAddress PAL: 0x0067ea88
 */
int inflate_block(int *pnLast);

/**
 * Decode a whole deflate stream into the window, flushing it to the output.
 *
 * The table pool is reset before each block. On success any whole bytes read ahead into the bit
 * buffer are returned to the staging buffer. The gzip trailer is neither read nor checked.
 *
 * @return kInflateOk, or the failure of inflate_block().
 * @ghidraAddress NTSC-U/C: 0x0063e0b0
 * @ghidraAddress PAL: 0x0067ec40
 */
int inflate();

/**
 * Record the pool high-water mark and rewind the pool to its start.
 *
 * inflate() includes an inlined copy of the body, and this routine is never called.
 *
 * @ghidraAddress NTSC-U/C: 0x0063e1a8
 * @ghidraAddress PAL: 0x0067ed38
 */
void HuftReset();

/**
 * Carve a table from the pool.
 *
 * The cursor is advanced even when the allocation fails, and an allocation that ends exactly at
 * the end of the pool fails. huft_build() includes an inlined copy of the body.
 *
 * @param nEntries The number of entries, including the link entry.
 * @return The table, or null after logging when the pool is exhausted.
 * @ghidraAddress NTSC-U/C: 0x0063e1e0
 * @ghidraAddress PAL: 0x0067ed70
 */
Huft *HuftAlloc(unsigned nEntries);

/**
 * Release a chain of tables. The release does nothing for tables from the pool.
 *
 * @param pTable The first table.
 * @return Zero.
 * @ghidraAddress NTSC-U/C: 0x0063e230
 * @ghidraAddress PAL: 0x0067edc0
 */
int huft_free(Huft *pTable);
