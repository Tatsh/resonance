#pragma once

#include "rndartt/acanvas8.h"

/**
 * Linear addressing for a canvas of eight-bit indexed pixels.
 *
 * Its RTTI descriptor is at 0x008f09c0, and its single public base is ACanvas8 at offset zero. Like
 * ACanvasLin4 it derives from the format class rather than from ACanvas. Its base supplies the
 * colour conversions and this class supplies everything that depends on how pixels are addressed.
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
 */
class ACanvasLin8 : public ACanvas8 {
public:
    /**
     * Construct over a bitmap.
     *
     * @param bitmap The bitmap the canvas addresses.
     * @ghidraAddress NTSC-U/C: 0x00628600
     * @ghidraAddress PAL: 0x00669190
     */
    explicit ACanvasLin8(const ABitmap &bitmap);

    /**
     * Slot 1.
     *
     * The body restores ACanvas's table rather than ACanvas8's, because the inlined destructor
     * chain runs to the root and the intermediate store is dead.
     *
     * @ghidraAddress NTSC-U/C: 0x00628568
     * @ghidraAddress PAL: 0x006690f8
     */
    virtual ~ACanvasLin8();

    // Lookup only, as in the base: this overload would otherwise hide the base's other one.
    using ACanvas8::PutPixelNoClip;

    /**
     * Slot 13.
     *
     * @ghidraAddress NTSC-U/C: 0x00628638
     * @ghidraAddress PAL: 0x006691c8
     */
    virtual void PutPixelNoClip(int nX, int nY);

    /**
     * Slot 15.
     *
     * @ghidraAddress NTSC-U/C: 0x00628658
     * @ghidraAddress PAL: 0x006691e8
     */
    virtual void PutPixelIndexedNoClip(int nX, int nY, int nIndex);

    /**
     * Slot 25.
     *
     * @ghidraAddress NTSC-U/C: 0x00628678
     * @ghidraAddress PAL: 0x00669208
     */
    virtual int GetPixelIndexedNoClip(int nX, int nY);

    /**
     * Slot 35. One memset across the span.
     *
     * @ghidraAddress NTSC-U/C: 0x00628698
     * @ghidraAddress PAL: 0x00669228
     */
    virtual void FillRowNoClip(int nY, int nLeft, int nRight);

    /**
     * Slot 37.
     *
     * @ghidraAddress NTSC-U/C: 0x006286d0
     * @ghidraAddress PAL: 0x00669260
     */
    virtual void FillColumnNoClip(int nX, int nTop, int nBottom);

    /**
     * Slot 39. One memset per row.
     *
     * @ghidraAddress NTSC-U/C: 0x00628718
     * @ghidraAddress PAL: 0x006692a8
     */
    virtual void FillRectNoClip(ARect rect);

    /**
     * Slot 43.
     *
     * @ghidraAddress NTSC-U/C: 0x006287b8
     * @ghidraAddress PAL: 0x00669348
     */
    virtual void RemapRectIndices(ARect rect, const unsigned char *pRemap);

    /**
     * Slot 46.
     *
     * @ghidraAddress NTSC-U/C: 0x00628db0
     * @ghidraAddress PAL: 0x00669940
     */
    virtual void TextureRowIndexed(int nY,
                                   int nLeft,
                                   int nRight,
                                   const ABitmap *pSource,
                                   APoint *pSourcePosition,
                                   const APoint *pSourceStep);

    /**
     * Slot 47. Unpacks nibbles, honouring the source's odd-start flag.
     *
     * @ghidraAddress NTSC-U/C: 0x00628848
     * @ghidraAddress PAL: 0x006693d8
     */
    virtual void Blit4NoClip(const ABitmap &source, int nX, int nY);

    /**
     * Slot 49.
     *
     * Three tiers, widest first. An unkeyed source whose pitch matches this canvas's is copied in
     * one block, an unkeyed source is copied a row at a time, and a keyed source is walked a byte
     * at a time.
     *
     * @ghidraAddress NTSC-U/C: 0x00628918
     * @ghidraAddress PAL: 0x006694a8
     */
    virtual void Blit8NoClip(const ABitmap &source, int nX, int nY);

    /**
     * Slot 57. Decodes through ARle8Reader, one row per call.
     *
     * @ghidraAddress NTSC-U/C: 0x00628a48
     * @ghidraAddress PAL: 0x006695d8
     */
    virtual void BlitRle8NoClip(const ABitmap &source, int nX, int nY);

    /**
     * Slot 75.
     *
     * @ghidraAddress NTSC-U/C: 0x00628ae8
     * @ghidraAddress PAL: 0x00669678
     */
    virtual void RemapRowIndexed(const ARowSpan &span, const unsigned char *pRemap);

    /**
     * Slot 78.
     *
     * Skips a source index equal to the span transparent colour, where the base skips index zero.
     *
     * @ghidraAddress NTSC-U/C: 0x00628ba8
     * @ghidraAddress PAL: 0x00669738
     */
    virtual void BlendRowIndexed(const ARowSpan &span, const unsigned char *const *ppBlend);

    /**
     * Slot 79.
     *
     * The routine occupies slot 79 of this class's table and does not appear in the 85 entries of
     * the four-bit sibling's table. The sibling does not override the slot.
     *
     * @ghidraAddress NTSC-U/C: 0x006284a0
     * @ghidraAddress PAL: 0x00669030
     */
    virtual void StretchRowIndexed(const AStretchSpan &span);

    /**
     * Slot 83.
     *
     * @ghidraAddress NTSC-U/C: 0x00628c80
     * @ghidraAddress PAL: 0x00669810
     */
    virtual void StretchRowRemap(const AStretchSpan &span, const unsigned char *pRemap);

    /**
     * Slot 84.
     *
     * @ghidraAddress NTSC-U/C: 0x00628d10
     * @ghidraAddress PAL: 0x006698a0
     */
    virtual void StretchRowBlend(const AStretchSpan &span, const unsigned char *const *ppBlend);
};
