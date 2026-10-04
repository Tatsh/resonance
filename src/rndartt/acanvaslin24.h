#pragma once

#include "rndartt/acanvas24.h"

/**
 * Linear addressing for a canvas of 24-bit pixels.
 *
 * Its RTTI descriptor is at 0x008ef240, and its single public base is ACanvas24 at offset zero. It
 * is the last of the four layout classes, and like the others it derives from its format class
 * rather than from ACanvas.
 *
 * Its table at 0x0083c878 runs the same 85 entries as its base's and overrides fifteen. Four are
 * the pure slots ACanvas24 leaves, which makes the class concrete: the alpha builder at 12, the
 * colourless store at 13, and the channel-triple pixel pair at 19 and 29. That is the third
 * independent confirmation of the family rule, because each layout class fills exactly the slots
 * its own format class leaves and no others.
 *
 * THREE BYTES ARE ONE PIXEL, so a pixel address is `mPixels + nY * mBytesPerRow + nX * 3`, and the
 * binary forms that column offset as `nX * 2 + nX` rather than with a multiply.
 *
 * THE ALPHA BUILDER IS EMPTY, and that is the point of it. A 24-bit pixel has no alpha bit to key,
 * so there is nothing to build, and the slot exists only because the base declares it pure. Its
 * two sibling formats do real work there: the 1555 layout walks every pixel setting or clearing the
 * alpha bit, and the indexed format does it in its own format class by rewriting palette entries.
 * So the same slot is layout-dependent, layout-independent, or vacuous depending on the format,
 * which is why the base cannot implement it and every format class leaves it to someone.
 *
 * Every override that reads a palette stores the three low bytes of the entry, red first. The
 * block copy and the textured row resolve the palette from the source, then the canvas, then
 * ACanvas::palDefault, and the span overrides read ARowInfo::mPalette or AScaledRowInfo::mPalette.
 * Each returns without drawing when the palette is null.
 *
 * The destructor in slot 1 at `0x006183d0` is compiler-generated. It restores ACanvas's table at
 * 0x00837dc8, which is all the inlined base destructors do, and frees the object when the
 * deleting flag is set. Its bytes are identical to ACanvasLin32's at `0x006141a0`.
 */
class ACanvasLin24 : public ACanvas24 {
public:
    /**
     * Construct over a bitmap.
     *
     * The body calls ACanvas's constructor directly rather than ACanvas24's, because the format
     * class's own constructor is trivial and inlined away. It zeroes the colour with a word store,
     * matching the union its format class keeps there.
     *
     * @param bitmap The bitmap the canvas addresses.
     * @ghidraAddress NTSC-U/C: 0x00618470
     * @ghidraAddress PAL: 0x00659000
     */
    explicit ACanvasLin24(const ABitmap &bitmap);

    /**
     * Slot 12. Empty, because a 24-bit pixel has no alpha bit to key.
     *
     * The override exists only because ACanvas24 declares the slot pure.
     *
     * @param nColorKey The colour the other formats would make transparent, unused here.
     * @ghidraAddress NTSC-U/C: 0x006184a8
     * @ghidraAddress PAL: 0x00659038
     */
    virtual void SetAlphaValues(unsigned int nColorKey);

    /**
     * Slot 13. Writes the three stored channel bytes.
     *
     * @ghidraAddress NTSC-U/C: 0x006184b0
     * @ghidraAddress PAL: 0x00659040
     */
    virtual void DrawPixelU(int nX, int nY);

    /**
     * Slot 19.
     *
     * @ghidraAddress NTSC-U/C: 0x006184e8
     * @ghidraAddress PAL: 0x00659078
     */
    virtual void DrawPixel24U(int nX, int nY, const unsigned char *pRGB);

    /**
     * Slot 29.
     *
     * @ghidraAddress NTSC-U/C: 0x00618528
     * @ghidraAddress PAL: 0x006590b8
     */
    virtual void GetPixel24U(int nX, int nY, unsigned char *pRGB);

    /**
     * Slot 35.
     *
     * @ghidraAddress NTSC-U/C: 0x00618568
     * @ghidraAddress PAL: 0x006590f8
     */
    virtual void DrawHorzLineU(int nY, int nLeft, int nRight);

    /**
     * Slot 37.
     *
     * @ghidraAddress NTSC-U/C: 0x006185d0
     * @ghidraAddress PAL: 0x00659160
     */
    virtual void DrawVertLineU(int nX, int nTop, int nBottom);

    /**
     * Slot 39.
     *
     * @ghidraAddress NTSC-U/C: 0x00618630
     * @ghidraAddress PAL: 0x006591c0
     */
    virtual void DrawRectU(ARect rect);

    /**
     * Slot 46.
     *
     * @ghidraAddress NTSC-U/C: 0x00618a28
     * @ghidraAddress PAL: 0x006595b8
     */
    virtual void DrawTmapRow8U(int nY,
                               int nLeft,
                               int nRight,
                               const ABitmap *pSource,
                               APoint *pSourcePosition,
                               const APoint *pSourceStep);

    /**
     * Slot 49.
     *
     * The key is compared against the low byte of the transparent colour.
     *
     * @ghidraAddress NTSC-U/C: 0x006186f8
     * @ghidraAddress PAL: 0x00659288
     */
    virtual void DrawBitmapLin8U(const ABitmap &source, int nX, int nY);

    /**
     * Slot 53.
     *
     * A keyed row compares the pixel's three bytes, widened with a zero, against the whole
     * transparent colour.
     *
     * @ghidraAddress NTSC-U/C: 0x00618270
     * @ghidraAddress PAL: 0x00658e00
     */
    virtual void DrawBitmapLin24U(const ABitmap &source, int nX, int nY);

    /**
     * Slot 75.
     *
     * The key is compared against the low byte of the transparent colour.
     *
     * @ghidraAddress NTSC-U/C: 0x00618800
     * @ghidraAddress PAL: 0x00659390
     */
    virtual void DrawClutBitmapRowLin8U(const ARowInfo &span, const unsigned char *pRemap);

    /**
     * Slot 79.
     *
     * @ghidraAddress NTSC-U/C: 0x006188b0
     * @ghidraAddress PAL: 0x00659440
     */
    virtual void DrawScaledBitmapRowLin8U(const AScaledRowInfo &span);

    /**
     * Slot 83.
     *
     * @ghidraAddress NTSC-U/C: 0x00618968
     * @ghidraAddress PAL: 0x006594f8
     */
    virtual void DrawScaledClutBitmapRowLin8U(const AScaledRowInfo &span,
                                              const unsigned char *pRemap);
};
