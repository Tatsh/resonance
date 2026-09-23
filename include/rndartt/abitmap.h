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

/** Colour key choices ABitmap::ApplyColorKey() takes. */
enum ABitmapColorKey {
    kABitmapColorKeyWhite = 1, /*!< Key on white. */
    kABitmapColorKeyBlack = 2  /*!< Key on black. */
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
 * Non-zero to leave bitmap colours in file order rather than swapping red and blue.
 *
 * Rnd::MovieStream::Update(), Rnd::Tex::OnMipLoaded(), and the routines at `0x00250638` and
 * `0x00254cf8` test it before calling ABitmap::SwapRedBlue(). VramTable::Screendump() swaps without
 * the test. The image never writes it, and it stays zero. The name is inferred.
 *
 * @ghidraAddress 0x00725cd0
 */
extern int g_nSkipColorSwap;

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

    /**
     * Return the ABitmapFormat code for a pixel width in bits.
     *
     * 4, 8, 16, 24, and 32 give kABitmapFormatLinear4 through kABitmapFormatLinear32, and every
     * other width gives kABitmapFormatLinear15.
     *
     * @param nBitsPerPixel The pixel width in bits.
     * @return The format code.
     * @ghidraAddress 0x00559310
     */
    static int FormatForBitsPerPixel(int nBitsPerPixel);

    /**
     * Return the size of a pixel rectangle in bytes, with rows packed at the format's own stride.
     *
     * The stride matches the one the constructor derives, `(nWidth + 2) / 2` for
     * kABitmapFormatLinear4 and nWidth times the matching entry of g_abBitmapBytesPerPixel
     * otherwise.
     *
     * @param nFormat The ABitmapFormat code.
     * @param nWidth The width in pixels.
     * @param nHeight The height in pixels.
     * @return The size in bytes.
     * @ghidraAddress 0x00559368
     */
    static int ComputeByteCount(int nFormat, int nWidth, int nHeight);

    /**
     * Exchange the red and blue fields of a run of 1555 pixels in place.
     *
     * @param pPixels The first pixel.
     * @param nCount The number of pixels.
     * @ghidraAddress 0x00559568
     */
    static void SwapRedBlue15(unsigned short *pPixels, int nCount);

    /**
     * Exchange the first and third byte of a run of three byte pixels in place.
     *
     * @param pPixels The first pixel.
     * @param nCount The number of pixels.
     * @ghidraAddress 0x005595c8
     */
    static void SwapRedBlue24(unsigned char *pPixels, int nCount);

    /**
     * Exchange the first and third byte of a run of four byte pixels in place.
     *
     * @param pPixels The first pixel.
     * @param nCount The number of pixels.
     * @ghidraAddress 0x00559600
     */
    static void SwapRedBlue32(unsigned char *pPixels, int nCount);

    /**
     * Pick a colour key from the flags and rebuild the alpha of every pixel from it.
     *
     * An indexed bitmap with a palette takes the index of the first palette entry among the first
     * APalette::mEnd whose colour channels are white (kABitmapColorKeyWhite) or black
     * (kABitmapColorKeyBlack). A 1555 bitmap takes 0x7fff or zero, and a 32 bit bitmap 0xffffff or
     * zero, the white bit winning when both are set. Other formats pick nothing.
     *
     * Whenever mHasTransparentColor is then set, whether by this call or before it, a canvas is
     * built over the bitmap with ACanvas::CreateForBitmap(), its
     * ACanvas::BuildAlphaFromColorKey() runs with mTransparentColor, and the canvas is deleted.
     * The canvas is used before the null test that guards its deletion.
     *
     * The one out-of-line copy sits inside the Rnd::Tex translation unit, and Rnd::Tex's slot 15
     * at 0x004e5928 is the one caller, passing the texture's flag word.
     *
     * @param nFlags ABitmapColorKey bits.
     * @ghidraAddress 0x004e5b78
     */
    void ApplyColorKey(int nFlags);

    /**
     * Exchange red and blue throughout the bitmap, or throughout its palette.
     *
     * An indexed format swaps the first mPalette->mEnd palette entries, and does nothing without a
     * palette. A direct colour format swaps every pixel of every row, stepping by mBytesPerRow.
     * The four row loops inline SwapRedBlue15(), SwapRedBlue24(), and SwapRedBlue32(). Callers
     * skip the call while g_nSkipColorSwap is non-zero.
     *
     * @ghidraAddress 0x00559140
     */
    void SwapRedBlue();

    /**
     * Copy the low byte of every palette entry into its alpha byte.
     *
     * The first mPalette->mEnd entries are rewritten, and a bitmap without a palette is not
     * changed. With bWhiten set, the three colour bytes of each entry also become 0xff, leaving a
     * white palette whose alpha carries the former low channel. Rnd::Tex::OnMipLoaded() is the one
     * caller, passing zero for texture flag 0x10 and one for flag 0x20. The out-of-line copy sits
     * in the Rnd::Tex translation unit. The name is inferred.
     *
     * @param bWhiten Non-zero to set the colour bytes to white as well.
     * @ghidraAddress 0x004e7d48
     */
    void SetPaletteAlphaFromLowByte(int bWhiten);

    /**
     * Make this bitmap an owning copy of another.
     *
     * Every descriptor field is taken from source, apart from mOwnsPixels, and mPalette is shared
     * rather than copied. Unless the format is kABitmapFormatRle8, a stride that differs from the
     * packed stride the constructor would derive is replaced by it, with mByteCount recomputed.
     * mByteCount bytes are then allocated with the tag "abitmap.h" and line 0x47, and the pixels
     * are copied in one block when both strides agree and row by row otherwise. No call site
     * survives in the shipped program, and the name is inferred.
     *
     * @param source The bitmap to copy.
     * @return 0, or -1 when the allocation fails.
     * @ghidraAddress 0x00558f28
     */
    int Copy(const ABitmap &source);

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
