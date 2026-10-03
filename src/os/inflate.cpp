#include "os/inflate.h"

#include <stdio.h>
#include <string.h>

#include "os/loadfile.h"
#include "os/log.h"

namespace {

// The longest code length a deflate stream can describe.
constexpr unsigned kMaxCodeBits = 16;

// The largest code count huft_build() is given, the fixed literal and length count.
constexpr unsigned kMaxCodes = 288;

// The number of table entries the pool provides.
constexpr int kHuftPoolSize = 2048;

// The extra bit counts above this value mark a subtable of that many bits more.
constexpr unsigned kSubtableOffset = 16;

// The extra bit count of a literal.
constexpr unsigned kCodeLiteral = 16;

// The extra bit count of the end-of-block code.
constexpr unsigned kCodeEndOfBlock = 15;

// The extra bit count of a code with no value.
constexpr unsigned kCodeInvalid = 99;

// The first value that is not a literal, the end-of-block code.
constexpr unsigned kEndOfBlockValue = 256;

// The codes below this value decode to themselves in the literal and length alphabet.
constexpr unsigned kLiteralSimpleCodes = 257;

// The block types a block header encodes.
enum BlockType {
    kBlockStored = 0,
    kBlockFixed = 1,
    kBlockDynamic = 2,
};

// The fields of a block header.
constexpr unsigned kLastBlockBits = 1;
constexpr unsigned kLastBlockMask = 1;
constexpr unsigned kBlockTypeBits = 2;
constexpr unsigned kBlockTypeMask = 3;

// A stored block length and its complement, each one little-endian half word.
constexpr unsigned kStoredLengthBits = 16;
constexpr unsigned kStoredLengthMask = 0xffff;

// The width of one byte in the bit buffer.
constexpr unsigned kByteBits = 8;

// The alignment a stored block restores by discarding bits.
constexpr unsigned kByteBitsMask = 7;

// The fixed code lengths, by range of the literal and length alphabet.
constexpr int kFixedLiteralCodes = 288;
constexpr int kFixedLength8End = 144;
constexpr int kFixedLength9End = 256;
constexpr int kFixedLength7End = 280;
constexpr unsigned kFixedLength7 = 7;
constexpr unsigned kFixedLength8 = 8;
constexpr unsigned kFixedLength9 = 9;
constexpr int kFixedLiteralLookupBits = 7;

// The fixed distance code lengths.
constexpr int kFixedDistanceCodes = 30;
constexpr unsigned kFixedDistanceLength = 5;
constexpr int kFixedDistanceLookupBits = 5;

// The fields of a dynamic block header.
constexpr unsigned kLiteralCountBits = 5;
constexpr unsigned kLiteralCountMask = 0x1f;
constexpr unsigned kDistanceCountBits = 5;
constexpr unsigned kDistanceCountMask = 0x1f;
constexpr unsigned kDistanceCountBase = 1;
constexpr unsigned kLengthCountBits = 4;
constexpr unsigned kLengthCountMask = 0xf;
constexpr unsigned kLengthCountBase = 4;
constexpr unsigned kMaxLiteralCodes = 286;
constexpr unsigned kMaxDistanceCodes = 30;

// The code length code alphabet and the three bits each of its lengths takes.
constexpr unsigned kLengthCodes = 19;
constexpr unsigned kLengthCodeBits = 3;
constexpr unsigned kLengthCodeMask = 7;
constexpr int kLengthCodeLookupBits = 7;

// The code length codes above 15 and their repeat fields.
constexpr unsigned kMaxLiteralLength = 16;
constexpr unsigned kRepeatPrevious = 16;
constexpr unsigned kRepeatPreviousBits = 2;
constexpr unsigned kRepeatPreviousMask = 3;
constexpr unsigned kRepeatPreviousBase = 3;
constexpr unsigned kRepeatZeroShort = 17;
constexpr unsigned kRepeatZeroShortBits = 3;
constexpr unsigned kRepeatZeroShortMask = 7;
constexpr unsigned kRepeatZeroShortBase = 3;
constexpr unsigned kRepeatZeroLongBits = 7;
constexpr unsigned kRepeatZeroLongMask = 0x7f;
constexpr unsigned kRepeatZeroLongBase = 11;

// The lookup bits a dynamic block requests from huft_build(), lbits and dbits in the image.
constexpr int kLiteralLookupBits = 9;
constexpr int kDistanceLookupBits = 6;

// NTSC-U/C: 0x007c3a98, PAL: 0x00807798, border: the order the code length code lengths arrive in.
constexpr unsigned kLengthCodeOrder[] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15,
};

