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
     * Write one row of indices into this canvas at four bits per pixel. Body not yet written.
     *
     * Non-virtual, and it fills no slot of any table in this family, which was checked against all
     * 85 entries of ACanvas, ACanvas8, ACanvasLin4, and ACanvasLin8. Its three callers are this
     * class's own three block copies, each of them confirmed by the table position it occupies, so
     * the attribution rests on a caller set entirely inside one class rather than on the title it
     * carries.
     *
     * The program titled it for an alpha mask build. All three callers instead feed it a row of
     * one byte per pixel and a destination position, which is what the retitle records. The fifth
     * argument arrives in a register the decompiler does not bind, so the prototype read as taking
     * no arguments at all.
     *
     * THE BODY IS UNRESOLVED, not merely unwritten. Its bulk loop packs two source bytes into one
     * destination byte, which is the expected conversion. Its leading and trailing per-pixel paths
     * instead store a literal 0 or 1 as a whole byte, which was confirmed by decoding the raw
     * instruction words rather than by reading a listing. Those two destination meanings cannot
     * both describe the same buffer. The per-pixel paths also read the destination byte and mask it
     * by nibble phase, preserving the neighbouring nibble, and then discard the result by storing a
     * constant. The phase-zero mask can never match when the byte only ever stores 0 or 1, so that
     * test is dead.
     *
     * @param pSource The source bitmap the row came from, read for its transparency fields.
     * @param pRow One byte per pixel, already expanded by the caller.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress 0x00627cf8
     */
    void WriteIndexedRow(const ABitmap *pSource, const unsigned char *pRow, int nX, int nY);
};
