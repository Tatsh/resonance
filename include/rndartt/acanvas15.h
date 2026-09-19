#pragma once

#include "rndartt/acanvas.h"

/**
 * Colour conversion for a canvas of 1555 pixels.
 *
 * The name comes from the RTTI descriptor at 0x008ef130, whose mangled form is `9ACanvas15` and
 * whose single public base is ACanvas at offset zero.
 *
 * The class is the counterpart of ACanvas32: it supplies every colour format in terms of the two
 * 1555 accessors ACanvasLin15 provides, so it has no addressing of its own. Its translation unit
 * sits between 0x0062fb40 and 0x006300d0, around its type function at 0x0062fb70.
 *
 * mColor is the halfword at offset 0x24 rather than the word ACanvas32 stores there, which is the
 * only layout difference in the family; the object is 0x28 bytes either way.
 *
 * Every pixel slot converts and then forwards to the layout class, reading the destination from
 * the table rather than calling a sibling directly: the store slots dispatch through the entry at
 * table offset 0x88 and the read slots through the one at 0xd8. So each format costs one
 * conversion body and one dispatch, never a second addressing body.
 */
class ACanvas15 : public ACanvas {
public:
    /**
     * Adopt a pixel description.
     *
     * Runs the ACanvas constructor, installs the ACanvas15 table at 0x00840748, and clears
     * mColor.
     *
     * @param bitmap The description to adopt.
     * @ghidraAddress 0x0062fbc0
     */
    explicit ACanvas15(const ABitmap &bitmap);

    /**
     * Set the pen colour from a palette index.
     *
     * Returns with no change when neither the bitmap nor g_pDefaultPalette supplies a palette.
     * The index is truncated to eight bits, and the 8888 entry is packed to 1555.
     *
     * @param nIndex The palette index.
     * @ghidraAddress 0x0062fd48
     */
    void SetColorIndex(int nIndex);

    /**
     * Set the pen colour from a 1555 halfword, which on this canvas is stored verbatim.
     *
     * @param nColor The colour.
     * @ghidraAddress 0x0062fbf8
     */
    void SetColor15(unsigned short nColor);

    /**
     * Set the pen colour from three bytes in red, green, blue order, with the alpha bit set.
     *
     * The alpha bit is set unconditionally rather than taken from any argument.
     *
     * @param pRGB The three channel bytes.
     * @ghidraAddress 0x0062fc00
     */
    void SetColorRGB(const unsigned char *pRGB);

    /**
     * Set the pen colour from an 8888 word.
     *
     * @param nColor The colour.
     * @ghidraAddress 0x0062fc38
     */
    void SetColor32(unsigned int nColor);

    /**
     * Set the pen colour from a native value, which on this canvas is a 1555 halfword.
     *
     * The compiled body matches SetColor15() instruction for instruction, and both slots exist.
     *
     * @param nColor The colour.
     * @ghidraAddress 0x0062fc70
     */
    void SetColorNative(unsigned int nColor);

    /**
     * Return the palette index nearest the pen colour.
     *
     * Prefers the inverse lookup table of the palette, indexing it with the pen colour and the
     * alpha bit cleared, and searches the whole table otherwise. Returns zero when no palette is
     * available.
     *
     * @return The palette index.
     * @ghidraAddress 0x0062fd98
     */
    int GetColorIndex();

    /**
     * Return the pen colour, which on this canvas is already 1555.
     *
     * @return The colour.
     * @ghidraAddress 0x0062fc78
     */
    unsigned short GetColor15();

    /**
     * Store the pen colour as three bytes in red, green, blue order.
     *
     * Each five bit channel moves into the top five bits of its byte, so the low three bits of
     * every channel read back as zero.
     *
     * @param pRGB The three channel bytes to write.
     * @ghidraAddress 0x0062fc80
     */
    void GetColorRGB(unsigned char *pRGB);

    /**
     * Return the pen colour as an 8888 word.
     *
     * Alpha is all ones when the 1555 alpha bit is set and zero otherwise.
     *
     * @return The colour.
     * @ghidraAddress 0x0062fca8
     */
    unsigned int GetColor32();

    /**
     * Return the pen colour in the native width, which on this canvas is a 1555 halfword.
     *
     * The compiled body matches GetColor15() instruction for instruction, and both slots exist.
     *
     * @return The colour.
     * @ghidraAddress 0x0062fce8
     */
    unsigned int GetColorNative();

    /**
     * Store one palette index at one point, with no clip test.
     *
     * Returns with no store when no palette is available.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nIndex The palette index.
     * @ghidraAddress 0x0062fe28
     */
    void PutPixelIndexedNoClip(int nX, int nY, int nIndex);

    /**
     * Store one red, green, blue triple at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param pRGB The three channel bytes.
     * @ghidraAddress 0x0062fec0
     */
    void PutPixelRGBNoClip(int nX, int nY, const unsigned char *pRGB);

    /**
     * Store one 8888 colour at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nColor The colour.
     * @ghidraAddress 0x0062ff18
     */
    void PutPixel32NoClip(int nX, int nY, unsigned int nColor);

    /**
     * Store one native value at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nColor The colour, truncated to a halfword.
     * @ghidraAddress 0x0062fcf0
     */
    void PutPixelNativeNoClip(int nX, int nY, unsigned int nColor);

    /**
     * Read one point as a palette index, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The palette index, or zero when no palette is available.
     * @ghidraAddress 0x0062ff70
     */
    int GetPixelIndexedNoClip(int nX, int nY);

    /**
     * Read one point into three bytes in red, green, blue order, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param pRGB The three channel bytes to write.
     * @ghidraAddress 0x00630020
     */
    void GetPixelRGBNoClip(int nX, int nY, unsigned char *pRGB);

    /**
     * Read one point as an 8888 colour, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The colour.
     * @ghidraAddress 0x00630078
     */
    unsigned int GetPixel32NoClip(int nX, int nY);

    /**
     * Read one point in the native width, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The colour.
     * @ghidraAddress 0x0062fd20
     */
    unsigned int GetPixelNativeNoClip(int nX, int nY);

protected:
    // ACanvasLin15 stores and reads the pen colour in its own slots, so protected is the narrowest
    // specifier the image supports, matching ACanvas32.
    unsigned short mColor; // +0x24
};