// NTSC-U/C: 0x007c3ae8, PAL: 0x008077e8, cplens: the length bases, with two unused codes
// padded with zero.
constexpr unsigned short kLengthBase[] = {
    3,  4,  5,  6,  7,  8,  9,  10,  11,  13,  15,  17,  19,  23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0,  0,
};

// NTSC-U/C: 0x007c3b28, PAL: 0x00807828, cplext: the length extra bits, with the two unused
// codes marked invalid.
constexpr unsigned short kLengthExtraBits[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 99, 99,
};

// NTSC-U/C: 0x007c3b68, PAL: 0x00807868, cpdist: the distance bases.
constexpr unsigned short kDistanceBase[] = {
    1,   2,   3,   4,   5,   7,    9,    13,   17,   25,   33,   49,   65,    97,    129,
    193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577,
};

// NTSC-U/C: 0x007c3ba8, PAL: 0x008078a8, cpdext: the distance extra bits.
constexpr unsigned short kDistanceExtraBits[] = {
    0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
    6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13,
};

// NTSC-U/C: 0x007c3be8, PAL: 0x008078e8, mask_bits: the mask of each bit count up to 16.
constexpr unsigned short kMaskBits[] = {
    0x0000,
    0x0001,
    0x0003,
    0x0007,
    0x000f,
    0x001f,
    0x003f,
    0x007f,
    0x00ff,
    0x01ff,
    0x03ff,
    0x07ff,
    0x0fff,
    0x1fff,
    0x3fff,
    0x7fff,
    0xffff,
};

// Reports a pool too small for a table.
constexpr char kHuftMemoryExceeded[] = "HUFT MEMORY EXCEEDED!!\n";

// Reports an incomplete code set in a dynamic block.
constexpr char kIncompleteLiteralTree[] = " incomplete literal tree\n";
constexpr char kIncompleteDistanceTree[] = " incomplete distance tree\n";

// Loads bytes into the bit buffer until it has at least the requested bit count.
inline void NeedBits(unsigned long long &llBits, unsigned &nBitCount, unsigned nWanted) {
    while (nBitCount < nWanted) {
        llBits |= static_cast<unsigned long long>(static_cast<unsigned char>(GzipGetByte()))
                  << nBitCount;
        nBitCount += kByteBits;
    }
}

// Discards consumed bits from the bit buffer.
inline void DumpBits(unsigned long long &llBits, unsigned &nBitCount, unsigned nUsed) {
    llBits >>= nUsed;
    nBitCount -= nUsed;
}

// Looks up one code through a table and its subtables, reporting null for an invalid code. The
// caller has already loaded the table's lookup bits, and the returned entry's bits remain.
inline const Huft *
DecodeCode(const Huft *pTable, unsigned nMask, unsigned long long &llBits, unsigned &nBitCount) {
    const Huft *pEntry = pTable + (static_cast<unsigned>(llBits) & nMask);
    unsigned nExtra = pEntry->mExtra;
    if (nExtra > kSubtableOffset) {
        do {
            if (nExtra == kCodeInvalid) {
                return nullptr;
            }
            DumpBits(llBits, nBitCount, pEntry->mBits);
            nExtra -= kSubtableOffset;
            NeedBits(llBits, nBitCount, nExtra);
            pEntry = pEntry->mValue.mpTable + (static_cast<unsigned>(llBits) & kMaskBits[nExtra]);
        } while ((nExtra = pEntry->mExtra) > kSubtableOffset);
    }
    return pEntry;
}

// Flushes the window once the write position arrives at its end.
inline void FlushWindowIfFull(unsigned &nWindow) {
    if (nWindow == kGzipWindowSize) {
        g_nGzipWindowPosition = nWindow;
        GzipFlushWindow();
        nWindow = 0;
    }
}

} // namespace

// NTSC-U/C: 0x008ee930, PAL: 0x00933930
// The bit buffer, bb.
unsigned long long g_llGzipBitBuffer = 0;

// NTSC-U/C: 0x008ee938, PAL: 0x00933938
// The number of valid bits in the bit buffer, bk.
unsigned g_nGzipBitCount = 0;

