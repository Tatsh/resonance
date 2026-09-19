#pragma once

#include "rndartt/acanvas24.h"

/**
 * Linear addressing for a canvas of 24-bit pixels.
 *
 * The name comes from the RTTI descriptor at 0x008ef240, whose mangled form is `12ACanvasLin24` and
 * whose single public base is ACanvas24 at offset zero. It is the last of the four layout classes,
 * and like the others it derives from its format class rather than from ACanvas.
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
 * Six bodies are not yet written: the two block copies, the indexed row remap, the textured row,
 * and the two stretch variants.
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
     * @ghidraAddress 0x00618470
     */
    explicit ACanvasLin24(const ABitmap &bitmap);

    /**
     * Slot 1.
     *
     * @ghidraAddress 0x006183d0
     */
    virtual ~ACanvasLin24();

    /**
     * Slot 12. Empty, because a 24-bit pixel has no alpha bit to key.
     *
     * @param nColorKey The colour the other formats would make transparent, unused here.
     * @ghidraAddress 0x006184a8
     */
    virtual void BuildAlphaFromColorKey(unsigned int nColorKey);

    /** Slot 13. Writes the three stored channel bytes. @ghidraAddress 0x006184b0 */
    virtual void PutPixelNoClip(int nX, int nY);

    /** Slot 19. @ghidraAddress 0x006184e8 */
    virtual void PutPixelRGBNoClip(int nX, int nY, const unsigned char *pRGB);

    /** Slot 29. @ghidraAddress 0x00618528 */
    virtual void GetPixelRGBNoClip(int nX, int nY, unsigned char *pRGB);

    /** Slot 35. @ghidraAddress 0x00618568 */
    virtual void FillRowNoClip(int nY, int nLeft, int nRight);

    /** Slot 37. @ghidraAddress 0x006185d0 */
    virtual void FillColumnNoClip(int nX, int nTop, int nBottom);

    /** Slot 39. @ghidraAddress 0x00618630 */
    virtual void FillRectNoClip(ARect rect);

    /** Slot 46. Body not yet written. @ghidraAddress 0x00618a28 */
    virtual void TextureRowIndexed(int nY,
                                   int nLeft,
                                   int nRight,
                                   const ABitmap *pSource,
                                   APoint *pSourcePosition,
                                   const APoint *pSourceStep);

    /** Slot 49. Body not yet written. @ghidraAddress 0x006186f8 */
    virtual void Blit8NoClip(const ABitmap &source, int nX, int nY);

    /** Slot 53. Body not yet written. @ghidraAddress 0x00618270 */
    virtual void Blit24NoClip(const ABitmap &source, int nX, int nY);

    /** Slot 75. Body not yet written. @ghidraAddress 0x00618800 */
    virtual void RemapRowIndexed(const ARowSpan &span, const unsigned char *pRemap);

    /** Slot 79. Body not yet written. @ghidraAddress 0x006188b0 */
    virtual void StretchRowIndexed(const AStretchSpan &span);

    /** Slot 83. Body not yet written. @ghidraAddress 0x00618968 */
    virtual void StretchRowRemap(const AStretchSpan &span, const unsigned char *pRemap);
};
