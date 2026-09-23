#pragma once

#include "rndartt/acanvas15.h"

/**
 * Linear addressing for a canvas of 1555 pixels.
 *
 * The name comes from the RTTI descriptor at 0x008ef2a0, whose mangled form is `12ACanvasLin15` and
 * whose single public base is ACanvas15 at offset zero. Like the other layout classes it derives
 * from the format class, so its base converts every other colour format into 1555 and this class
 * supplies everything that depends on where a pixel sits in memory.
 *
 * Its table at 0x0083cb58 runs the same 85 entries as its base's and overrides sixteen. Four are
 * the pure slots ACanvas15 leaves, which makes the class concrete: the alpha builder at 12, the
 * colourless store at 13, and the 1555 pixel pair at 17 and 27. The other twelve replace
 * implementations the base already has, because knowing the layout lets a fill walk memory
 * directly.
 *
 * TWO BYTES ARE ONE PIXEL, so a pixel address is `mPixels + nY * mBytesPerRow + nX * 2`.
 *
 * THE ALPHA BUILDER IS WHY SLOT 12 IS LEFT PURE BY THE DIRECT-COLOUR FORMATS. Here it walks every
 * pixel of the bitmap, setting the 1555 alpha bit on each whose colour differs from the key and
 * clearing it on each that matches, so it needs the pixel layout. The indexed format supplies the
 * same slot in its own format class instead, because there the work is rewriting palette entries
 * and no layout is involved. That asymmetry was inferred from the slot tables and is confirmed
 * here by the body.
 *
 * Every override that reads a palette converts the entry to 1555 through APackRgb1555From8888().
 * The block copies and the textured row resolve the palette from the source, then the canvas, then
 * g_pDefaultPalette, and the span overrides read ARowSpan::mPalette or AStretchSpan::mPalette.
 * Each returns without drawing when the palette is null.
 */
class ACanvasLin15 : public ACanvas15 {
public:
    /**
     * Construct over a bitmap.
     *
     * The body calls ACanvas's constructor directly rather than ACanvas15's, because the format
     * class's own constructor is trivial and inlined away.
     *
     * @param bitmap The bitmap the canvas addresses.
     * @ghidraAddress 0x00619008
     */
    explicit ACanvasLin15(const ABitmap &bitmap);

    /**
     * Slot 1.
     *
     * The body restores ACanvas's table rather than ACanvas15's, because the inlined destructor
     * chain runs to the root and the intermediate store is dead.
     *
     * @ghidraAddress 0x00618f68
     */
    virtual ~ACanvasLin15();

    /** Slot 12. Walks every pixel, keying the alpha bit off the colour. @ghidraAddress 0x00619040
     */
    virtual void BuildAlphaFromColorKey(unsigned int nColorKey);

    // Lookup only: this overload would otherwise hide the root's three-argument one, which
    // ACanvas15 does not redeclare.
    using ACanvas::PutPixelNoClip;

    /** Slot 13. @ghidraAddress 0x006190c0 */
    virtual void PutPixelNoClip(int nX, int nY);

    /** Slot 17. @ghidraAddress 0x006190e8 */
    virtual void PutPixel15NoClip(int nX, int nY, unsigned short nColor);

    /** Slot 27. @ghidraAddress 0x00619108 */
    virtual unsigned short GetPixel15NoClip(int nX, int nY);

    /** Slot 35. @ghidraAddress 0x00619128 */
    virtual void FillRowNoClip(int nY, int nLeft, int nRight);

    /** Slot 37. @ghidraAddress 0x00619170 */
    virtual void FillColumnNoClip(int nX, int nTop, int nBottom);

    /** Slot 39. @ghidraAddress 0x006191c0 */
    virtual void FillRectNoClip(ARect rect);

    /** Slot 46. @ghidraAddress 0x00619518 */
    virtual void TextureRowIndexed(int nY,
                                   int nLeft,
                                   int nRight,
                                   const ABitmap *pSource,
                                   APoint *pSourcePosition,
                                   const APoint *pSourceStep);

    /** Slot 47. @ghidraAddress 0x00618b00 */
    virtual void Blit4NoClip(const ABitmap &source, int nX, int nY);

    /** Slot 49. @ghidraAddress 0x00618c78 */
    virtual void Blit8NoClip(const ABitmap &source, int nX, int nY);

    /** Slot 51. @ghidraAddress 0x00618e00 */
    virtual void Blit15NoClip(const ABitmap &source, int nX, int nY);

    /** Slot 75. @ghidraAddress 0x00619280 */
    virtual void RemapRowIndexed(const ARowSpan &span, const unsigned char *pRemap);

    /** Slot 79. @ghidraAddress 0x00619360 */
    virtual void StretchRowIndexed(const AStretchSpan &span);

    /** Slot 83. @ghidraAddress 0x00619430 */
    virtual void StretchRowRemap(const AStretchSpan &span, const unsigned char *pRemap);
};