// NTSC-U/C: 0x008ee93c, PAL: 0x0093393c
// The table entries the current block allocated, hufts. Only huft_build() reads the count.
unsigned g_nGzipHuftCount = 0;

// NTSC-U/C: 0x008ea930, PAL: 0x0092f930
// The pool the decoding tables are carved from.
Huft g_aGzipHuftPool[kHuftPoolSize] = {};

// NTSC-U/C: 0x007c3a90, PAL: 0x00807790
// The next free entry of the pool.
Huft *g_pGzipHuftPoolNext = g_aGzipHuftPool;

// NTSC-U/C: 0x007c3a94, PAL: 0x00807794
// The most pool entries one block has used. Only HuftReset() reads the count.
int g_nGzipHuftPeak = 0;

// NTSC-U/C: 0x0063c748, PAL: 0x0067d2d8
int huft_build(const unsigned *pLengths,
               unsigned nCodes,
               unsigned nSimple,
               const unsigned short *pBase,
               const unsigned short *pExtra,
               Huft **ppTable,
               int *pnLookupBits) {
    unsigned aCount[kMaxCodeBits + 1];
    Huft *apStack[kMaxCodeBits];
    unsigned aValues[kMaxCodes];
    unsigned aOffsets[kMaxCodeBits + 1];
    Huft entry{};

    memset(aCount, 0, sizeof(aCount));
    const unsigned *pLength = pLengths;
    unsigned i = nCodes;
    do {
        ++aCount[*pLength];
        ++pLength;
    } while (--i);
    if (aCount[0] == nCodes) {
        *ppTable = nullptr;
        *pnLookupBits = 0;
        return kInflateOk;
    }

    int nLookupBits = *pnLookupBits;
    unsigned j;
    for (j = 1; j <= kMaxCodeBits; ++j) {
        if (aCount[j] != 0) {
            break;
        }
    }
    int nCodeBits = static_cast<int>(j);
    if (static_cast<unsigned>(nLookupBits) < j) {
        nLookupBits = static_cast<int>(j);
    }
    for (i = kMaxCodeBits; i != 0; --i) {
        if (aCount[i] != 0) {
            break;
        }
    }
    const int nMaxCodeBits = static_cast<int>(i);
    if (static_cast<unsigned>(nLookupBits) > i) {
        nLookupBits = static_cast<int>(i);
    }
    *pnLookupBits = nLookupBits;

    // The unused code patterns of the longest length. A complete set has none.
    int nUnused = 1 << j;
    for (; j < i; ++j, nUnused <<= 1) {
        if ((nUnused -= static_cast<int>(aCount[j])) < 0) {
            return kInflateBadCodes;
        }
    }
    if ((nUnused -= static_cast<int>(aCount[i])) < 0) {
        return kInflateBadCodes;
    }
    aCount[i] += static_cast<unsigned>(nUnused);

    aOffsets[1] = j = 0;
    const unsigned *pCount = aCount + 1;
    unsigned *pOffset = aOffsets + 2;
    while (--i) {
        *pOffset++ = (j += *pCount++);
    }

    pLength = pLengths;
    i = 0;
    do {
        if ((j = *pLength++) != 0) {
            aValues[aOffsets[j]++] = i;
        }
    } while (++i < nCodes);

    aOffsets[0] = i = 0;
    const unsigned *pValue = aValues;
    int nLevel = -1;
    int nTableBits = -nLookupBits;
    apStack[0] = nullptr;
    Huft *pTable = nullptr;
    unsigned nEntries = 0;

    for (; nCodeBits <= nMaxCodeBits; ++nCodeBits) {
        unsigned nLeft = aCount[nCodeBits];
        while (nLeft--) {
            while (nCodeBits > nTableBits + nLookupBits) {
                ++nLevel;
                nTableBits += nLookupBits;

                nEntries = static_cast<unsigned>(nMaxCodeBits - nTableBits);
                if (nEntries > static_cast<unsigned>(nLookupBits)) {
                    nEntries = static_cast<unsigned>(nLookupBits);
                }
                unsigned nPatterns = 1U << (j = static_cast<unsigned>(nCodeBits - nTableBits));
                if (nPatterns > nLeft + 1) {
                    nPatterns -= nLeft + 1;
                    const unsigned *pLongerCount = aCount + nCodeBits;
                    while (++j < nEntries) {
                        if ((nPatterns <<= 1) <= *++pLongerCount) {
                            break;
                        }
                        nPatterns -= *pLongerCount;
                    }
                }
                nEntries = 1U << j;

                if ((pTable = HuftAlloc(nEntries + 1)) == nullptr) {
                    if (nLevel != 0) {
                        huft_free(apStack[0]);
                    }
                    return kInflateOutOfMemory;
                }
                g_nGzipHuftCount += nEntries + 1;
                *ppTable = pTable + 1;
                *(ppTable = &pTable->mValue.mpTable) = nullptr;
                apStack[nLevel] = ++pTable;

                if (nLevel != 0) {
                    aOffsets[nLevel] = i;
                    entry.mBits = static_cast<unsigned char>(nLookupBits);
                    entry.mExtra = static_cast<unsigned char>(kSubtableOffset + j);
                    entry.mValue.mpTable = pTable;
                    j = i >> (nTableBits - nLookupBits);
                    apStack[nLevel - 1][j] = entry;
                }
            }

            // An invalid entry reuses the previous entry's value.
            entry.mBits = static_cast<unsigned char>(nCodeBits - nTableBits);
            if (pValue >= aValues + nCodes) {
                entry.mExtra = kCodeInvalid;
            } else if (*pValue < nSimple) {
                entry.mExtra = (*pValue < kEndOfBlockValue) ? kCodeLiteral : kCodeEndOfBlock;
                entry.mValue.mBase = static_cast<unsigned short>(*pValue);
                ++pValue;
            } else {
                entry.mExtra = static_cast<unsigned char>(pExtra[*pValue - nSimple]);
                entry.mValue.mBase = pBase[*pValue++ - nSimple];
            }

            const unsigned nStride = 1U << (nCodeBits - nTableBits);
            for (j = i >> nTableBits; j < nEntries; j += nStride) {
                pTable[j] = entry;
            }

            // The codes are bit-reversed. The next code is the current code incremented from its
            // top bit.
            for (j = 1U << (nCodeBits - 1); (i & j) != 0; j >>= 1) {
                i ^= j;
            }
            i ^= j;

            while ((i & ((1U << nTableBits) - 1)) != aOffsets[nLevel]) {
                --nLevel;
                nTableBits -= nLookupBits;
            }
        }
    }

    return (nUnused != 0 && nMaxCodeBits != 1) ? kInflateError : kInflateOk;
}

