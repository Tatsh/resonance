#pragma once

#include "rndartt/acanvas.h"

/** Mask of the three colour channels of an 8888 pen colour. */
constexpr unsigned int kACanvas32ChannelsMask = 0x00ffffff;

/** Alpha byte of a fully opaque 8888 colour. */
constexpr unsigned int kACanvas32AlphaOpaque = 0xff000000;

/**
 * Colour conversion for a canvas of 32 bit pixels.
 *
 * The name comes from the RTTI descriptor at 0x0086f6b0, whose mangled form is `9ACanvas32` and
 * whose single public base is ACanvas at offset zero.
 *
 * The class supplies every colour format in terms of the two 8888 accessors ACanvasLin32 provides,
 * so it has no addressing of its own. Its whole translation unit sits between 0x0062f668 and
 * 0x0062faf8, immediately around its type function at 0x0062f740.
 *
 * mColor occupies the four bytes at offset 0x24, which makes the object 0x28 bytes, the same size
 * as every other member of the family.
 *
 * The routines that write individual channels of mColor compile to byte stores, and the
 * reconstruction builds a word with shifts instead. The two agree on a little endian target,
 * which the PlayStation 2 is.
 */
class ACanvas32 : public ACanvas {
public:
    /**
     * Adopt a pixel description.
     *
     * Runs the ACanvas constructor, installs the ACanvas32 table at 0x00840468, and clears
     * mColor.
     *
     * @param bitmap The description to adopt.
     * @ghidraAddress 0x0062f790
     */
    explicit ACanvas32(const ABitmap &bitmap);

    /**
     * Set the pen colour from a palette index.
     *
     * Returns with no change when neither the bitmap nor g_pDefaultPalette supplies a palette.
     * The index is truncated to eight bits.
     *
     * @param nIndex The palette index.
     * @ghidraAddress 0x0062f8c0
     */
    void SetColorIndex(int nIndex);

    /**
     * Discard a 1555 pen colour.
     *
     * The compiled body is one return instruction. A 1555 pen colour therefore has no effect on a
     * 32 bit canvas, which every other subclass of the family honours. The omission is in the
     * original.
     *
     * @param nColor The colour the routine discards.
     * @ghidraAddress 0x0062f7c8
     */
    void SetColor15(unsigned short nColor);

    /**
     * Set the pen colour from three bytes in red, green, blue order, with full alpha.
     *
     * @param pRGB The three channel bytes.
     * @ghidraAddress 0x0062f7d0
     */
    void SetColorRGB(const unsigned char *pRGB);

    /**
     * Set the pen colour from an 8888 word.
     *
     * @param nColor The colour.
     * @ghidraAddress 0x0062f7f8
     */
    void SetColor32(unsigned int nColor);

    /**
     * Set the pen colour from a native value, which on this canvas is an 8888 word.
     *
     * The compiled body matches SetColor32() instruction for instruction, and both slots exist.
     *
     * @param nColor The colour.
     * @ghidraAddress 0x0062f800
     */
    void SetColorNative(unsigned int nColor);

    /**
     * Return the palette index nearest the pen colour.
     *
     * Prefers the inverse lookup table of the palette and searches the whole table otherwise.
     * Returns zero when no palette is available.
     *
     * @return The palette index.
     * @ghidraAddress 0x0062f668
     */
    int GetColorIndex();

    /**
     * Return the pen colour packed to 1555, with the alpha bit set.
     *
     * The alpha bit is set unconditionally rather than from the pen alpha.
     *
     * @return The colour.
     * @ghidraAddress 0x0062f808
     */
    unsigned short GetColor15();

    /**
     * Store the pen colour as three bytes in red, green, blue order.
     *
     * @param pRGB The three channel bytes to write.
     * @ghidraAddress 0x0062f840
     */
    void GetColorRGB(unsigned char *pRGB);

    /**
     * Return the pen colour as an 8888 word.
     *
     * @return The colour.
     * @ghidraAddress 0x0062f860
     */
    unsigned int GetColor32();

    /**
     * Return the pen colour in the native width, which on this canvas is an 8888 word.
     *
     * The compiled body matches GetColor32() instruction for instruction, and both slots exist.
     *
     * @return The colour.
     * @ghidraAddress 0x0062f868
     */
    unsigned int GetColorNative();

    /**
     * Store one palette index at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nIndex The palette index.
     * @ghidraAddress 0x0062f8f8
     */
    void PutPixelIndexedNoClip(int nX, int nY, int nIndex);

    /**
     * Store one 1555 colour at one point, with no clip test.
     *
     * Expands each five bit channel into the top five bits of its byte, and takes alpha from the
     * top bit of the argument.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nColor The colour.
     * @ghidraAddress 0x0062f950
     */
    void PutPixel15NoClip(int nX, int nY, unsigned short nColor);

    /**
     * Store one red, green, blue triple at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param pRGB The three channel bytes.
     * @ghidraAddress 0x0062f9b8
     */
    void PutPixelRGBNoClip(int nX, int nY, const unsigned char *pRGB);

    /**
     * Store one native value at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nColor The colour.
     * @ghidraAddress 0x0062f870
     */
    void PutPixelNativeNoClip(int nX, int nY, unsigned int nColor);

    /**
     * Read one point as a palette index, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The palette index, or zero when no palette is available.
     * @ghidraAddress 0x0062fa08
     */
    int GetPixelIndexedNoClip(int nX, int nY);

    /**
     * Read one point as a 1555 colour, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The colour, with alpha set from the top bit of the stored alpha byte.
     * @ghidraAddress 0x0062faa0
     */
    unsigned short GetPixel15NoClip(int nX, int nY);

    /**
     * Read one point into three bytes in red, green, blue order, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param pRGB The three channel bytes to write.
     * @ghidraAddress 0x0062faf8
     */
    void GetPixelRGBNoClip(int nX, int nY, unsigned char *pRGB);

    /**
     * Read one point in the native width, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The colour.
     * @ghidraAddress 0x0062f898
     */
    unsigned int GetPixelNativeNoClip(int nX, int nY);

protected:
    // ACanvasLin32 stores and reads the pen colour in four of its slots, so protected is the
    // narrowest specifier the image supports.
    unsigned int mColor; // +0x24
};
