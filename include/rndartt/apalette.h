#pragma once

/** Entries a palette stores, one for every value an eight bit index can take. */
constexpr int kAPaletteEntryCount = 256;

/** Entries in the inverse lookup table, one for every 1555 colour with the alpha bit cleared. */
constexpr int kAPaletteInverseEntryCount = 0x8000;

/**
 * A 256 entry colour table an ABitmap points at.
 *
 * The class is not polymorphic and has no RTTI. Its name comes from the allocation tag its blocks
 * are billed to, "APalette". The tagged allocator records that tag verbatim.
 *
 * The record is 0x408 bytes, the entry array followed by two words. The allocation site clears
 * both words, and SetEntries() writes one of them.
 *
 * The class also has a tagged `operator new` and `operator delete` pair, the release half at
 * 0x005f9cf8. The allocation macro every class in this tree uses generates both rather than a
 * programmer writing them out. Neither is declared here.
 */
class APalette {
public:
    /**
     * Copy a run of entries into the table.
     *
     * Records the index one past the run in mEnd. A previous run's value is overwritten rather
     * than retained as a high water mark.
     *
     * @param pEntries The entries to copy in.
     * @param nFirst The first palette index to write.
     * @param nCount The number of entries to write.
     * @ghidraAddress 0x00613df8
     */
    void SetEntries(const unsigned int *pEntries, int nFirst, int nCount);

    /**
     * Search an inclusive index range for the entry nearest a colour.
     *
     * Compares by the sum of the squared differences of the red, green, and blue bytes. The first
     * entry of the range is the result until a strictly closer entry appears, and the alpha byte
     * takes no part.
     *
     * The initial best distance is 0x30000, which exceeds the largest possible sum of three
     * squared byte differences, 0x2fa43. The result is truncated to eight bits.
     *
     * @param nColor The colour to match, packed as 8888.
     * @param nFirst The first index to consider.
     * @param nLast The last index to consider.
     * @return The nearest index, or zero when the range is empty.
     * @ghidraAddress 0x00613f10
     */
    int FindNearestEntry(unsigned int nColor, int nFirst, int nLast) const;

    unsigned int mEntries[kAPaletteEntryCount]; /*!< The colour table. +0x000 */
    /**
     * Table of one palette index per 1555 colour, null until a caller supplies one.
     *
     * ACanvas8::SetColor15() indexes it with the 1555 colour and the alpha bit cleared, and
     * ACanvas32::GetColorIndex() indexes it with the pen colour packed the same way. Both fall
     * back to FindNearestEntry() over the whole table when the member is null, so the table is a
     * cache of that search rather than a required part of the palette. The entry count follows
     * the widest index either routine forms.
     *
     * Nothing in the image was found to allocate or fill the table. +0x400
     */
    unsigned char *mpRgb15ToIndex;
    int mEnd; /*!< One past the last entry written. +0x404 */
};
