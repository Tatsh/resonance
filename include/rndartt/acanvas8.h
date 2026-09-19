#pragma once

#include "rndartt/acanvas.h"

/**
 * Colour conversion for a canvas of 8-bit indexed pixels.
 *
 * The name comes from the RTTI descriptor at 0x008ef130, whose mangled form is `8ACanvas8` and
 * whose single public base is ACanvas at offset zero.
 *
 * THE CLASS IS ABSTRACT, and it leaves more pure than its siblings do. Its table at 0x00841678 runs
 * the same 85 entries as ACanvas's, so it adds no virtual, and it fills 21 of the base's pure slots
 * while leaving slots 13, 15, and 25 pointing at the shared pure-virtual stub. Those three are the
 * two indexed pixel accessors and the colourless store, which are the three that depend on how
 * pixels are packed rather than on the colour format.
 *
 * Two layout classes derive from this one, ACanvasLin4 and ACanvasLin8, and their descriptors
 * record ACanvas8 as their base rather than ACanvas. Four bits and eight bits per pixel address
 * memory differently, which is why this class cannot implement those three slots and its 1555
 * sibling, with one layout subclass, leaves only slot 13.
 *
 * mColor is a single BYTE at offset 0x24, written with `sb` and read with `lbu`, where ACanvas15
 * keeps a halfword and ACanvas32 a word at the same offset. For this format the native value and
 * the palette index are the same thing, which the binary states outright: SetColorIndex at
 * 0x00635bf8 and SetColorNative at 0x00635c00 are two distinct routines occupying two distinct
 * table slots whose bodies are byte for byte the same single store, and the same holds for the two
 * getters.
 *
 * Every slot below is mapped from the table rather than from its routine title, by comparing each
 * entry against ACanvas's table at 0x00837dc8 at the same index. That matters here: the titles the
 * program carries suggest a different set of members from the one the slots prove, and the base
 * declares both spellings.
 *
 * No body is written yet. The conversion bodies are recoverable and the three pure slots are not
 * this class's to supply.
 */
class ACanvas8 : public ACanvas {
public:
    /**
     * Construct over a bitmap.
     *
     * @param bitmap The bitmap the canvas addresses.
     * @ghidraAddress 0x00635bc0
     */
    explicit ACanvas8(const ABitmap &bitmap);

    /** Slot 1. @ghidraAddress 0x00635b40 */
    virtual ~ACanvas8();

    /** Slot 2. Stores the index as the single colour byte. @ghidraAddress 0x00635bf8 */
    virtual void SetColorIndex(int nIndex);

    /** Slot 3. @ghidraAddress 0x00635c70 */
    virtual void SetColor15(unsigned short nColor);

    /** Slot 4. @ghidraAddress 0x00635d08 */
    virtual void SetColorRGB(const unsigned char *pRGB);

    /** Slot 5. @ghidraAddress 0x00635db0 */
    virtual void SetColor32(unsigned int nColor);

    /** Slot 6. The same single store as slot 2. @ghidraAddress 0x00635c00 */
    virtual void SetColorNative(unsigned int nColor);

    /** Slot 7. @ghidraAddress 0x00635c08 */
    virtual int GetColorIndex();

    /** Slot 8. @ghidraAddress 0x00635e30 */
    virtual unsigned short GetColor15();

    /** Slot 9. @ghidraAddress 0x00635e78 */
    virtual void GetColorRGB(unsigned char *pRGB);

    /** Slot 10. @ghidraAddress 0x00635ec0 */
    virtual unsigned int GetColor32();

    /** Slot 11. The same single load as slot 7. @ghidraAddress 0x00635c10 */
    virtual unsigned int GetColorNative();

    /** Slot 12. @ghidraAddress 0x006362d0 */
    virtual void BuildAlphaFromColorKey(unsigned int nColorKey);

    /** Slot 17. @ghidraAddress 0x00635ef8 */
    virtual void PutPixel15NoClip(int nX, int nY, unsigned short nColor);

    /** Slot 19. @ghidraAddress 0x00635fd8 */
    virtual void PutPixelRGBNoClip(int nX, int nY, const unsigned char *pRGB);

    /** Slot 21. @ghidraAddress 0x006360c8 */
    virtual void PutPixelNoClip(int nX, int nY, unsigned int nColor);

    /** Slot 23. @ghidraAddress 0x00635c18 */
    virtual void PutPixelNativeNoClip(int nX, int nY, unsigned int nColor);

    /** Slot 27. @ghidraAddress 0x00636190 */
    virtual unsigned short GetPixel15NoClip(int nX, int nY);

    /** Slot 29. @ghidraAddress 0x006361f8 */
    virtual void GetPixelRGBNoClip(int nX, int nY, unsigned char *pRGB);

    /**
     * Slot 31.
     *
     * The program titles this routine after the 32-bit format, and the base's slot 31 is the
     * colourless read rather than a format-specific one, so the title is not the member. Slot 21
     * is the same case on the store side.
     *
     * @ghidraAddress 0x00636270
     */
    virtual unsigned int GetPixelNoClip(int nX, int nY);

    /** Slot 33. @ghidraAddress 0x00635c48 */
    virtual unsigned int GetPixelNativeNoClip(int nX, int nY);

protected:
    // The colour the store slots write, one byte where the wider formats keep a halfword or a word.
    unsigned char mColor; // +0x24
};