// NTSC-U/C: 0x0063cd50, PAL: 0x0067d8e0
int inflate_codes(Huft *pLiteralTable, Huft *pDistanceTable, int nLiteralBits, int nDistanceBits) {
    unsigned long long llBits = g_llGzipBitBuffer;
    unsigned nBitCount = g_nGzipBitCount;
    unsigned nWindow = g_nGzipWindowPosition;
    const unsigned nLiteralMask = kMaskBits[nLiteralBits];
    const unsigned nDistanceMask = kMaskBits[nDistanceBits];

    for (;;) {
        NeedBits(llBits, nBitCount, static_cast<unsigned>(nLiteralBits));
        const Huft *pEntry = DecodeCode(pLiteralTable, nLiteralMask, llBits, nBitCount);
        if (pEntry == nullptr) {
            return kInflateError;
        }
        DumpBits(llBits, nBitCount, pEntry->mBits);
        unsigned nExtra = pEntry->mExtra;

        if (nExtra == kCodeLiteral) {
            g_bGzipWindow[nWindow++] = static_cast<unsigned char>(pEntry->mValue.mBase);
            FlushWindowIfFull(nWindow);
            continue;
        }
        if (nExtra == kCodeEndOfBlock) {
            break;
        }

        NeedBits(llBits, nBitCount, nExtra);
        unsigned nLength =
            pEntry->mValue.mBase + (static_cast<unsigned>(llBits) & kMaskBits[nExtra]);
        DumpBits(llBits, nBitCount, nExtra);

        NeedBits(llBits, nBitCount, static_cast<unsigned>(nDistanceBits));
        pEntry = DecodeCode(pDistanceTable, nDistanceMask, llBits, nBitCount);
        if (pEntry == nullptr) {
            return kInflateError;
        }
        DumpBits(llBits, nBitCount, pEntry->mBits);
        nExtra = pEntry->mExtra;
        NeedBits(llBits, nBitCount, nExtra);
        unsigned nDistance =
            nWindow - pEntry->mValue.mBase - (static_cast<unsigned>(llBits) & kMaskBits[nExtra]);
        DumpBits(llBits, nBitCount, nExtra);

        do {
            nDistance &= kGzipWindowSize - 1;
            unsigned nCopy = kGzipWindowSize - ((nDistance > nWindow) ? nDistance : nWindow);
            if (nCopy > nLength) {
                nCopy = nLength;
            }
            nLength -= nCopy;
            if (nWindow - nDistance >= nCopy) {
                memcpy(g_bGzipWindow + nWindow, g_bGzipWindow + nDistance, nCopy);
                nWindow += nCopy;
                nDistance += nCopy;
            } else {
                do {
                    g_bGzipWindow[nWindow++] = g_bGzipWindow[nDistance++];
                } while (--nCopy);
            }
            FlushWindowIfFull(nWindow);
        } while (nLength != 0);
    }

    g_nGzipWindowPosition = nWindow;
    g_llGzipBitBuffer = llBits;
    g_nGzipBitCount = nBitCount;
    return kInflateOk;
}

