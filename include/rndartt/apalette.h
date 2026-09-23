#pragma once

#include <stddef.h>
#include <vector>

#include "os/mem.h"

struct NormalKey;

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
 * The record is 0x408 bytes, the entry array followed by two words.
 *
 * The constructor, the destructor, and the allocation operator are open-coded at every site that
 * creates or releases a palette. The release operator has two out-of-line emissions, 0x005f9cf8 and
 * 0x0061d4a0, one for each translation unit that needed a copy.
 *
 * The default constructor also runs for the static palette AGifFile keeps, from the static
 * initialiser at 0x0062b5a0, which also runs the destructor at exit.
 */
class APalette {
public:
    /**
     * Allocate a palette under the tag `APalette`.
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     */
    static void *operator new(size_t nSize) {
        return AllocateTaggedMemory(nSize, "APalette");
    }

    /**
     * Release a palette under the tag `APalette`.
     *
     * The second emission of the same body is at 0x0061d4a0.
     *
     * @param pBlock The block.
     * @ghidraAddress 0x005f9cf8
     */
    static void operator delete(void *pBlock) {
        FreeTaggedMemory(pBlock, "APalette");
    }

    /**
     * Construct a palette with no entries written.
     *
     * Clears mpRgb15ToIndex and mEnd and does not touch the entry table.
     * ABitmap::SetPaletteEntries() open-codes it at 0x005eb2d0.
     */
    APalette() : mpRgb15ToIndex(nullptr), mEnd(0) {
    }

    /**
     * Construct a palette from a table of entries.
     *
     * Clears mpRgb15ToIndex and copies the entries in from index zero through SetEntries(), which
     * also writes mEnd. ABmpFile::ReadPalette() open-codes it at 0x0061ce90 and
     * AGifFile::ReadImage() at 0x0062acd8.
     *
     * @param pEntries The entries to copy in.
     * @param nCount The number of entries.
     */
    APalette(const unsigned int *pEntries, int nCount) : mpRgb15ToIndex(nullptr) {
        SetEntries(pEntries, 0, nCount);
    }

    /**
     * Release the inverse lookup table.
     *
     * The release is the single-object path, MemFreeScalar(), rather than the array path. ABmpFile
     * open-codes the destructor at 0x0061c9d0 and 0x0061ca28, testing the member against null
     * before each release.
     */
    ~APalette() {
        delete mpRgb15ToIndex;
    }

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
     * Copy a run of three byte red, green, blue entries into the table with full alpha.
     *
     * Records the index one past the run in mEnd, as SetEntries() does. AGifFile reads its colour
     * tables through it.
     *
     * @param pRGB The entries to copy in, three bytes each in red, green, blue order.
     * @param nFirst The first palette index to write.
     * @param nCount The number of entries to write.
     * @ghidraAddress 0x00613e48
     */
    void SetEntriesRGB(const unsigned char *pRGB, int nFirst, int nCount);

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

    /**
     * Fill the table with a ramp of sixteen shades for every key, from black towards the key's
     * colour.
     *
     * mEnd becomes sixteen times the key count. Shade i of a key is entry 16 times the key's
     * position plus i, with each channel `(unsigned)(channel * t * 255)` at t = i / 17 and alpha
     * 0x80, where a channel is the key's mScale times its ratio. Entry 16 is then set to
     * 0x80000000, entry 0 to zero, and mEnd is raised to 17 when lower. More than sixteen keys
     * write past the table. The quantiser at `0x00557af8` is the one caller. The keys arrive in the
     * first argument register and the palette in the second, so the routine is static rather than
     * an instance method. The name is inferred.
     *
     * @param keys The keys, in the order the quantiser indexes them.
     * @param palette The palette to fill.
     * @ghidraAddress 0x00557970
     */
    static void BuildRampPalette(const std::vector<NormalKey> &keys, APalette &palette);

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
