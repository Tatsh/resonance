#pragma once

#include "rndartt/acanvas32.h"

/**
 * Canvas over a linear rectangle of 32 bit pixels.
 *
 * The name comes from the RTTI descriptor at 0x0086f5f0, whose mangled form is `12ACanvasLin32`
 * and whose single public base is ACanvas32 at offset zero. Its virtual function table is at
 * 0x0083c360 and terminates on the zero entry at 0x0083c610, matching the 85 slots of ACanvas.
 *
 * ACanvas::CreateForBitmap() constructs one for kABitmapFormatLinear32 through branch 4 of the
 * jump table at 0x00837d90.
 *
 * The class adds no data member, so it shares the 0x28 byte layout of ACanvas32. It overrides
 * exactly the slots that benefit from addressing the rectangle directly. Every other slot arrives
 * from ACanvas32 or ACanvas.
 *
 * The address of a pixel is `mBitmap.mPixels + mBitmap.mBytesPerRow * nY + 4 * nX` in every
 * override.
 *
 * The destructor in slot 1 at `0x006141a0` is compiler-generated. It restores ACanvas's table at
 * 0x00837dc8, which is all the inlined base destructors do, and frees the object when the
 * deleting flag is set.
 */
class ACanvasLin32 : public ACanvas32 {
public:
    /**
     * Adopt a pixel description.
     *
     * The out-of-line copy calls ACanvas's constructor directly, installs this class's table, and
     * clears mColor. ACanvas::CreateForBitmap() compiles the same sequence inline.
     *
     * @param bitmap The description to adopt.
     * @ghidraAddress 0x00614240
     */
    explicit ACanvasLin32(const ABitmap &bitmap);

    /**
     * Rewrite every alpha byte of the pixel rectangle from a colour key.
     *
     * A pixel whose three colour channels equal the key loses its alpha byte, and every other
     * pixel gains a full one. The key is masked to its three colour channels first.
     *
     * @param nColorKey The colour to treat as transparent.
     * @ghidraAddress 0x00614278
     */
    void BuildAlphaFromColorKey(unsigned int nColorKey);

    /**
     * Store the pen colour at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @ghidraAddress 0x00614308
     */
    void PutPixelNoClip(int nX, int nY);

    /**
     * Store one 8888 colour at one point, with no clip test.
     *
     * Every colour format of ACanvas32 arrives here, so the whole family of pixel writers narrows
     * to this one word store.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nColor The colour.
     * @ghidraAddress 0x00614330
     */
    void PutPixelNoClip(int nX, int nY, unsigned int nColor);

    /**
     * Read one point as an 8888 colour, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The colour.
     * @ghidraAddress 0x00614350
     */
    unsigned int GetPixelNoClip(int nX, int nY);

    /**
     * Fill part of one row with the pen colour, with no clip test.
     *
     * @param nY The row.
     * @param nLeft The first column.
     * @param nRight One past the last column.
     * @ghidraAddress 0x00614370
     */
    void FillRowNoClip(int nY, int nLeft, int nRight);

    /**
     * Fill part of one column with the pen colour, with no clip test.
     *
     * @param nX The column.
     * @param nTop The first row.
     * @param nBottom One past the last row.
     * @ghidraAddress 0x006143b8
     */
    void FillColumnNoClip(int nX, int nTop, int nBottom);

    /**
     * Fill a rectangle with the pen colour, with no clip test.
     *
     * @param rect The rectangle.
     * @ghidraAddress 0x00614408
     */
    void FillRectNoClip(ARect rect);

    /**
     * Fill part of one row by sampling an indexed source bitmap along a fixed step.
     *
     * Resolves the palette from the source, then the canvas, then g_pDefaultPalette, and returns
     * when none is available. Unlike the base implementation it applies no transparency test.
     *
     * @param nY The destination row.
     * @param nLeft The first destination column.
     * @param nRight One past the last destination column.
     * @param pSource The source bitmap.
     * @param pSourcePosition The source position in 24.8 fixed point, advanced in place.
     * @param pSourceStep The per column advance in 24.8 fixed point.
     * @ghidraAddress 0x006148e0
     */
    void TextureRowIndexed(int nY,
                           int nLeft,
                           int nRight,
                           const ABitmap *pSource,
                           APoint *pSourcePosition,
                           const APoint *pSourceStep);

    /**
     * Copy a four bit source bitmap, with no clip test.
     *
     * Unpacks one row into g_abCanvasRowScratch and stores one palette entry
     * per pixel. A skipped pixel still advances the destination.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress 0x006144b0
     */
    void Blit4NoClip(const ABitmap &source, int nX, int nY);

    /**
     * Copy an eight bit source bitmap, with no clip test.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress 0x00614608
     */
    void Blit8NoClip(const ABitmap &source, int nX, int nY);

    /**
     * Copy a 32 bit source bitmap, with no clip test.
     *
     * A source with no transparent colour whose stride matches the canvas stride copies in one
     * call. A source with no transparent colour and a different stride copies one row per call.
     * A source with a transparent colour tests every pixel.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress 0x00614070
     */
    void Blit32NoClip(const ABitmap &source, int nX, int nY);

    /**
     * Store one row of palette indices through a remap table.
     *
     * Returns when the span supplies no palette, unlike the base implementation, which resolves
     * the index through the virtual writer instead.
     *
     * @param span The row to store.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress 0x006146f0
     */
    void RemapRowIndexed(const ARowSpan &span, const unsigned char *pRemap);

    /**
     * Store one row of palette indices sampled along a fixed step.
     *
     * @param span The row to store.
     * @ghidraAddress 0x00614790
     */
    void StretchRowIndexed(const AStretchSpan &span);

    /**
     * Store one row of palette indices sampled along a fixed step, through a remap table.
     *
     * @param span The row to store.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress 0x00614830
     */
    void StretchRowRemap(const AStretchSpan &span, const unsigned char *pRemap);
};