// NTSC-U/C: 0x0063d348, PAL: 0x0067ded8
int inflate_stored() {
    unsigned long long llBits = g_llGzipBitBuffer;
    unsigned nBitCount = g_nGzipBitCount;
    unsigned nWindow = g_nGzipWindowPosition;

    DumpBits(llBits, nBitCount, nBitCount & kByteBitsMask);

    NeedBits(llBits, nBitCount, kStoredLengthBits);
    unsigned nLength = static_cast<unsigned>(llBits) & kStoredLengthMask;
    DumpBits(llBits, nBitCount, kStoredLengthBits);
    NeedBits(llBits, nBitCount, kStoredLengthBits);
    if (nLength != (static_cast<unsigned>(~llBits) & kStoredLengthMask)) {
        return kInflateError;
    }
    DumpBits(llBits, nBitCount, kStoredLengthBits);

    while (nLength--) {
        NeedBits(llBits, nBitCount, kByteBits);
        g_bGzipWindow[nWindow++] = static_cast<unsigned char>(llBits);
        FlushWindowIfFull(nWindow);
        DumpBits(llBits, nBitCount, kByteBits);
    }

    g_nGzipWindowPosition = nWindow;
    g_llGzipBitBuffer = llBits;
    g_nGzipBitCount = nBitCount;
    return kInflateOk;
}

// NTSC-U/C: 0x0063d5c8, PAL: 0x0067e158
int inflate_fixed() {
    unsigned aLengths[kFixedLiteralCodes];

    int i;
    for (i = 0; i < kFixedLength8End; ++i) {
        aLengths[i] = kFixedLength8;
    }
    for (; i < kFixedLength9End; ++i) {
        aLengths[i] = kFixedLength9;
    }
    for (; i < kFixedLength7End; ++i) {
        aLengths[i] = kFixedLength7;
    }
    // The two unused codes make the set complete.
    for (; i < kFixedLiteralCodes; ++i) {
        aLengths[i] = kFixedLength8;
    }
    Huft *pLiteralTable;
    int nLiteralBits = kFixedLiteralLookupBits;
    if ((i = huft_build(aLengths,
                        kFixedLiteralCodes,
                        kLiteralSimpleCodes,
                        kLengthBase,
                        kLengthExtraBits,
                        &pLiteralTable,
                        &nLiteralBits)) != kInflateOk) {
        return i;
    }

    // The distance set is deliberately incomplete, and only a result above kInflateError fails.
    for (i = 0; i < kFixedDistanceCodes; ++i) {
        aLengths[i] = kFixedDistanceLength;
    }
    Huft *pDistanceTable;
    int nDistanceBits = kFixedDistanceLookupBits;
    if ((i = huft_build(aLengths,
                        kFixedDistanceCodes,
                        0,
                        kDistanceBase,
                        kDistanceExtraBits,
                        &pDistanceTable,
                        &nDistanceBits)) > kInflateError) {
        huft_free(pLiteralTable);
        return i;
    }

    if (inflate_codes(pLiteralTable, pDistanceTable, nLiteralBits, nDistanceBits) != kInflateOk) {
        return kInflateError;
    }

    huft_free(pLiteralTable);
    huft_free(pDistanceTable);
    return kInflateOk;
}

