#pragma once

#include "rndartt/apalette.h"

/**
 * Pixel layout code stored in bits 8 through 11 of ABitmap::mFlags.
 *
 * ACanvas::CreateForBitmap() accepts only codes 0 through 5 and rejects every larger code. Each
 * accepted code selects one of the five linear canvas subclasses. The mapping was recovered from
 * the jump table at 0x00837d90 together with the type function each branch installs.
 */
enum ABitmapFormat {
    kABitmapFormatLinear4 = 0,  /*!< Four bits per pixel, drawn by ACanvasLin4. */
    kABitmapFormatLinear8 = 1,  /*!< One byte per pixel, drawn by ACanvasLin8. */
    kABitmapFormatLinear15 = 2, /*!< Two bytes per pixel, drawn by ACanvasLin15. */
    kABitmapFormatLinear24 = 3, /*!< Three bytes per pixel, drawn by ACanvasLin24. */
    kABitmapFormatLinear32 = 4, /*!< Four bytes per pixel, drawn by ACanvasLin32. */
    kABitmapFormatUnknown5 = 5, /*!< One byte per pixel, also drawn by ACanvasLin8. */
    kABitmapFormatCount = 6     /*!< One past the last code the factory accepts. */
};

/** Bit position of the ABitmapFormat code inside ABitmap::mFlags. */
constexpr int kABitmapFormatShift = 8;

/** Mask of the ABitmapFormat code inside ABitmap::mFlags. */
constexpr unsigned int kABitmapFormatMask = 0xf00;

/**
 * Description of a pixel rectangle, its layout, and its palette.
 *
 * The record is not polymorphic and has no RTTI. Its name is inferred from the header its
 * allocations bill themselves to, `C:/FREQ/src/rndartt/abitmap.h`. That is the only source path
 * the shipped image retains. 165 copies of the tag pair ("APalette", "abitmap.h") appear in the
 * data segment, one per translation unit that inlined a palette allocation.
 *
 * ACanvas stores one of these at offset zero and reads mWidth and mHeight directly. The members
 * are therefore public, although a friend declaration would fit the image equally well.
 *
 * The record is 0x18 bytes. ACanvas::CreateForBitmap() copies it whole, then fills in
 * mBytesPerRow and mPixels for the copy.
 *
 * One detail of the layout is unsettled. The factory reads mFlags with a 32 bit load at offset
 * 0x04 and masks bits 8 through 11 out of it, while mWidth is read separately as a halfword at
 * offset 0x06. A 16 bit mFlags followed by a 16 bit mWidth reproduces both accesses, and the wide
 * load is then a toolchain choice rather than a second field.
 *
 * A second model of this record exists in the analysis program, 0x420 bytes with an APalette
 * placed inline at offset 0x18. That model does not fit the record ACanvas stores at offset zero.
 * An ACanvas is 0x28 bytes and its offsets 0x18 through 0x1f are the clip rectangle. A base and
 * derived pair reconciles the two, the 0x18 byte head described here being the base and the
 * 0x420 byte form adding the inline palette.
 */
struct ABitmap {
    /**
     * Set or replace a run of palette entries, allocating the palette on first use.
     *
     * The palette is allocated with the tag "APalette". Its two trailing words are cleared before
     * the allocation is tested against null. That order is harmless in practice.
     * AllocateTaggedMemory() treats a failure as fatal and never returns null.
     *
     * @param pEntries The entries to copy in.
     * @param nFirst The first palette index to write.
     * @param nCount The number of entries to write.
     * @ghidraAddress 0x005eb290
     */
    void SetPaletteEntries(const unsigned int *pEntries, int nFirst, int nCount);

    void *mPixels;         /*!< The pixel rectangle, null until allocated. +0x00 */
    unsigned short mFlags; /*!< Layout flags, with the format code at bits 8 to 11. +0x04 */
    short mWidth;          /*!< The width in pixels. +0x06 */
    short mHeight;         /*!< The height in pixels. +0x08 */
    short mBytesPerRow;    /*!< The distance between two rows in bytes. +0x0a */
    int mUnknown0c;        /*!< Purpose undetermined. +0x0c */
    int mUnknown10;        /*!< Purpose undetermined. +0x10 */
    APalette *mPalette;    /*!< The palette, null for a direct colour format. +0x14 */
};
