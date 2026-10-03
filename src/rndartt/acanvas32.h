#pragma once

#include "rndartt/acanvas.h"

/** Mask of the three colour channels of an 8888 pen colour. */
constexpr unsigned int kACanvas32ChannelsMask = 0x00ffffff;

/** Alpha byte of a fully opaque 8888 colour. */
constexpr unsigned int kACanvas32AlphaOpaque = 0xff000000;

/**
 * Colour conversion for a canvas of 32 bit pixels.
 *
 * Its RTTI descriptor is at 0x0086f6b0, and its single public base is ACanvas at offset zero.
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
     * @ghidraAddress NTSC-U/C: 0x0062f790
     * @ghidraAddress PAL: 0x00670320
     */
    explicit ACanvas32(const ABitmap &bitmap);

    /**
     * Set the pen colour from a palette index.
     *
     * Returns with no change when neither the bitmap nor ACanvas::palDefault supplies a palette.
     * The index is truncated to eight bits.
     *
     * @param nIndex The palette index.
     * @ghidraAddress NTSC-U/C: 0x0062f8c0
     * @ghidraAddress PAL: 0x00670450
     */
    void SetColor8(int nIndex);

    /**
     * Discard a 1555 pen colour.
     *
     * The compiled body is one return instruction. A 1555 pen colour therefore has no effect on a
     * 32 bit canvas, which every other subclass of the family honours. The omission is in the
     * original.
     *
     * @param nColor The colour the routine discards.
     * @ghidraAddress NTSC-U/C: 0x0062f7c8
     * @ghidraAddress PAL: 0x00670358
     */
    void SetColor15(unsigned short nColor);

    /**
     * Set the pen colour from three bytes in red, green, blue order, with full alpha.
     *
     * @param pRGB The three channel bytes.
     * @ghidraAddress NTSC-U/C: 0x0062f7d0
     * @ghidraAddress PAL: 0x00670360
     */
    void SetColor24(const unsigned char *pRGB);

    /**
     * Set the pen colour from an 8888 word.
     *
     * @param nColor The colour.
     * @ghidraAddress NTSC-U/C: 0x0062f7f8
     * @ghidraAddress PAL: 0x00670388
     */
    void SetColor32(unsigned int nColor);

    /**
     * Set the pen colour from a native value, which on this canvas is an 8888 word.
     *
     * The compiled body matches SetColor32() instruction for instruction, and both slots exist.
     *
     * @param nColor The colour.
     * @ghidraAddress NTSC-U/C: 0x0062f800
     * @ghidraAddress PAL: 0x00670390
     */
    void SetColorNative(unsigned int nColor);

    /**
     * Return the palette index nearest the pen colour.
     *
     * Prefers the inverse lookup table of the palette and searches the whole table otherwise.
     * Returns zero when no palette is available.
     *
     * @return The palette index.
     * @ghidraAddress NTSC-U/C: 0x0062f668
     * @ghidraAddress PAL: 0x006701f8
     */
    int GetColor8();

    /**
     * Return the pen colour packed to 1555, with the alpha bit set.
     *
     * The alpha bit is set unconditionally rather than from the pen alpha.
     *
     * @return The colour.
     * @ghidraAddress NTSC-U/C: 0x0062f808
     * @ghidraAddress PAL: 0x00670398
     */
    unsigned short GetColor15();

    /**
     * Store the pen colour as three bytes in red, green, blue order.
     *
     * @param pRGB The three channel bytes to write.
     * @ghidraAddress NTSC-U/C: 0x0062f840
     * @ghidraAddress PAL: 0x006703d0
     */
    void GetColor24(unsigned char *pRGB);

    /**
     * Return the pen colour as an 8888 word.
     *
     * @return The colour.
     * @ghidraAddress NTSC-U/C: 0x0062f860
     * @ghidraAddress PAL: 0x006703f0
     */
    unsigned int GetColor32();

    /**
     * Return the pen colour in the native width, which on this canvas is an 8888 word.
     *
     * The compiled body matches GetColor32() instruction for instruction, and both slots exist.
     *
     * @return The colour.
     * @ghidraAddress NTSC-U/C: 0x0062f868
     * @ghidraAddress PAL: 0x006703f8
     */
    unsigned int GetColorNative();

    /**
     * Store one palette index at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nIndex The palette index.
     * @ghidraAddress NTSC-U/C: 0x0062f8f8
     * @ghidraAddress PAL: 0x00670488
     */
    void DrawPixel8U(int nX, int nY, int nIndex);

    /**
     * Store one 1555 colour at one point, with no clip test.
     *
     * Expands each five bit channel into the top five bits of its byte, and takes alpha from the
     * top bit of the argument.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nColor The colour.
     * @ghidraAddress NTSC-U/C: 0x0062f950
     * @ghidraAddress PAL: 0x006704e0
     */
    void DrawPixel15U(int nX, int nY, unsigned short nColor);

    /**
     * Store one red, green, blue triple at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param pRGB The three channel bytes.
     * @ghidraAddress NTSC-U/C: 0x0062f9b8
     * @ghidraAddress PAL: 0x00670548
     */
    void DrawPixel24U(int nX, int nY, const unsigned char *pRGB);

    /**
     * Store one native value at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nColor The colour.
     * @ghidraAddress NTSC-U/C: 0x0062f870
     * @ghidraAddress PAL: 0x00670400
     */
    void DrawPixelNativeU(int nX, int nY, unsigned int nColor);

    /**
     * Read one point as a palette index, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The palette index, or zero when no palette is available.
     * @ghidraAddress NTSC-U/C: 0x0062fa08
     * @ghidraAddress PAL: 0x00670598
     */
    int GetPixel8U(int nX, int nY);

    /**
     * Read one point as a 1555 colour, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The colour, with alpha set from the top bit of the stored alpha byte.
     * @ghidraAddress NTSC-U/C: 0x0062faa0
     * @ghidraAddress PAL: 0x00670630
     */
    unsigned short GetPixel15U(int nX, int nY);

    /**
     * Read one point into three bytes in red, green, blue order, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param pRGB The three channel bytes to write.
     * @ghidraAddress NTSC-U/C: 0x0062faf8
     * @ghidraAddress PAL: 0x00670688
     */
    void GetPixel24U(int nX, int nY, unsigned char *pRGB);

    /**
     * Read one point in the native width, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The colour.
     * @ghidraAddress NTSC-U/C: 0x0062f898
     * @ghidraAddress PAL: 0x00670428
     */
    unsigned int GetPixelNativeU(int nX, int nY);

protected:
    // ACanvasLin32 stores and reads the pen colour in four of its slots, so protected is the
    // narrowest specifier the image supports.
    unsigned int mColor; // +0x24
};
