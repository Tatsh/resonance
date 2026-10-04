#pragma once

#include "rndartt/acanvas.h"

/**
 * Colour conversion for a canvas of 8-bit indexed pixels.
 *
 * Its RTTI descriptor is at 0x008ef130, and its single public base is ACanvas at offset zero.
 *
 * THE CLASS IS ABSTRACT. Its table at 0x00841678 runs the same 85 entries as ACanvas's, so it adds
 * no virtual, and of the base's 22 pure slots it leaves three pointing at the shared pure-virtual
 * stub: 13, 15, and 25, which are the colourless store and the indexed pixel pair.
 *
 * WHICH THREE IT LEAVES IS THE DESIGN OF THE WHOLE FAMILY, and it is exact. A format class
 * implements every colour format OTHER than its own, by converting into its own, and leaves its own
 * format's pixel pair to whichever subclass knows the memory layout. This class leaves the indexed
 * pair at 15 and 25, ACanvas15 leaves the 1555 pair at 17 and 27, and ACanvas24 leaves the
 * channel-triple pair at 19 and 29. Each also leaves slot 13, the colourless store.
 *
 * This class differs from its two siblings in one slot and can explain it: the alpha builder at
 * slot 12 is supplied here and left pure by both of them, because an indexed format owns a palette
 * whose entries can be rewritten and neither of theirs does.
 *
 * Two layout classes derive from this one, ACanvasLin4 and ACanvasLin8, and their descriptors
 * record ACanvas8 as their base rather than ACanvas. Four bits and eight bits per pixel address
 * memory differently, which is why the indexed pair cannot live here.
 *
 * mColor is a single BYTE at offset 0x24, written with `sb` and read with `lbu`, where ACanvas15
 * stores a halfword and ACanvas32 a word at the same offset. For this format the native value and
 * the palette index are the same thing. SetColor8 at 0x00635bf8 and SetColorNative at 0x00635c00
 * are two distinct routines occupying two distinct table slots whose bodies are byte for byte the
 * same single store, and the two getters match in the same way.
 *
 * Every slot below is mapped from the table, by comparing each entry against ACanvas's table at
 * 0x00837dc8 at the same index. The base declares both spellings.
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
     * @ghidraAddress NTSC-U/C: 0x00635bc0
     * @ghidraAddress PAL: 0x00676750
     */
    explicit ACanvas8(const ABitmap &bitmap);

    /**
     * Slot 1.
     *
     * @ghidraAddress NTSC-U/C: 0x00635b40
     * @ghidraAddress PAL: 0x006766d0
     */
    virtual ~ACanvas8();

    /**
     * Slot 2. Stores the index as the single colour byte.
     *
     * @ghidraAddress NTSC-U/C: 0x00635bf8
     * @ghidraAddress PAL: 0x00676788
     */
    virtual void SetColor8(int nIndex);

    /**
     * Slot 3.
     *
     * @ghidraAddress NTSC-U/C: 0x00635c70
     * @ghidraAddress PAL: 0x00676800
     */
    virtual void SetColor15(unsigned short nColor);

    /**
     * Slot 4.
     *
     * @ghidraAddress NTSC-U/C: 0x00635d08
     * @ghidraAddress PAL: 0x00676898
     */
    virtual void SetColor24(const unsigned char *pRGB);

    /**
     * Slot 5.
     *
     * @ghidraAddress NTSC-U/C: 0x00635db0
     * @ghidraAddress PAL: 0x00676940
     */
    virtual void SetColor32(unsigned int nColor);

    /**
     * Slot 6. The same single store as slot 2.
     *
     * @ghidraAddress NTSC-U/C: 0x00635c00
     * @ghidraAddress PAL: 0x00676790
     */
    virtual void SetColorNative(unsigned int nColor);

    /**
     * Slot 7.
     *
     * @ghidraAddress NTSC-U/C: 0x00635c08
     * @ghidraAddress PAL: 0x00676798
     */
    virtual int GetColor8();

    /**
     * Slot 8.
     *
     * @ghidraAddress NTSC-U/C: 0x00635e30
     * @ghidraAddress PAL: 0x006769c0
     */
    virtual unsigned short GetColor15();

    /**
     * Slot 9.
     *
     * @ghidraAddress NTSC-U/C: 0x00635e78
     * @ghidraAddress PAL: 0x00676a08
     */
    virtual void GetColor24(unsigned char *pRGB);

    /**
     * Slot 10.
     *
     * @ghidraAddress NTSC-U/C: 0x00635ec0
     * @ghidraAddress PAL: 0x00676a50
     */
    virtual unsigned int GetColor32();

    /**
     * Slot 11. The same single load as slot 7.
     *
     * @ghidraAddress NTSC-U/C: 0x00635c10
     * @ghidraAddress PAL: 0x006767a0
     */
    virtual unsigned int GetColorNative();

    /**
     * Slot 12.
     *
     * The only slot with no fallback to the default palette. A canvas with no palette does nothing
     * here.
     *
     * @ghidraAddress NTSC-U/C: 0x006362d0
     * @ghidraAddress PAL: 0x00676e60
     */
    virtual void SetAlphaValues(unsigned int nColorKey);

    /**
     * Slot 17.
     *
     * @ghidraAddress NTSC-U/C: 0x00635ef8
     * @ghidraAddress PAL: 0x00676a88
     */
    virtual void DrawPixel15U(int nX, int nY, unsigned short nColor);

    /**
     * Slot 19.
     *
     * @ghidraAddress NTSC-U/C: 0x00635fd8
     * @ghidraAddress PAL: 0x00676b68
     */
    virtual void DrawPixel24U(int nX, int nY, const unsigned char *pRGB);

    /**
     * Slot 21.
     *
     * @ghidraAddress NTSC-U/C: 0x006360c8
     * @ghidraAddress PAL: 0x00676c58
     */
    virtual void DrawPixel32U(int nX, int nY, unsigned int nColor);

    /**
     * Slot 23.
     *
     * Native is the palette index for this format, so the store narrows and forwards to
     * DrawPixel8U().
     *
     * @ghidraAddress NTSC-U/C: 0x00635c18
     * @ghidraAddress PAL: 0x006767a8
     */
    virtual void DrawPixelNativeU(int nX, int nY, unsigned int nColor);

    /**
     * Slot 27.
     *
     * @ghidraAddress NTSC-U/C: 0x00636190
     * @ghidraAddress PAL: 0x00676d20
     */
    virtual unsigned short GetPixel15U(int nX, int nY);

    /**
     * Slot 29.
     *
     * @ghidraAddress NTSC-U/C: 0x006361f8
     * @ghidraAddress PAL: 0x00676d88
     */
    virtual void GetPixel24U(int nX, int nY, unsigned char *pRGB);

    /**
     * Slot 31.
     *
     * The base's slot 31 is the colourless read rather than a format-specific one. Slot 21 is the
     * same case on the store side.
     *
     * @ghidraAddress NTSC-U/C: 0x00636270
     * @ghidraAddress PAL: 0x00676e00
     */
    virtual unsigned int GetPixel32U(int nX, int nY);

    /**
     * Slot 33.
     *
     * Native is the palette index for this format, so the read forwards the result of
     * GetPixel8U() unchanged.
     *
     * @ghidraAddress NTSC-U/C: 0x00635c48
     * @ghidraAddress PAL: 0x006767d8
     */
    virtual unsigned int GetPixelNativeU(int nX, int nY);

protected:
    // The colour the store slots write, one byte where the wider formats keep a halfword or a word.
    unsigned char mColor; // +0x24
};
