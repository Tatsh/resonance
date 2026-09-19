#pragma once

#include "rndartt/acanvas8.h"

/**
 * Linear addressing for a canvas of four-bit indexed pixels.
 *
 * The name comes from the RTTI descriptor at 0x008f0920, whose mangled form is `11ACanvasLin4` and
 * whose single public base is ACanvas8 at offset zero. It derives from the FORMAT class rather than
 * from ACanvas, which is the two-level shape the whole family uses: ACanvas8 supplies the colour
 * conversions and leaves pure what depends on how pixels are packed, and this class supplies that.
 *
 * Its table at 0x0083f4d8 runs the same 85 entries as its base's and overrides eight. Three are the
 * pure slots ACanvas8 leaves, which makes this class the first concrete one in the chain. The other
 * three are the block copies, which a linear layout can do far better than the generic per-pixel
 * implementations it inherits.
 *
 * TWO PIXELS SHARE A BYTE, and the nibble is chosen by the bitmap's odd-start flag. The byte offset
 * inside a row is `(nX + mOddNibbleStart) / 2` and the nibble is the low one when
 * `(nX ^ mOddNibbleStart)` is even. That flag is bit 13 of the halfword at ABitmap `+0x04`, which
 * `abitmap.h` already documents as mOddNibbleStart, so the field and the addressing corroborate
 * each other.
 *
 * The sibling ACanvasLin8 overrides eighteen slots where this one overrides eight, because it also
 * specialises the fills and the row operations. Four bits per pixel makes those harder rather than
 * easier, so this class inherits them.
 */
class ACanvasLin4 : public ACanvas8 {
public:
    /**
     * Construct over a bitmap.
     *
     * @param bitmap The bitmap the canvas addresses.
     * @ghidraAddress 0x006280d0
     */
    explicit ACanvasLin4(const ABitmap &bitmap);

    /**
     * Slot 1.
     *
     * The body restores ACanvas's table rather than ACanvas8's, because the inlined destructor
     * chain runs to the root and the intermediate store is dead.
     *
     * @ghidraAddress 0x00628018
     */
    virtual ~ACanvasLin4();

    /** Slot 13. Writes the stored colour into the addressed nibble. @ghidraAddress 0x00628108 */
    virtual void PutPixelNoClip(int nX, int nY);

    /** Slot 15. @ghidraAddress 0x00628180 */
    virtual void PutPixelIndexedNoClip(int nX, int nY, int nIndex);

    /** Slot 25. @ghidraAddress 0x006281f0 */
    virtual int GetPixelIndexedNoClip(int nX, int nY);

    /**
     * Slot 47. Body not yet written.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress 0x00627e88
     */
    virtual void Blit4NoClip(const ABitmap &source, int nX, int nY);

    /**
     * Slot 49. Body not yet written.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress 0x00628248
     */
    virtual void Blit8NoClip(const ABitmap &source, int nX, int nY);

    /**
     * Slot 57. Body not yet written.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress 0x006282e0
     */
    virtual void BlitRle8NoClip(const ABitmap &source, int nX, int nY);

private:
    /**
     * Build one row of the alpha mask from the transparent colour. Body not yet written.
     *
     * Non-virtual, and it fills no slot of any table in this family, which was checked against all
     * 85 entries of ACanvas, ACanvas8, ACanvasLin4, and ACanvasLin8. Its three callers are this
     * class's own three block copies, each of them confirmed by the table position it occupies, so
     * the attribution rests on a caller set entirely inside one class rather than on the title it
     * carries.
     *
     * @ghidraAddress 0x00627cf8
     */
    void BuildAlphaRowFromColorKey();
};
