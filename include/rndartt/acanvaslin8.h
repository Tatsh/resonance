#pragma once

#include "rndartt/acanvas8.h"

/**
 * Linear addressing for a canvas of eight-bit indexed pixels.
 *
 * The name comes from the RTTI descriptor at 0x008f09c0, whose mangled form is `11ACanvasLin8` and
 * whose single public base is ACanvas8 at offset zero. Like ACanvasLin4 it derives from the format
 * class rather than from ACanvas, so its base supplies the colour conversions and this class
 * supplies everything that depends on how pixels are addressed.
 *
 * Its table at 0x0083f818 runs the same 85 entries as its base's and overrides eighteen, where the
 * four-bit sibling overrides eight. Three are the pure slots ACanvas8 leaves, which makes the class
 * concrete. The other thirteen replace implementations the base already has, because one byte per
 * pixel lets a fill become a memset and a copy become a row copy, where the generic versions
 * address one pixel at a time. Four bits per pixel does not pay off that way, which is why the
 * sibling inherits all thirteen.
 *
 * ONE BYTE IS ONE PIXEL, so the address of a pixel is `mPixels + nY * mBytesPerRow + nX` with no
 * packing and no odd-start flag. The four-bit sibling needs both.
 *
 * Nine bodies are not yet written: the three block copies, the four row operations, the two stretch
 * variants, and the textured row. Each is recoverable and each is a loop over the row primitives
 * the written bodies already establish.
 */
class ACanvasLin8 : public ACanvas8 {
public:
    /**
     * Construct over a bitmap.
     *
     * @param bitmap The bitmap the canvas addresses.
     * @ghidraAddress 0x00628600
     */
    explicit ACanvasLin8(const ABitmap &bitmap);

    /**
     * Slot 1.
     *
     * The body restores ACanvas's table rather than ACanvas8's, because the inlined destructor
     * chain runs to the root and the intermediate store is dead.
     *
     * @ghidraAddress 0x00628568
     */
    virtual ~ACanvasLin8();

    /** Slot 13. @ghidraAddress 0x00628638 */
    virtual void PutPixelNoClip(int nX, int nY);

    /** Slot 15. @ghidraAddress 0x00628658 */
    virtual void PutPixelIndexedNoClip(int nX, int nY, int nIndex);

    /** Slot 25. @ghidraAddress 0x00628678 */
    virtual int GetPixelIndexedNoClip(int nX, int nY);

    /** Slot 35. One memset across the span. @ghidraAddress 0x00628698 */
    virtual void FillRowNoClip(int nY, int nLeft, int nRight);

    /** Slot 37. @ghidraAddress 0x006286d0 */
    virtual void FillColumnNoClip(int nX, int nTop, int nBottom);

    /** Slot 39. One memset per row. @ghidraAddress 0x00628718 */
    virtual void FillRectNoClip(ARect rect);

    /** Slot 43. Body not yet written. @ghidraAddress 0x006287b8 */
    virtual void RemapRectIndices(ARect rect, const unsigned char *pRemap);

    /** Slot 46. Body not yet written. @ghidraAddress 0x00628db0 */
    virtual void TextureRowIndexed(int nY,
                                   int nLeft,
                                   int nRight,
                                   const ABitmap *pSource,
                                   APoint *pSourcePosition,
                                   const APoint *pSourceStep);

    /** Slot 47. Body not yet written. @ghidraAddress 0x00628848 */
    virtual void Blit4NoClip(const ABitmap &source, int nX, int nY);

    /** Slot 49. Body not yet written. @ghidraAddress 0x00628918 */
    virtual void Blit8NoClip(const ABitmap &source, int nX, int nY);

    /** Slot 57. Body not yet written. @ghidraAddress 0x00628a48 */
    virtual void BlitRle8NoClip(const ABitmap &source, int nX, int nY);

    /** Slot 75. Body not yet written. @ghidraAddress 0x00628ae8 */
    virtual void RemapRowIndexed(const ARowSpan &span, const unsigned char *pRemap);

    /** Slot 78. Body not yet written. @ghidraAddress 0x00628ba8 */
    virtual void BlendRowIndexed(const ARowSpan &span, const unsigned char *const *ppBlend);

    /**
     * Slot 79. Body not yet written.
     *
     * The program titled this routine for the four-bit sibling until the table diff placed it
     * here: it occupies slot 79 of this class's table and appears in none of the 85 entries of the
     * sibling's, which does not override the slot at all. It sits just below this class's main
     * block, so the earlier attribution rested on adjacency.
     *
     * @ghidraAddress 0x006284a0
     */
    virtual void StretchRowIndexed(const AStretchSpan &span);

    /** Slot 83. Body not yet written. @ghidraAddress 0x00628c80 */
    virtual void StretchRowRemap(const AStretchSpan &span, const unsigned char *pRemap);

    /** Slot 84. Body not yet written. @ghidraAddress 0x00628d10 */
    virtual void StretchRowBlend(const AStretchSpan &span, const unsigned char *const *ppBlend);
};
