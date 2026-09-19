#pragma once

/**
 * Indexed palette of a bitmap.
 *
 * The layout is recovered from the accesses `Rnd::PsTex::RebuildClut()` and
 * `Rnd::PsTex::RestoreSurfaces()` make, not from any type information in the image, and the
 * entry count is the only field whose purpose is settled. A palette is embedded in its bitmap
 * rather than referenced, which is why the run at `ABitmap` `+0x18` is exactly this size.
 */
class APalette {
public:
    /** Colour entries, as the GS consumes them. */
    unsigned mEntries[256];
    /** Purpose undetermined. */
    int mUnknown400;
    /** Number of entries the bitmap actually uses. */
    int mEntryCount;
};

/**
 * Bitmap as the art library hands it over.
 *
 * Only the embedded palette is recovered. Everything before it is reached by offset from routines
 * that are not yet reconstructed, so the leading members are placeholders and the total size is
 * unrecovered. Do not treat the gaps as settled.
 */
class ABitmap {
public:
    int mUnknown00;
    int mUnknown04;
    int mUnknown08;
    int mUnknown0c;
    int mUnknown10;
    int mUnknown14;
    /** Palette every mip level of the bitmap shares. Confirmed at `+0x18`. */
    APalette mPalette;
};