// NTSC-U/C: 0x0063d720, PAL: 0x0067e2b0
int inflate_dynamic() {
    unsigned aLengths[kMaxLiteralCodes + kMaxDistanceCodes];
    unsigned long long llBits = g_llGzipBitBuffer;
    unsigned nBitCount = g_nGzipBitCount;

    NeedBits(llBits, nBitCount, kLiteralCountBits);
    const unsigned nLiteralCodes =
        kLiteralSimpleCodes + (static_cast<unsigned>(llBits) & kLiteralCountMask);
    DumpBits(llBits, nBitCount, kLiteralCountBits);
    NeedBits(llBits, nBitCount, kDistanceCountBits);
    const unsigned nDistanceCodes =
        kDistanceCountBase + (static_cast<unsigned>(llBits) & kDistanceCountMask);
    DumpBits(llBits, nBitCount, kDistanceCountBits);
    NeedBits(llBits, nBitCount, kLengthCountBits);
    const unsigned nLengthCodes =
        kLengthCountBase + (static_cast<unsigned>(llBits) & kLengthCountMask);
    DumpBits(llBits, nBitCount, kLengthCountBits);
    if (nLiteralCodes > kMaxLiteralCodes || nDistanceCodes > kMaxDistanceCodes) {
        return kInflateError;
    }

    unsigned j;
    for (j = 0; j < nLengthCodes; ++j) {
        NeedBits(llBits, nBitCount, kLengthCodeBits);
        aLengths[kLengthCodeOrder[j]] = static_cast<unsigned>(llBits) & kLengthCodeMask;
        DumpBits(llBits, nBitCount, kLengthCodeBits);
    }
    for (; j < kLengthCodes; ++j) {
        aLengths[kLengthCodeOrder[j]] = 0;
    }

    Huft *pLiteralTable;
    int nLiteralBits = kLengthCodeLookupBits;
    int i = huft_build(
        aLengths, kLengthCodes, kLengthCodes, nullptr, nullptr, &pLiteralTable, &nLiteralBits);
    if (i != kInflateOk) {
        if (i == kInflateError) {
            huft_free(pLiteralTable);
        }
        return i;
    }

    // The image does not reject a null table built from all zero lengths.
    const unsigned nCodes = nLiteralCodes + nDistanceCodes;
    const unsigned nMask = kMaskBits[nLiteralBits];
    unsigned nLast = 0;
    Huft *pDistanceTable;
    for (i = 0; static_cast<unsigned>(i) < nCodes;) {
        NeedBits(llBits, nBitCount, static_cast<unsigned>(nLiteralBits));
        pDistanceTable = pLiteralTable + (static_cast<unsigned>(llBits) & nMask);
        j = pDistanceTable->mBits;
        DumpBits(llBits, nBitCount, j);
        j = pDistanceTable->mValue.mBase;
        if (j < kMaxLiteralLength) {
            aLengths[i++] = nLast = j;
        } else if (j == kRepeatPrevious) {
            NeedBits(llBits, nBitCount, kRepeatPreviousBits);
            j = kRepeatPreviousBase + (static_cast<unsigned>(llBits) & kRepeatPreviousMask);
            DumpBits(llBits, nBitCount, kRepeatPreviousBits);
            if (static_cast<unsigned>(i) + j > nCodes) {
                return kInflateError;
            }
            while (j--) {
                aLengths[i++] = nLast;
            }
        } else if (j == kRepeatZeroShort) {
            NeedBits(llBits, nBitCount, kRepeatZeroShortBits);
            j = kRepeatZeroShortBase + (static_cast<unsigned>(llBits) & kRepeatZeroShortMask);
            DumpBits(llBits, nBitCount, kRepeatZeroShortBits);
            if (static_cast<unsigned>(i) + j > nCodes) {
                return kInflateError;
            }
            while (j--) {
                aLengths[i++] = 0;
            }
            nLast = 0;
        } else {
            NeedBits(llBits, nBitCount, kRepeatZeroLongBits);
            j = kRepeatZeroLongBase + (static_cast<unsigned>(llBits) & kRepeatZeroLongMask);
            DumpBits(llBits, nBitCount, kRepeatZeroLongBits);
            if (static_cast<unsigned>(i) + j > nCodes) {
                return kInflateError;
            }
            while (j--) {
                aLengths[i++] = 0;
            }
            nLast = 0;
        }
    }

    huft_free(pLiteralTable);

    g_llGzipBitBuffer = llBits;
    g_nGzipBitCount = nBitCount;

    nLiteralBits = kLiteralLookupBits;
    if ((i = huft_build(aLengths,
                        nLiteralCodes,
                        kLiteralSimpleCodes,
                        kLengthBase,
                        kLengthExtraBits,
                        &pLiteralTable,
                        &nLiteralBits)) != kInflateOk) {
        if (i == kInflateError) {
            fprintf(stderr, kIncompleteLiteralTree);
            huft_free(pLiteralTable);
        }
        return i;
    }
    int nDistanceBits = kDistanceLookupBits;
    if ((i = huft_build(aLengths + nLiteralCodes,
                        nDistanceCodes,
                        0,
                        kDistanceBase,
                        kDistanceExtraBits,
                        &pDistanceTable,
                        &nDistanceBits)) != kInflateOk) {
        if (i == kInflateError) {
            fprintf(stderr, kIncompleteDistanceTree);
            huft_free(pDistanceTable);
        }
        huft_free(pLiteralTable);
        return i;
    }

    if (inflate_codes(pLiteralTable, pDistanceTable, nLiteralBits, nDistanceBits) != kInflateOk) {
        return kInflateError;
    }

    huft_free(pLiteralTable);
    huft_free(pDistanceTable);
    return kInflateOk;
}

