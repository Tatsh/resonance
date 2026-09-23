#pragma once

class APalette;

/**
 * Pixel layout code stored in ABitmap::mFormat.
 *
 * ACanvas::CreateForBitmap() accepts only codes 0 through 5 and rejects every larger code. Each
 * accepted code selects one of the five linear canvas subclasses. The mapping was recovered from
 * the jump table at 0x00837d90 together with the type function each branch installs.
 *
 * The same code selects a copy routine in ACanvas::DrawGlyphNoClip(), through the six entry member
 * pointer table at 0x0077dc98, and a read routine through the table at 0x0077dcf8. Both tables
 * confirm the order, because their entries address the 4, 8, 15, 24, 32, and run length encoded
 * slots in that sequence.
 *
 * Code 5 describes a run length encoded source, and the same code allocated as a canvas gives a
 * linear eight bit surface. The two readings agree, because a run length encoded rectangle is a
 * copy source rather than a drawing destination.
 */
enum ABitmapFormat {
    kABitmapFormatLinear4 = 0,  /*!< Four bits per pixel, drawn by ACanvasLin4. */
    kABitmapFormatLinear8 = 1,  /*!< One byte per pixel, drawn by ACanvasLin8. */
    kABitmapFormatLinear15 = 2, /*!< Two bytes per pixel, drawn by ACanvasLin15. */
    kABitmapFormatLinear24 = 3, /*!< Three bytes per pixel, drawn by ACanvasLin24. */
    kABitmapFormatLinear32 = 4, /*!< Four bytes per pixel, drawn by ACanvasLin32. */
    kABitmapFormatRle8 = 5,     /*!< Run length encoded bytes, allocated as ACanvasLin8. */
    kABitmapFormatCount = 6     /*!< One past the last code the factory accepts. */
};

/**
 * Bytes one pixel of each ABitmapFormat occupies.
 *
 * The entry for kABitmapFormatLinear4 is zero, because four bit rows use a separate stride
 * formula. Both ABitmap::ABitmap() and ACanvas::CreateForBitmap() read the table.
 *
 * @ghidraAddress 0x00725cc0
 */
extern const unsigned char g_abBitmapBytesPerPixel[kABitmapFormatCount];

/**
 * Bits one pixel of each ABitmapFormat occupies.
 *
 * The table sits immediately after g_abBitmapBytesPerPixel and no reader of it was located inside
 * the art library. A second pair of the same shape exists at 0x0070d3d0, where the first table
 * maps the same codes to PlayStation 2 graphics synthesiser storage modes.
 *
 * @ghidraAddress 0x00725cc8
 */
extern const unsigned char g_abBitmapBitsPerPixel[kABitmapFormatCount];

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
 * The record is 0x18 bytes. ACanvas::CreateForBitmap() copies it whole, then fills in mBytesPerRow
 * and mPixels for the copy. Every clipped copy slot of ACanvas copies exactly 0x18 bytes of its
 * argument onto its stack, mutates the copy, and passes the copy on, which confirms the size.
 *
 * The five bit-fields were modelled as one halfword mFlags in an earlier pass. The constructor
 * settles them, because it writes the low eight bits with a byte store and the four bit code and
 * the two flag bits with read, modify, and write cycles over the enclosing halfword. Reading that
 * halfword with a word load, as both the constructor and ACanvas::Blit4NoClip() do, is a toolchain
 * choice over a 16 bit container rather than evidence of a wider member. The neighbouring mWidth
 * survives every such cycle because the read precedes the write.
 *
 * A second model of this record exists in the analysis program, 0x420 bytes with an APalette
 * placed inline at offset 0x18. That model does not fit the record ACanvas stores at offset zero.
 * An ACanvas is 0x28 bytes and its offsets 0x18 through 0x1f are the clip rectangle. A base and
 * derived pair reconciles the two, the 0x18 byte head described here being the base and the
 * 0x420 byte form adding the inline palette.
 *
 * One copy of the record does not fit the 0x18 byte size. ACanvas::DrawGlyph() copies 0x1c bytes
 * of the glyph it is about to draw and then passes the copy where an ABitmap is expected. Either
 * a font glyph is a derived record with one further word, or the description is 0x1c bytes with a
 * member no canvas routine reads. The four bytes are unresolved.
 */
struct ABitmap {
    /**
     * Describe a pixel rectangle, allocating the pixels when the caller supplies none.
     *
     * Writes mFormat from the second argument, clears mOddNibbleStart, and writes
     * mHasTransparentColor from the third. A row stride of zero is derived from the format
     * instead, as `(nWidth + 2) / 2` for kABitmapFormatLinear4 and as nWidth times the matching
     * entry of g_abBitmapBytesPerPixel for every other code. mByteCount then becomes the stride
     * times the height.
     *
     * A null pixel pointer allocates mByteCount bytes with the tag "abitmap.h" and line 0x47, and
     * sets mOwnsPixels. A supplied pointer clears the flag.
     *
     * mTransparentColor and mPalette are both cleared, so a caller that wants either writes it
     * afterwards. ACanvas::BlitRle8NoClip() and its three relatives do exactly that.
     *
     * @param pPixels The pixel rectangle, or null to allocate one.
     * @param nFormat The ABitmapFormat code.
     * @param bHasTransparentColor Whether a later mTransparentColor applies.
     * @param nWidth The width in pixels.
     * @param nHeight The height in pixels.
     * @param nBytesPerRow The row stride, or zero to derive it from the format.
     * @ghidraAddress 0x005593a0
     */
    ABitmap(void *pPixels,
            int nFormat,
            bool bHasTransparentColor,
            int nWidth,
            int nHeight,
            int nBytesPerRow);

    /**
     * Set or replace a run of palette entries, allocating the palette on first use.
     *
     * The palette is constructed before the allocation is tested against null, and the path
     * where mPalette was already set calls SetEntries() with no test. The order is harmless in
     * practice. AllocateTaggedMemory() treats a failure as fatal and never returns null.
     *
     * @param pEntries The entries to copy in.
     * @param nFirst The first palette index to write.
     * @param nCount The number of entries to write.
     * @ghidraAddress 0x005eb290
     */
    void SetPaletteEntries(const unsigned int *pEntries, int nFirst, int nCount);

    void *mPixels; /*!< The pixel rectangle, null until allocated. +0x00 */
    unsigned short mHasTransparentColor : 8; /*!< Whether mTransparentColor applies. +0x04 */
    unsigned short mFormat : 4;              /*!< The ABitmapFormat code. +0x05 bits 0 to 3 */
    unsigned short mOwnsPixels : 1;          /*!< Set when the constructor allocated. bit 4 */
    unsigned short mOddNibbleStart : 1;      /*!< Set when a four bit row starts high. bit 5 */
    unsigned short mUnused : 2;              /*!< No reader or writer was located. bits 6 and 7 */
    short mWidth;                            /*!< The width in pixels. +0x06 */
    short mHeight;                           /*!< The height in pixels. +0x08 */
    short mBytesPerRow;                      /*!< The distance between two rows in bytes. +0x0a */
    unsigned int mTransparentColor;          /*!< The pixel value a copy skips. +0x0c */
    int mByteCount;                          /*!< The size of the pixel rectangle in bytes. +0x10 */
    APalette *mPalette; /*!< The palette, null for a direct colour format. +0x14 */
};
