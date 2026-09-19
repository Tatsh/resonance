#pragma once

/** Entries a palette stores, one for every value an eight bit index can take. */
constexpr int kAPaletteEntryCount = 256;

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

    unsigned int mEntries[kAPaletteEntryCount]; /*!< The colour table. +0x000 */
    int mUnknown400;                            /*!< Purpose undetermined. +0x400 */
    int mEnd;                                   /*!< One past the last entry written. +0x404 */
};