// NTSC-U/C: 0x0063def8, PAL: 0x0067ea88
int inflate_block(int *pnLast) {
    unsigned long long llBits = g_llGzipBitBuffer;
    unsigned nBitCount = g_nGzipBitCount;

    NeedBits(llBits, nBitCount, kLastBlockBits);
    *pnLast = static_cast<int>(llBits & kLastBlockMask);
    DumpBits(llBits, nBitCount, kLastBlockBits);

    NeedBits(llBits, nBitCount, kBlockTypeBits);
    const unsigned nType = static_cast<unsigned>(llBits) & kBlockTypeMask;
    DumpBits(llBits, nBitCount, kBlockTypeBits);

    g_llGzipBitBuffer = llBits;
    g_nGzipBitCount = nBitCount;

    if (nType == kBlockDynamic) {
        return inflate_dynamic();
    }
    if (nType == kBlockStored) {
        return inflate_stored();
    }
    if (nType == kBlockFixed) {
        return inflate_fixed();
    }
    return kInflateBadCodes;
}

// NTSC-U/C: 0x0063e0b0, PAL: 0x0067ec40
int inflate() {
    g_nGzipWindowPosition = 0;
    g_nGzipBitCount = 0;
    g_llGzipBitBuffer = 0;

    int nLast;
    do {
        HuftReset();
        g_nGzipHuftCount = 0;
        const int nResult = inflate_block(&nLast);
        if (nResult != kInflateOk) {
            return nResult;
        }
    } while (nLast == 0);

    while (g_nGzipBitCount >= kByteBits) {
        g_nGzipBitCount -= kByteBits;
        --g_nGzipInputPosition;
    }

    GzipFlushWindow();
    return kInflateOk;
}

// NTSC-U/C: 0x0063e1a8, PAL: 0x0067ed38
void HuftReset() {
    const int nUsed = static_cast<int>(g_pGzipHuftPoolNext - g_aGzipHuftPool);
    if (g_nGzipHuftPeak < nUsed) {
        g_nGzipHuftPeak = nUsed;
    }
    g_pGzipHuftPoolNext = g_aGzipHuftPool;
}

// NTSC-U/C: 0x0063e1e0, PAL: 0x0067ed70
Huft *HuftAlloc(unsigned nEntries) {
    Huft *pTable = g_pGzipHuftPoolNext;
    g_pGzipHuftPoolNext = pTable + nEntries;
    if (g_pGzipHuftPoolNext < g_aGzipHuftPool + kHuftPoolSize) {
        return pTable;
    }
    LogPrintf(kHuftMemoryExceeded);
    return nullptr;
}

// NTSC-U/C: 0x0063e230, PAL: 0x0067edc0
int huft_free(Huft *pTable) {
    (void)pTable;
    return 0;
}
