#pragma once

#include <vector>

#include "rndartt/abitmap.h"
#include "rndartt/arect.h"
#include "rndartt/arle8reader.h"

class APalette;
struct AFont;
struct APoint;
struct APolygon;
struct Color;

/** Fractional bits in the coordinates DrawLine() and DrawTmapRow8U() take. */
constexpr int kACanvasFractionBits = 8;

/**
 * Bits ACanvas::ClipCode() returns.
 *
 * The four bits form a Cohen and Sutherland outcode against the canvas clip rectangle. A point
 * inside the rectangle produces zero.
 */
enum ACanvasClipCode {
    kACanvasClipLeft = 1,  /*!< The point sits left of the clip rectangle. */
    kACanvasClipRight = 2, /*!< The point sits at or right of the clip rectangle. */
    kACanvasClipAbove = 4, /*!< The point sits above the clip rectangle. */
    kACanvasClipBelow = 8  /*!< The point sits at or below the clip rectangle. */
};

/**
 * Drawing surface over an ABitmap, with a clip rectangle.
 *
 * Its RTTI descriptor is at 0x0086f660 and lacks a base class. The virtual function table
 * pointer sits after the data members at offset 0x20, the position the toolchain uses for a class
 * with no base.
 *
 * The table at 0x00837dc8 has 85 slots and terminates on the all zero entry at 0x00838070. Slot 0
 * is the type function at 0x005ead28 and slot 1 is the destructor. 22 of the remaining slots point
 * at the shared pure virtual stub at 0x005381a8. Every declaration below appears in slot order,
 * which is the order the toolchain assigns, so the position of a member in this class reproduces
 * its index.
 *
 * Five subclasses implement the drawing operations, one per accepted ABitmapFormat code, and each
 * derives through an intermediate width class: ACanvasLin4 through ACanvas8, ACanvasLin8 through
 * ACanvas8, ACanvasLin15 through ACanvas15, ACanvasLin24 through ACanvas24, and ACanvasLin32
 * through ACanvas32. Every object in the family is 0x28 bytes, the subclass pen colour at offset
 * 0x24 being one byte wide for the two indexed formats, two for the 15 bit format, and four for
 * the 24 and 32 bit formats.
 *
 * The division of labour between the two subclass levels is clean. The width class implements the
 * five pen colour setters, the five pen colour getters, and the eight per format pixel accessors,
 * all in terms of the two accessors that use its own storage width. The layout class implements
 * those two, plus whichever bulk operation benefits from direct addressing.
 *
 * Every operation appears twice, once with the clip test and once without. The unclipped form
 * takes the U suffix. For the pixel accessors and the row, column, and rectangle fills the
 * unclipped form is pure virtual here and the clipped form is implemented here in terms of it. For
 * the copy and read slots both forms are implemented here, the clipped one calling ClipBitmap()
 * and then the unclipped one.
 *
 * Six colour formats arrive at a pixel. The pen colour form uses no argument. The indexed form
 * takes a
 * palette index, the 1555 form a halfword, the RGB form three bytes, the 8888 form a word, and the
 * native form a value in the canvas storage width. On a 32 bit canvas the 8888 and native forms
 * coincide, and ACanvas32 implements the native pair by forwarding to the 8888 pair.
 *
 * Both data members are public. Rnd::Font::ComputeCharUV() reads mBitmap.mWidth and
 * mBitmap.mHeight from outside the hierarchy and the image supplies no accessor, so the access
 * rule gives public. A friend declaration for Rnd::Font fits the image equally well. mClip has no
 * reader outside the hierarchy and would otherwise be protected, and it shares the public section
 * so that the recovered order of the two is preserved.
 */
class ACanvas {
public:
    struct ARowInfo;
    struct AScaledRowInfo;
    struct Rle8Clip;

    /**
     * Adopt a pixel description.
     *
     * The description is copied whole and the clip rectangle is set to the full bitmap, from the
     * origin to mWidth by mHeight. Every subclass constructor calls this first and then installs
     * its own virtual function table.
     *
     * @param bitmap The description to adopt.
     * @ghidraAddress NTSC-U/C: 0x005eb1a0
     * @ghidraAddress PAL: 0x0062d2e8
     */
    explicit ACanvas(const ABitmap &bitmap);

    /**
     * Construct the linear canvas subclass that draws a bitmap's format.
     *
     * Copies the description. When asked to allocate, the copy gains a row stride derived from
     * its format, `(mWidth + 2) / 2` for kABitmapFormatLinear4 and mWidth times the matching entry
     * of g_abBitmapBytesPerPixel otherwise, and a pixel rectangle allocated with the source file
     * and line as its tag. The copy's mByteCount is not updated. The format code then
     * selects the subclass through the jump table at 0x00837d90.
     *
     * The byte count passed to the allocator is a 64-bit product of the height and the stride,
     * computed through the helper at 0x00600270 and truncated to 32 bits.
     *
     * @param bitmap The description to copy.
     * @param bAllocatePixels Whether to allocate a fresh pixel rectangle for the copy.
     * @return The new canvas, or null for a format code of kABitmapFormatCount or more, or when
     *         the allocation returns null.
     * @ghidraAddress NTSC-U/C: 0x005e8bc8
     * @ghidraAddress PAL: 0x0062ad10
     */
    static ACanvas *NewCompatibleCanvas(const ABitmap &bitmap, bool bAllocatePixels);

    /**
     * Construct a canvas over a fresh pixel rectangle shaped like a bitmap.
     *
     * Rewrites a format code of kABitmapFormatRle8 to kABitmapFormatLinear8 in a copy, then calls
     * NewCompatibleCanvas() with allocation requested. The image has no caller.
     *
     * @param bitmap The description to copy.
     * @return The new canvas, or null.
     * @ghidraAddress NTSC-U/C: 0x005eb200
     * @ghidraAddress PAL: 0x0062d348
     */
    static ACanvas *NewCompatibleLinearCanvas(const ABitmap &bitmap);

    /**
     * Construct a canvas over a rectangle of a bitmap's pixels, sharing them.
     *
     * Builds the rectangle through the sub-rectangle ABitmap constructor and selects the subclass
     * through the jump table at 0x00837db0. Unlike NewCompatibleCanvas(), a kABitmapFormatLinear4
     * rectangle gets an ACanvasLin8, and kABitmapFormatRle8 gets no canvas. The image has no
     * caller.
     *
     * @param source The bitmap whose pixels the canvas draws into.
     * @param nX The left column of the rectangle.
     * @param nY The top row of the rectangle.
     * @param nWidth The width in pixels.
     * @param nHeight The height in pixels.
     * @return The new canvas, or null for a format code of kABitmapFormatRle8 or more.
     * @ghidraAddress NTSC-U/C: 0x005e8e38
     * @ghidraAddress PAL: 0x0062af80
     */
    static ACanvas *SubCanvas(const ABitmap &source, int nX, int nY, int nWidth, int nHeight);

    /**
     * Reduce this canvas's 32 bit pixels to palette indices over ramps of the given colours.
     *
     * One NormalKey per distinct hue is collected from colors through
     * NormalKey::InsertUniqueNormalKey(), and the static APalette::BuildRampPalette() writes their
     * ramps into dest's palette. Every source pixel then writes one byte to dest. A pixel with zero
     * alpha becomes index 0. Any other pixel is matched to the key whose ratios have the least
     * summed absolute difference from its own, the first such key winning a tie. Its shade is
     * `(int)(pixel scale / key scale / 17 + 0.5)`, and a shade of 0 becomes index 16 while any
     * other becomes 16 times the key's position plus the shade, capped at 15. With no colours,
     * dest's first width times height bytes are cleared. The pixel count is this canvas's width
     * times height, and dest is assumed to be as large. MetRenderer's routine at `0x001716d0` is
     * the one caller, passing the locked canvases of two textures. The name is inferred.
     *
     * @param dest The eight bit canvas whose pixels and palette are written.
     * @param colors The colours whose hues the palette covers. Their alpha is not read.
     * @ghidraAddress NTSC-U/C: 0x00557af8
     * @ghidraAddress PAL: 0x00598c50
     */
    void QuantizeToRamps(ACanvas &dest, const std::vector<const Color *> &colors) const;

    /**
     * Copy a source bitmap, clipped, choosing the slot by source format.
     *
     * Calls through the six entry table of pointers to member functions at 0x0077dcc8, the same
     * table DrawChar() uses, whose entries are DrawBitmapLin4() through DrawBitmapLin32() and then
     * DrawBitmapRle8(). The one caller in the image is Rnd::Movie::OnChunk() at 0x005cf4c0, outside
     * the art library.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress NTSC-U/C: 0x005ec1d8
     * @ghidraAddress PAL: 0x0062e320
     */
    void DrawBitmap(const ABitmap &source, int nX, int nY);

    /**
     * Release the canvas.
     *
     * The compiled body reinstalls this class's virtual function table before releasing. That is
     * what the toolchain emits for a base destructor. The pixel rectangle is not released here.
     *
     * Every subclass destructor compiles to the same body, which installs this class's table and
     * releases the object when the deleting flag is set. ACanvas15, ACanvas32, and ACanvasLin32
     * leave theirs implicit (0x0062fb40, 0x0062f710, and 0x006141a0), and the other subclasses
     * declare one.
     *
     * @ghidraAddress NTSC-U/C: 0x005ead68
     * @ghidraAddress PAL: 0x0062ceb0
     */
    virtual ~ACanvas();

    /**
     * Set the pen colour from a palette index.
     *
     * @param nIndex The palette index.
     */
    virtual void SetColor8(int nIndex) = 0;

    /**
     * Set the pen colour from a 1555 halfword.
     *
     * @param nColor The colour, five bits per channel with alpha in the top bit.
     */
    virtual void SetColor15(unsigned short nColor) = 0;

    /**
     * Set the pen colour from three bytes in red, green, blue order.
     *
     * Alpha is forced to full on every subclass that stores one.
     *
     * @param pRGB The three channel bytes.
     */
    virtual void SetColor24(const unsigned char *pRGB) = 0;

    /**
     * Set the pen colour from an 8888 word.
     *
     * @param nColor The colour, one byte per channel in red, green, blue, alpha order.
     */
    virtual void SetColor32(unsigned int nColor) = 0;

    /**
     * Set the pen colour from a value already in the canvas storage width.
     *
     * @param nColor The colour in the canvas pixel format.
     */
    virtual void SetColorNative(unsigned int nColor) = 0;

    /**
     * Return the palette index nearest the pen colour.
     *
     * @return The palette index.
     */
    virtual int GetColor8() = 0;

    /**
     * Return the pen colour packed to 1555.
     *
     * @return The colour, five bits per channel with alpha in the top bit.
     */
    virtual unsigned short GetColor15() = 0;

    /**
     * Store the pen colour as three bytes in red, green, blue order.
     *
     * @param pRGB The three channel bytes to write.
     */
    virtual void GetColor24(unsigned char *pRGB) = 0;

    /**
     * Return the pen colour as an 8888 word.
     *
     * @return The colour, one byte per channel.
     */
    virtual unsigned int GetColor32() = 0;

    /**
     * Return the pen colour in the canvas storage width.
     *
     * @return The colour in the canvas pixel format.
     */
    virtual unsigned int GetColorNative() = 0;

    /**
     * Rewrite every alpha byte of the pixel rectangle from a colour key.
     *
     * A pixel whose colour channels equal the key loses its alpha, and every other pixel gains
     * full alpha. The two indexed subclasses implement the slot over their own storage width.
     *
     * @param nColorKey The colour to treat as transparent.
     */
    virtual void SetAlphaValues(unsigned int nColorKey) = 0;

    /**
     * Store the pen colour at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     */
    virtual void DrawPixelU(int nX, int nY) = 0;

    /**
     * Store the pen colour at one point.
     *
     * Returns with no store for a point outside the clip rectangle.
     *
     * @param nX The column.
     * @param nY The row.
     * @ghidraAddress NTSC-U/C: 0x005eb520
     * @ghidraAddress PAL: 0x0062d668
     */
    virtual void DrawPixel(int nX, int nY);

    /**
     * Store one palette index at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nIndex The palette index.
     */
    virtual void DrawPixel8U(int nX, int nY, int nIndex) = 0;

    /**
     * Store one palette index at one point, clipped.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nIndex The palette index.
     * @ghidraAddress NTSC-U/C: 0x005eb590
     * @ghidraAddress PAL: 0x0062d6d8
     */
    virtual void DrawPixel8(int nX, int nY, int nIndex);

    /**
     * Store one 1555 colour at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nColor The colour.
     */
    virtual void DrawPixel15U(int nX, int nY, unsigned short nColor) = 0;

    /**
     * Store one 1555 colour at one point, clipped.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nColor The colour.
     * @ghidraAddress NTSC-U/C: 0x005eb608
     * @ghidraAddress PAL: 0x0062d750
     */
    virtual void DrawPixel15(int nX, int nY, unsigned short nColor);

    /**
     * Store one red, green, blue triple at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param pRGB The three channel bytes.
     */
    virtual void DrawPixel24U(int nX, int nY, const unsigned char *pRGB) = 0;

    /**
     * Store one red, green, blue triple at one point, clipped.
     *
     * @param nX The column.
     * @param nY The row.
     * @param pRGB The three channel bytes.
     * @ghidraAddress NTSC-U/C: 0x005eb680
     * @ghidraAddress PAL: 0x0062d7c8
     */
    virtual void DrawPixel24(int nX, int nY, const unsigned char *pRGB);

    /**
     * Store one 8888 colour at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nColor The colour.
     */
    virtual void DrawPixel32U(int nX, int nY, unsigned int nColor) = 0;

    /**
     * Store one 8888 colour at one point, clipped.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nColor The colour.
     * @ghidraAddress NTSC-U/C: 0x005eb6f0
     * @ghidraAddress PAL: 0x0062d838
     */
    virtual void DrawPixel32(int nX, int nY, unsigned int nColor);

    /**
     * Store one value in the canvas storage width at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nColor The colour in the canvas pixel format.
     */
    virtual void DrawPixelNativeU(int nX, int nY, unsigned int nColor) = 0;

    /**
     * Store one value in the canvas storage width at one point, clipped.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nColor The colour in the canvas pixel format.
     * @ghidraAddress NTSC-U/C: 0x005eb760
     * @ghidraAddress PAL: 0x0062d8a8
     */
    virtual void DrawPixelNative(int nX, int nY, unsigned int nColor);

    /**
     * Read one point as a palette index, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The palette index.
     */
    virtual int GetPixel8U(int nX, int nY) = 0;

    /**
     * Read one point as a palette index, clipped.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The palette index, or zero when the point is clipped away.
     * @ghidraAddress NTSC-U/C: 0x005eb7d0
     * @ghidraAddress PAL: 0x0062d918
     */
    virtual int GetPixel8(int nX, int nY);

    /**
     * Read one point as a 1555 colour, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The colour.
     */
    virtual unsigned short GetPixel15U(int nX, int nY) = 0;

    /**
     * Read one point as a 1555 colour, clipped.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The colour, or zero when the point is clipped away.
     * @ghidraAddress NTSC-U/C: 0x005eb848
     * @ghidraAddress PAL: 0x0062d990
     */
    virtual unsigned short GetPixel15(int nX, int nY);

    /**
     * Read one point into three bytes in red, green, blue order, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param pRGB The three channel bytes to write.
     */
    virtual void GetPixel24U(int nX, int nY, unsigned char *pRGB) = 0;

    /**
     * Read one point into three bytes in red, green, blue order, clipped.
     *
     * Clears the three bytes for a point outside the clip rectangle.
     *
     * @param nX The column.
     * @param nY The row.
     * @param pRGB The three channel bytes to write.
     * @ghidraAddress NTSC-U/C: 0x005eb8c0
     * @ghidraAddress PAL: 0x0062da08
     */
    virtual void GetPixel24(int nX, int nY, unsigned char *pRGB);

    /**
     * Read one point as an 8888 colour, with no clip test.
     *
     * ACanvasLin32 implements the slot at 0x00614350 as a single word load at the row stride times
     * the row plus four times the column, which settles the result as one 32-bit word. Its
     * signedness is not recoverable.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The colour.
     */
    virtual unsigned int GetPixel32U(int nX, int nY) = 0;

    /**
     * Read one point as an 8888 colour, clipped.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The colour, or zero when the point is clipped away.
     * @ghidraAddress NTSC-U/C: 0x005eb948
     * @ghidraAddress PAL: 0x0062da90
     */
    virtual unsigned int GetPixel32(int nX, int nY);

    /**
     * Read one point in the canvas storage width, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The value in the canvas pixel format.
     */
    virtual unsigned int GetPixelNativeU(int nX, int nY) = 0;

    /**
     * Read one point in the canvas storage width, clipped.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The value, or zero when the point is clipped away.
     * @ghidraAddress NTSC-U/C: 0x005eb9c0
     * @ghidraAddress PAL: 0x0062db08
     */
    virtual unsigned int GetPixelNative(int nX, int nY);

    /**
     * Fill part of one row with the pen colour, with no clip test.
     *
     * @param nY The row.
     * @param nLeft The first column.
     * @param nRight One past the last column.
     * @ghidraAddress NTSC-U/C: 0x005eba38
     * @ghidraAddress PAL: 0x0062db80
     */
    virtual void DrawHorzLineU(int nY, int nLeft, int nRight);

    /**
     * Fill part of one row with the pen colour.
     *
     * Clamps the column range to the clip rectangle and returns for a row outside it.
     *
     * @param nY The row.
     * @param nLeft The first column.
     * @param nRight One past the last column.
     * @ghidraAddress NTSC-U/C: 0x005ebab8
     * @ghidraAddress PAL: 0x0062dc00
     */
    virtual void DrawHorzLine(int nY, int nLeft, int nRight);

    /**
     * Fill part of one column with the pen colour, with no clip test.
     *
     * @param nX The column.
     * @param nTop The first row.
     * @param nBottom One past the last row.
     * @ghidraAddress NTSC-U/C: 0x005ebb30
     * @ghidraAddress PAL: 0x0062dc78
     */
    virtual void DrawVertLineU(int nX, int nTop, int nBottom);

    /**
     * Fill part of one column with the pen colour, clamped to the clip rectangle.
     *
     * @param nX The column.
     * @param nTop The first row.
     * @param nBottom One past the last row.
     * @ghidraAddress NTSC-U/C: 0x005ebbb0
     * @ghidraAddress PAL: 0x0062dcf8
     */
    virtual void DrawVertLine(int nX, int nTop, int nBottom);

    /**
     * Fill a rectangle with the pen colour, with no clip test.
     *
     * @param rect The rectangle.
     * @ghidraAddress NTSC-U/C: 0x005ebc28
     * @ghidraAddress PAL: 0x0062dd70
     */
    virtual void DrawRectU(ARect rect);

    /**
     * Fill a rectangle with the pen colour.
     *
     * Intersects the rectangle with the clip rectangle in place and returns for an empty result.
     *
     * @param rect The rectangle.
     * @ghidraAddress NTSC-U/C: 0x005ebc98
     * @ghidraAddress PAL: 0x0062dde0
     */
    virtual void DrawRect(ARect rect);

    /**
     * Draw the four edges of a rectangle in the pen colour, with no clip test.
     *
     * @param rect The rectangle.
     * @ghidraAddress NTSC-U/C: 0x005e9378
     * @ghidraAddress PAL: 0x0062b4c0
     */
    virtual void DrawBoxU(ARect rect);

    /**
     * Draw the four edges of a rectangle in the pen colour, through the clipped fills.
     *
     * @param rect The rectangle.
     * @ghidraAddress NTSC-U/C: 0x005e9440
     * @ghidraAddress PAL: 0x0062b588
     */
    virtual void DrawBox(ARect rect);

    /**
     * Rewrite every palette index inside a rectangle through a remap table.
     *
     * @param rect The rectangle.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress NTSC-U/C: 0x005ebd40
     * @ghidraAddress PAL: 0x0062de88
     */
    virtual void DrawClutRectU(ARect rect, const unsigned char *pRemap);

    /**
     * Draw a line in the pen colour, with no clip test.
     *
     * Every coordinate is 24.8 fixed point. SetupLine() supplies a step of one whole unit
     * along the major axis and the pixel count.
     *
     * @param nX0 The first column.
     * @param nY0 The first row.
     * @param nX1 The last column.
     * @param nY1 The last row.
     * @ghidraAddress NTSC-U/C: 0x005ebec8
     * @ghidraAddress PAL: 0x0062e010
     */
    virtual void DrawLineU(int nX0, int nY0, int nX1, int nY1);

    /**
     * Draw a line in the pen colour, clipped.
     *
     * Clips both endpoints first and returns when nothing survives. Every coordinate is 24.8
     * fixed point.
     *
     * @param nX0 The first column.
     * @param nY0 The first row.
     * @param nX1 The last column.
     * @param nY1 The last row.
     * @ghidraAddress NTSC-U/C: 0x005ebf70
     * @ghidraAddress PAL: 0x0062e0b8
     */
    virtual void DrawLine(int nX0, int nY0, int nX1, int nY1);

    /**
     * Fill part of one row by sampling an indexed source bitmap along a fixed step.
     *
     * The position advances by the step once per destination column, and the whole parts of the
     * two components address one source byte. The step advances both components, so the sampled
     * run is an arbitrary straight line through the source rather than one source row.
     *
     * @param nY The destination row.
     * @param nLeft The first destination column.
     * @param nRight One past the last destination column.
     * @param pSource The source bitmap.
     * @param pSourcePosition The source position in 24.8 fixed point, advanced in place.
     * @param pSourceStep The per column advance in 24.8 fixed point.
     * @ghidraAddress NTSC-U/C: 0x005ec050
     * @ghidraAddress PAL: 0x0062e198
     */
    virtual void DrawTmapRow8U(int nY,
                               int nLeft,
                               int nRight,
                               const ABitmap *pSource,
                               APoint *pSourcePosition,
                               const APoint *pSourceStep);

    /**
     * Copy a four bit source bitmap, with no clip test.
     *
     * Walks the low nibble of each byte first, and the kABitmapOddNibbleStart bit of the source
     * starts a row in the high nibble instead.
     *
     * The transparency test compares a whole source byte against mTransparentColor rather than
     * the nibble the loop just consumed, and it reads the byte after the loop has already advanced
     * past it in the high nibble case. Both are faithful to the binary.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress NTSC-U/C: 0x005ec280
     * @ghidraAddress PAL: 0x0062e3c8
     */
    virtual void DrawBitmapLin4U(const ABitmap &source, int nX, int nY);

    /**
     * Copy a four bit source bitmap, clipped.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress NTSC-U/C: 0x005ec3c0
     * @ghidraAddress PAL: 0x0062e508
     */
    virtual void DrawBitmapLin4(const ABitmap &source, int nX, int nY);

    /**
     * Copy an eight bit source bitmap, with no clip test.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress NTSC-U/C: 0x005ec450
     * @ghidraAddress PAL: 0x0062e598
     */
    virtual void DrawBitmapLin8U(const ABitmap &source, int nX, int nY);

    /**
     * Copy an eight bit source bitmap, clipped.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress NTSC-U/C: 0x005ec570
     * @ghidraAddress PAL: 0x0062e6b8
     */
    virtual void DrawBitmapLin8(const ABitmap &source, int nX, int nY);

    /**
     * Copy a 1555 source bitmap, with no clip test.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress NTSC-U/C: 0x005ec600
     * @ghidraAddress PAL: 0x0062e748
     */
    virtual void DrawBitmapLin15U(const ABitmap &source, int nX, int nY);

    /**
     * Copy a 1555 source bitmap, clipped.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress NTSC-U/C: 0x005ec738
     * @ghidraAddress PAL: 0x0062e880
     */
    virtual void DrawBitmapLin15(const ABitmap &source, int nX, int nY);

    /**
     * Copy a 24 bit source bitmap, with no clip test.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress NTSC-U/C: 0x005ec7c8
     * @ghidraAddress PAL: 0x0062e910
     */
    virtual void DrawBitmapLin24U(const ABitmap &source, int nX, int nY);

    /**
     * Copy a 24 bit source bitmap, clipped.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress NTSC-U/C: 0x005ec928
     * @ghidraAddress PAL: 0x0062ea70
     */
    virtual void DrawBitmapLin24(const ABitmap &source, int nX, int nY);

    /**
     * Copy a 32 bit source bitmap, with no clip test.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress NTSC-U/C: 0x005ec9b8
     * @ghidraAddress PAL: 0x0062eb00
     */
    virtual void DrawBitmapLin32U(const ABitmap &source, int nX, int nY);

    /**
     * Copy a 32 bit source bitmap, clipped.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress NTSC-U/C: 0x005ecad8
     * @ghidraAddress PAL: 0x0062ec20
     */
    virtual void DrawBitmapLin32(const ABitmap &source, int nX, int nY);

    /**
     * Copy a run length encoded eight bit source bitmap, with no clip test.
     *
     * Decodes one row into g_abCanvasRowScratch, describes the decoded row as an
     * eight bit ABitmap of one row, and copies it with DrawBitmapLin8U().
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress NTSC-U/C: 0x005ecb68
     * @ghidraAddress PAL: 0x0062ecb0
     */
    virtual void DrawBitmapRle8U(const ABitmap &source, int nX, int nY);

    /**
     * Copy a run length encoded eight bit source bitmap, clipped.
     *
     * Forwards to DrawBitmapRle8U() when the whole source fits inside the clip rectangle, and
     * otherwise decodes and copies the surviving rows one at a time.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress NTSC-U/C: 0x005e9cb8
     * @ghidraAddress PAL: 0x0062be00
     */
    virtual void DrawBitmapRle8(const ABitmap &source, int nX, int nY);

    /**
     * Read a rectangle of the canvas into a four bit bitmap, clipped.
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress NTSC-U/C: 0x005ecef0
     * @ghidraAddress PAL: 0x0062f038
     */
    virtual void GetBitmapLin4(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into a four bit bitmap, with no clip test.
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress NTSC-U/C: 0x005ecdb8
     * @ghidraAddress PAL: 0x0062ef00
     */
    virtual void GetBitmapLin4U(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into an eight bit bitmap, clipped.
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress NTSC-U/C: 0x005ed070
     * @ghidraAddress PAL: 0x0062f1b8
     */
    virtual void GetBitmapLin8(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into an eight bit bitmap, with no clip test.
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress NTSC-U/C: 0x005ecf80
     * @ghidraAddress PAL: 0x0062f0c8
     */
    virtual void GetBitmapLin8U(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into a 1555 bitmap, clipped.
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress NTSC-U/C: 0x005ed208
     * @ghidraAddress PAL: 0x0062f350
     */
    virtual void GetBitmapLin15(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into a 1555 bitmap, with no clip test.
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress NTSC-U/C: 0x005ed100
     * @ghidraAddress PAL: 0x0062f248
     */
    virtual void GetBitmapLin15U(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into a 24 bit bitmap, clipped.
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress NTSC-U/C: 0x005ed398
     * @ghidraAddress PAL: 0x0062f4e0
     */
    virtual void GetBitmapLin24(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into a 24 bit bitmap, with no clip test.
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress NTSC-U/C: 0x005ed298
     * @ghidraAddress PAL: 0x0062f3e0
     */
    virtual void GetBitmapLin24U(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into a 32 bit bitmap, clipped.
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress NTSC-U/C: 0x005ed520
     * @ghidraAddress PAL: 0x0062f668
     */
    virtual void GetBitmapLin32(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into a 32 bit bitmap, with no clip test.
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress NTSC-U/C: 0x005ed428
     * @ghidraAddress PAL: 0x0062f570
     */
    virtual void GetBitmapLin32U(const ABitmap &dest, int nX, int nY);

    /**
     * Draw one glyph of a font, with no clip test.
     *
     * Selects the unclipped copy slot from the glyph format code, through the six entry table of
     * pointers to member functions at 0x0077dc98. A code below mFirstCharCode indexes before the
     * glyph array, and a code at or beyond the glyph count yields a null glyph the routine then
     * reads through. Both are faithful to the binary.
     *
     * @param nCharCode The character code.
     * @param pFont The font.
     * @param nX The destination column.
     * @param nY The baseline row.
     * @ghidraAddress NTSC-U/C: 0x005e9ee8
     * @ghidraAddress PAL: 0x0062c030
     */
    virtual void DrawCharU(int nCharCode, const AFont *pFont, int nX, int nY);

    /**
     * Draw one glyph of a font, clipped.
     *
     * Selects the clipped copy slot through the table at 0x0077dcc8.
     *
     * @param nCharCode The character code.
     * @param pFont The font.
     * @param nX The destination column.
     * @param nY The baseline row.
     * @ghidraAddress NTSC-U/C: 0x005e9fc8
     * @ghidraAddress PAL: 0x0062c110
     */
    virtual void DrawChar(int nCharCode, const AFont *pFont, int nX, int nY);

    /**
     * Draw a null terminated string, with no clip test.
     *
     * A newline returns to the starting column and advances the row by the font line height.
     * Every other code draws one glyph and advances the column by the glyph width.
     *
     * @param pText The string.
     * @param pFont The font.
     * @param nX The starting column.
     * @param nY The baseline row.
     * @ghidraAddress NTSC-U/C: 0x005ed5b0
     * @ghidraAddress PAL: 0x0062f6f8
     */
    virtual void DrawTextU(const char *pText, const AFont *pFont, int nX, int nY);

    /**
     * Draw a null terminated string, clipped.
     *
     * @param pText The string.
     * @param pFont The font.
     * @param nX The starting column.
     * @param nY The baseline row.
     * @ghidraAddress NTSC-U/C: 0x005ea120
     * @ghidraAddress PAL: 0x0062c268
     */
    virtual void DrawText(const char *pText, const AFont *pFont, int nX, int nY);

    /**
     * Copy a four bit source bitmap through a remap table.
     *
     * Unpacks one row into g_abCanvasRowScratch and stores the remapped row.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress NTSC-U/C: 0x005ed858
     * @ghidraAddress PAL: 0x0062f9a0
     */
    virtual void
    DrawClutBitmapLin4U(const ABitmap &source, int nX, int nY, const unsigned char *pRemap);

    /**
     * Copy an eight bit source bitmap through a remap table, one row at a time.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress NTSC-U/C: 0x005eda30
     * @ghidraAddress PAL: 0x0062fb78
     */
    virtual void
    DrawClutBitmapLin8U(const ABitmap &source, int nX, int nY, const unsigned char *pRemap);

    /**
     * Store one row of palette indices through a remap table.
     *
     * Skips a source value equal to the span transparent colour.
     *
     * @param span The row to store.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress NTSC-U/C: 0x005edd08
     * @ghidraAddress PAL: 0x0062fe50
     */
    virtual void DrawClutBitmapRowLin8U(const ARowInfo &span, const unsigned char *pRemap);

    /**
     * Copy a four bit source bitmap through a blend table.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress NTSC-U/C: 0x005edfb8
     * @ghidraAddress PAL: 0x00630100
     */
    virtual void DrawBlendBitmapLin4U(const ABitmap &source,
                                      int nX,
                                      int nY,
                                      const unsigned char *const *ppBlend);

    /**
     * Copy an eight bit source bitmap through a blend table.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress NTSC-U/C: 0x005ee1c8
     * @ghidraAddress PAL: 0x00630310
     */
    virtual void DrawBlendBitmapLin8U(const ABitmap &source,
                                      int nX,
                                      int nY,
                                      const unsigned char *const *ppBlend);

    /**
     * Store one row of palette indices blended against the destination.
     *
     * Reads the destination index, then selects the replacement from the blend row of the source
     * index. Index zero is transparent when the span requests transparency, which differs from
     * DrawClutBitmapRowLin8U(), where the span transparent colour applies instead.
     *
     * @param span The row to store.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress NTSC-U/C: 0x005ee4a0
     * @ghidraAddress PAL: 0x006305e8
     */
    virtual void DrawBlendBitmapRowLin8U(const ARowInfo &span, const unsigned char *const *ppBlend);

    /**
     * Store one row of palette indices sampled along a fixed step.
     *
     * @param span The row to store.
     * @ghidraAddress NTSC-U/C: 0x005ee968
     * @ghidraAddress PAL: 0x00630ab0
     */
    virtual void DrawScaledBitmapRowLin8U(const AScaledRowInfo &span);

    /**
     * Store one row of 1555 colours sampled along a fixed step.
     *
     * @param span The row to store.
     * @ghidraAddress NTSC-U/C: 0x005eea18
     * @ghidraAddress PAL: 0x00630b60
     */
    virtual void DrawScaledBitmapRowLin15U(const AScaledRowInfo &span);

    /**
     * Store one row of red, green, blue triples sampled along a fixed step.
     *
     * @param span The row to store.
     * @ghidraAddress NTSC-U/C: 0x005eeac8
     * @ghidraAddress PAL: 0x00630c10
     */
    virtual void DrawScaledBitmapRowLin24U(const AScaledRowInfo &span);

    /**
     * Store one row of 8888 colours sampled along a fixed step.
     *
     * @param span The row to store.
     * @ghidraAddress NTSC-U/C: 0x005eebb8
     * @ghidraAddress PAL: 0x00630d00
     */
    virtual void DrawScaledBitmapRowLin32U(const AScaledRowInfo &span);

    /**
     * Store one row of palette indices sampled along a fixed step, through a remap table.
     *
     * @param span The row to store.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress NTSC-U/C: 0x005eedb8
     * @ghidraAddress PAL: 0x00630f00
     */
    virtual void DrawScaledClutBitmapRowLin8U(const AScaledRowInfo &span,
                                              const unsigned char *pRemap);

    /**
     * Store one row of palette indices sampled along a fixed step, blended against the
     * destination.
     *
     * @param span The row to store.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress NTSC-U/C: 0x005eefc0
     * @ghidraAddress PAL: 0x00631108
     */
    virtual void DrawScaledBlendBitmapRowLin8U(const AScaledRowInfo &span,
                                               const unsigned char *const *ppBlend);

    ABitmap mBitmap; /*!< The pixel rectangle this canvas draws into. +0x00 */
    ARect mClip;     /*!< The clip rectangle every clipped operation tests against. +0x18 */

protected:
    // Every member below is reached only from this class and its subclasses, so protected is the
    // narrowest specifier the image supports. No access from outside the hierarchy was located.

    /**
     * Classify a point against the clip rectangle.
     *
     * The code is formed in eight bits, and ClipLine() compares combinations of two codes
     * in eight bits as well.
     *
     * @param nX The horizontal coordinate.
     * @param nY The vertical coordinate.
     * @return The ACanvasClipCode bits, or zero when the point is inside.
     * @ghidraAddress NTSC-U/C: 0x005eb3d0
     * @ghidraAddress PAL: 0x0062d518
     */
    unsigned char ClipCode(int nX, int nY) const;

    /**
     * Clip a run length encoded copy against the clip rectangle.
     *
     * Records the decoded columns that remain and the destination row the copy stops at, and
     * moves the destination position onto the clip rectangle. Rows above the clip rectangle are
     * consumed through ARle8Reader::SkipRow(), so the reader then addresses the first row that
     * remains. The row fields are not written when no column remains.
     *
     * The image has no caller. DrawBitmapRle8(), DrawClutBitmapRle8(), and DrawBlendBitmapRle8()
     * each compile an inlined copy.
     *
     * @param source The source bitmap.
     * @param pnX The destination column, moved right to the clip rectangle when it lies left of
     *            it.
     * @param pnY The destination row, moved down likewise.
     * @param pReader The reader over the source, already at the first row.
     * @param pSpan The record to fill.
     * @return Zero when no column or no row remains.
     * @ghidraAddress NTSC-U/C: 0x005eb418
     * @ghidraAddress PAL: 0x0062d560
     */
    int ClipRle8Bitmap(
        const ABitmap &source, int *pnX, int *pnY, ARle8Reader *pReader, Rle8Clip *pSpan) const;

    /**
     * Clip a line against the clip rectangle, rewriting both endpoints in place.
     *
     * Every coordinate is 24.8 fixed point. The routine is a Cohen and Sutherland clip that only
     * ever moves the second endpoint. When the second endpoint is inside and the first is not, the
     * two are exchanged first, so the endpoints can return in the opposite order. An endpoint
     * clipped to the right or bottom edge lands g_nFixedEpsilon inside the exclusive edge.
     *
     * The outcode of the first endpoint is computed once, before the loop, and after an exchange
     * it is taken as zero rather than recomputed.
     *
     * @param pnX0 The first column.
     * @param pnY0 The first row.
     * @param pnX1 The last column.
     * @param pnY1 The last row.
     * @return Zero when nothing remains, and one otherwise.
     * @ghidraAddress NTSC-U/C: 0x005e8fd8
     * @ghidraAddress PAL: 0x0062b120
     */
    int ClipLine(int *pnX0, int *pnY0, int *pnX1, int *pnY1) const;

    /**
     * Clip a source bitmap and a destination position against the clip rectangle.
     *
     * Advances the pixel pointer, shrinks the extent, and moves the destination position, all in
     * place. Every clipped copy and read slot calls this first.
     *
     * The left clip of a kABitmapFormatLinear4 bitmap has two defects in the binary. The pixel
     * pointer advances by half the destination column rather than by half the columns clipped
     * away, and for an odd count the new mOddNibbleStart is the inverse of this canvas's own
     * flag rather than of the bitmap's.
     *
     * @param pBitmap The bitmap to clip.
     * @param pnX The destination column.
     * @param pnY The destination row.
     * @return Zero when nothing remains.
     * @ghidraAddress NTSC-U/C: 0x005e91b8
     * @ghidraAddress PAL: 0x0062b300
     */
    int ClipBitmap(ABitmap *pBitmap, int *pnX, int *pnY) const;

    /**
     * Intersect a rectangle with the clip rectangle in place.
     *
     * Non-virtual, and the image has no caller.
     *
     * @param pRect The rectangle to clip.
     * @return Zero when the intersection is empty.
     * @ghidraAddress NTSC-U/C: 0x005eaeb8
     * @ghidraAddress PAL: 0x0062d000
     */
    int ClipRect(ARect *pRect) const;

    /**
     * Rewrite the palette indices inside a rectangle through a remap table, clipped.
     *
     * Intersects the rectangle with the clip rectangle and calls DrawClutRectU() when the
     * result is not empty. Non-virtual and orphaned.
     *
     * @param rect The rectangle.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress NTSC-U/C: 0x005ebe10
     * @ghidraAddress PAL: 0x0062df58
     */
    void DrawClutRect(ARect rect, const unsigned char *pRemap);

    /**
     * Copy a source bitmap, with no clip test, choosing the slot by source format.
     *
     * Calls through the table at 0x0077dc98, the one DrawCharU() uses. Non-virtual and
     * orphaned.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress NTSC-U/C: 0x005ec130
     * @ghidraAddress PAL: 0x0062e278
     */
    void DrawBitmapU(const ABitmap &source, int nX, int nY);

    /**
     * Clip against the clip rectangle and then copy a four bit source through a remap table.
     *
     * The clip runs against a stack copy of the source description. Non-virtual and orphaned.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress NTSC-U/C: 0x005ed990
     * @ghidraAddress PAL: 0x0062fad8
     */
    void DrawClutBitmapLin4(const ABitmap &source, int nX, int nY, const unsigned char *pRemap);

    /**
     * Clip against the clip rectangle and then copy an eight bit source through a remap table.
     *
     * Non-virtual and orphaned.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress NTSC-U/C: 0x005edb38
     * @ghidraAddress PAL: 0x0062fc80
     */
    void DrawClutBitmapLin8(const ABitmap &source, int nX, int nY, const unsigned char *pRemap);

    /**
     * Clip against the clip rectangle and then copy a four bit source through a blend table.
     *
     * Non-virtual and orphaned.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress NTSC-U/C: 0x005ee128
     * @ghidraAddress PAL: 0x00630270
     */
    void
    DrawBlendBitmapLin4(const ABitmap &source, int nX, int nY, const unsigned char *const *ppBlend);

    /**
     * Clip against the clip rectangle and then copy an eight bit source through a blend table.
     *
     * Non-virtual and orphaned.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress NTSC-U/C: 0x005ee2d0
     * @ghidraAddress PAL: 0x00630418
     */
    void
    DrawBlendBitmapLin8(const ABitmap &source, int nX, int nY, const unsigned char *const *ppBlend);

    /**
     * Fill a convex polygon with its colour.
     *
     * Sets the pen colour through SetColorNative() from APolygon::mColor, then walks two edges
     * down from the top vertex, one forward and one backward through the vertex list, filling
     * each row between them with DrawHorzLine(). Each edge column is rounded to the nearest whole
     * column with g_nFixedHalf. Rows above the clip rectangle advance the edges without drawing,
     * and the walk stops at the bottom of the clip rectangle or when the two edges meet.
     *
     * Non-virtual and orphaned.
     *
     * @param polygon The polygon.
     * @ghidraAddress NTSC-U/C: 0x005e95e8
     * @ghidraAddress PAL: 0x0062b730
     */
    void DrawFlatConvexPolygon(const APolygon &polygon);

    /**
     * Fill a convex polygon by sampling its texture.
     *
     * Walks the edges as DrawFlatConvexPolygon() does. Each row interpolates the texture position
     * between the two edges, clamps its columns to the clip rectangle, advancing the start position
     * by the columns clamped away on the left, and draws with DrawTmapRow8U(). A row of no columns
     * draws nothing. FindTopmostPolyVertex() is compiled inline here.
     *
     * Non-virtual and orphaned.
     *
     * @param polygon The polygon, whose first word is its texture.
     * @ghidraAddress NTSC-U/C: 0x005e98c8
     * @ghidraAddress PAL: 0x0062ba10
     */
    void DrawTmappedConvexPolygon(const APolygon &polygon);

    /**
     * Copy a source bitmap through a remap table, choosing the arm by source format.
     *
     * Non-virtual, and orphaned in the shipped image: it fills no slot of this class's table and
     * the image has no caller and no data reference. Format code 0 goes to DrawClutBitmapLin4U(),
     * code 1 to DrawClutBitmapLin8U(), and code 5 to DrawClutBitmapRle8U(). Every other code
     * returns without drawing.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress NTSC-U/C: 0x005ed6a8
     * @ghidraAddress PAL: 0x0062f7f0
     */
    void DrawClutBitmapU(const ABitmap &source, int nX, int nY, const unsigned char *pRemap);

    /**
     * Clip against the clip rectangle and then copy through a remap table.
     *
     * The clip runs against a stack copy of the source description, because ClipBitmap()
     * rewrites what it is given. A zero result returns without drawing.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress NTSC-U/C: 0x005ed718
     * @ghidraAddress PAL: 0x0062f860
     */
    void DrawClutBitmap(const ABitmap &source, int nX, int nY, const unsigned char *pRemap);

    /**
     * Copy a run length encoded source through a remap table.
     *
     * The run length encoded arm of DrawClutBitmapU(), reached only for format code 5. It decodes
     * each row into g_abCanvasRowScratch through ARle8Reader and stores it with
     * DrawClutBitmapRowLin8U().
     *
     * The row loop advances nY rather than the span row, so its bound recedes with the row and a
     * source of one row or more never finishes. DrawClutBitmapU() has no caller in the image, so
     * the defect is unreachable.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress NTSC-U/C: 0x005edbd8
     * @ghidraAddress PAL: 0x0062fd20
     */
    void DrawClutBitmapRle8U(const ABitmap &source, int nX, int nY, const unsigned char *pRemap);

    /**
     * Clip and copy a run length encoded source through a remap table.
     *
     * The clipped counterpart of DrawClutBitmapRle8U(), reached only from DrawClutBitmap(). Rows
     * above the clip rectangle are consumed through ARle8Reader::SkipRow() rather than decoded.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress NTSC-U/C: 0x005ea280
     * @ghidraAddress PAL: 0x0062c3c8
     */
    void DrawClutBitmapRle8(const ABitmap &source, int nX, int nY, const unsigned char *pRemap);

    /**
     * Copy a source bitmap through a table of blend tables, choosing the arm by source format.
     *
     * Non-virtual and orphaned, with the same shape as DrawClutBitmapU(). Format code 0 goes to
     * DrawBlendBitmapLin4U(), code 1 to DrawBlendBitmapLin8U(), and code 5 to
     * DrawBlendBitmapRle8U().
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress NTSC-U/C: 0x005ede08
     * @ghidraAddress PAL: 0x0062ff50
     */
    void
    DrawBlendBitmapU(const ABitmap &source, int nX, int nY, const unsigned char *const *ppBlend);

    /**
     * Clip against the clip rectangle and then copy through a table of blend tables.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress NTSC-U/C: 0x005ede78
     * @ghidraAddress PAL: 0x0062ffc0
     */
    void
    DrawBlendBitmap(const ABitmap &source, int nX, int nY, const unsigned char *const *ppBlend);

    /**
     * Copy a run length encoded source through a table of blend tables.
     *
     * The same code as DrawClutBitmapRle8U() with DrawBlendBitmapRowLin8U() in place of
     * DrawClutBitmapRowLin8U(), including the row loop that never finishes. DrawBlendBitmapU() has
     * no caller in the image.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress NTSC-U/C: 0x005ee370
     * @ghidraAddress PAL: 0x006304b8
     */
    void DrawBlendBitmapRle8U(const ABitmap &source,
                              int nX,
                              int nY,
                              const unsigned char *const *ppBlend);

    /**
     * Clip and copy a run length encoded source through a table of blend tables.
     *
     * The same code as DrawClutBitmapRle8() with DrawBlendBitmapRowLin8U() in place of
     * DrawClutBitmapRowLin8U().
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress NTSC-U/C: 0x005ea460
     * @ghidraAddress PAL: 0x0062c5a8
     */
    void
    DrawBlendBitmapRle8(const ABitmap &source, int nX, int nY, const unsigned char *const *ppBlend);

    /**
     * Read a rectangle of the canvas into a bitmap, choosing the slot by destination format.
     *
     * Non-virtual and orphaned, like DrawClutBitmapU(). Calls through the six entry table of
     * pointers to member functions at 0x0077dcf8, whose first five entries address the unclipped
     * read slots GetBitmapLin4U() through GetBitmapLin32U() and whose sixth is ReadRectRle8().
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress NTSC-U/C: 0x005ecc68
     * @ghidraAddress PAL: 0x0062edb0
     */
    void GetBitmapU(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into a bitmap, clipped, choosing the slot by destination
     * format.
     *
     * Non-virtual and orphaned. Calls through the table at 0x0077dd28, which addresses the clipped
     * read slots GetBitmapLin4() through GetBitmapLin32() and then ReadRectRle8().
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress NTSC-U/C: 0x005ecd10
     * @ghidraAddress PAL: 0x0062ee58
     */
    void GetBitmap(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into a run length encoded bitmap.
     *
     * Empty. The run length encoded entry of both read tables addresses this one body, so a read
     * into kABitmapFormatRle8 does nothing.
     *
     * @param dest The destination bitmap.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress NTSC-U/C: 0x005eb190
     * @ghidraAddress PAL: 0x0062d2d8
     */
    void ReadRectRle8(const ABitmap &dest, int nX, int nY);

    /**
     * Prepare a stretched copy of a source bitmap into a destination rectangle.
     *
     * Both steps are the source extent in 24.8 fixed point divided by the destination extent, and
     * each position starts at half its step, so the walk samples the centre of each source cell.
     * The destination rectangle is clamped to the clip rectangle, and the positions advance by one
     * step per row or column clamped away. AScaledRowInfo::mSource addresses the first source row
     * the walk samples.
     *
     * The palette resolves from the source, then the canvas, then g_pDefaultPalette.
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle, before clipping.
     * @param pBlit The record to fill.
     * @return Zero when the rectangle is empty before or after clipping.
     * @ghidraAddress NTSC-U/C: 0x005ea780
     * @ghidraAddress PAL: 0x0062c8c8
     */
    int
    ClipAndSetupScaledBitmap(const ABitmap &source, const ARect &rect, AScaledRowInfo *pBlit) const;

    /**
     * Stretch a source bitmap into a destination rectangle, choosing the arm by source format.
     *
     * Non-virtual and orphaned. Calls through the six entry table of pointers to member functions
     * at 0x0077dd58, whose entries are DrawScaledBitmapLin4() through DrawScaledBitmapLin32() and
     * then DrawScaledBitmapRle8(). Every arm clips against the clip rectangle.
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @ghidraAddress NTSC-U/C: 0x005ee580
     * @ghidraAddress PAL: 0x006306c8
     */
    void DrawScaledBitmap(const ABitmap &source, const ARect &rect);

    /**
     * Stretch a four bit source bitmap into a destination rectangle.
     *
     * Unpacks each sampled row into g_abCanvasRowScratch and stores it with
     * DrawScaledBitmapRowLin8U(). The row walk starts at the first source row rather than at the
     * row ClipAndSetupScaledBitmap() selected. A rectangle clipped at the top samples rows from too
     * high in the source.
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @ghidraAddress NTSC-U/C: 0x005ea640
     * @ghidraAddress PAL: 0x0062c788
     */
    void DrawScaledBitmapLin4(const ABitmap &source, const ARect &rect);

    /**
     * Stretch an eight bit source bitmap into a destination rectangle, through
     * DrawScaledBitmapRowLin8U().
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @ghidraAddress NTSC-U/C: 0x005ee628
     * @ghidraAddress PAL: 0x00630770
     */
    void DrawScaledBitmapLin8(const ABitmap &source, const ARect &rect);

    /**
     * Stretch a 1555 source bitmap into a destination rectangle, through
     * DrawScaledBitmapRowLin15U().
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @ghidraAddress NTSC-U/C: 0x005ee6f8
     * @ghidraAddress PAL: 0x00630840
     */
    void DrawScaledBitmapLin15(const ABitmap &source, const ARect &rect);

    /**
     * Stretch a 24 bit source bitmap into a destination rectangle, through
     * DrawScaledBitmapRowLin24U().
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @ghidraAddress NTSC-U/C: 0x005ee7c8
     * @ghidraAddress PAL: 0x00630910
     */
    void DrawScaledBitmapLin24(const ABitmap &source, const ARect &rect);

    /**
     * Stretch a 32 bit source bitmap into a destination rectangle, through
     * DrawScaledBitmapRowLin32U().
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @ghidraAddress NTSC-U/C: 0x005ee898
     * @ghidraAddress PAL: 0x006309e0
     */
    void DrawScaledBitmapLin32(const ABitmap &source, const ARect &rect);

    /**
     * Stretch a run length encoded source into a destination rectangle.
     *
     * Decodes the first sampled row into g_abCanvasRowScratch, then decodes again only when the
     * sampled row changes, consuming any rows between through ARle8Reader::SkipRow().
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @ghidraAddress NTSC-U/C: 0x005ea960
     * @ghidraAddress PAL: 0x0062caa8
     */
    void DrawScaledBitmapRle8(const ABitmap &source, const ARect &rect);

    /**
     * Stretch an indexed source through a remap table, choosing the arm by source format.
     *
     * Non-virtual and orphaned. Format code 0 goes to DrawScaledClutBitmapLin4(), code 1 to
     * DrawScaledClutBitmapLin8(), and code 5 to DrawScaledClutBitmapRle8(). Every other code draws
     * nothing.
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress NTSC-U/C: 0x005eec70
     * @ghidraAddress PAL: 0x00630db8
     */
    void
    DrawScaledClutBitmap(const ABitmap &source, const ARect &rect, const unsigned char *pRemap);

    /**
     * Stretch a four bit source through a remap table. Empty.
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress NTSC-U/C: 0x005eecd0
     * @ghidraAddress PAL: 0x00630e18
     */
    void
    DrawScaledClutBitmapLin4(const ABitmap &source, const ARect &rect, const unsigned char *pRemap);

    /**
     * Stretch an eight bit source through a remap table, through DrawScaledClutBitmapRowLin8U().
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress NTSC-U/C: 0x005eecd8
     * @ghidraAddress PAL: 0x00630e20
     */
    void
    DrawScaledClutBitmapLin8(const ABitmap &source, const ARect &rect, const unsigned char *pRemap);

    /**
     * Stretch a run length encoded source through a remap table.
     *
     * The same code as DrawScaledBitmapRle8() with DrawScaledClutBitmapRowLin8U() in place of
     * DrawScaledBitmapRowLin8U().
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress NTSC-U/C: 0x005eaa98
     * @ghidraAddress PAL: 0x0062cbe0
     */
    void
    DrawScaledClutBitmapRle8(const ABitmap &source, const ARect &rect, const unsigned char *pRemap);

    /**
     * Stretch an indexed source blended against the destination, choosing the arm by source
     * format.
     *
     * Non-virtual and orphaned. Format code 0 goes to DrawScaledBlendBitmapLin4(), code 1 to
     * DrawScaledBlendBitmapLin8(), and code 5 to DrawScaledBlendBitmapRle8(). The four bit arm
     * reads its source as eight bit and the eight bit arm is empty.
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress NTSC-U/C: 0x005eee78
     * @ghidraAddress PAL: 0x00630fc0
     */
    void DrawScaledBlendBitmap(const ABitmap &source,
                               const ARect &rect,
                               const unsigned char *const *ppBlend);

    /**
     * Stretch an eight bit source blended against the destination. Empty.
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress NTSC-U/C: 0x005eeed8
     * @ghidraAddress PAL: 0x00631020
     */
    void DrawScaledBlendBitmapLin8(const ABitmap &source,
                                   const ARect &rect,
                                   const unsigned char *const *ppBlend);

    /**
     * Stretch a four bit source blended against the destination.
     *
     * The source is read as eight bit, through DrawScaledBlendBitmapRowLin8U().
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress NTSC-U/C: 0x005eeee0
     * @ghidraAddress PAL: 0x00631028
     */
    void DrawScaledBlendBitmapLin4(const ABitmap &source,
                                   const ARect &rect,
                                   const unsigned char *const *ppBlend);

    /**
     * Stretch a run length encoded source blended against the destination.
     *
     * The same code as DrawScaledBitmapRle8() with DrawScaledBlendBitmapRowLin8U() in place of
     * DrawScaledBitmapRowLin8U().
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress NTSC-U/C: 0x005eabe0
     * @ghidraAddress PAL: 0x0062cd28
     */
    void DrawScaledBlendBitmapRle8(const ABitmap &source,
                                   const ARect &rect,
                                   const unsigned char *const *ppBlend);

    /**
     * Derive the per pixel step of a line and return the pixel count.
     *
     * Every coordinate and both steps are 24.8 fixed point. The major axis advances one whole
     * unit per pixel and the minor axis advances the quotient of the two extents.
     *
     * @param nX0 The first column.
     * @param nY0 The first row.
     * @param nX1 The last column.
     * @param nY1 The last row.
     * @param pnStepX The column step to write.
     * @param pnStepY The row step to write.
     * @return The pixel count, or zero for a line of no extent.
     * @ghidraAddress NTSC-U/C: 0x005e9508
     * @ghidraAddress PAL: 0x0062b650
     */
    static int SetupLine(int nX0, int nY0, int nX1, int nY1, int *pnStepX, int *pnStepY);

    /**
     * Return the position in APolygon::mIndices of the vertex with the smallest row.
     *
     * A tie resolves to the earlier position.
     *
     * @param polygon The polygon.
     * @return The index into APolygon::mIndices.
     * @ghidraAddress NTSC-U/C: 0x005ebfe0
     * @ghidraAddress PAL: 0x0062e128
     */
    static int FindTopmostPolyVertex(const APolygon &polygon);

    /**
     * Unpack one row of four bit pixels into one byte each.
     *
     * The low nibble of a byte supplies the earlier pixel.
     *
     * @param pSource The packed row.
     * @param pDest The unpacked row.
     * @param nCount The number of pixels.
     * @param bStartHighNibble Whether the first pixel is the high nibble of the first byte.
     * @ghidraAddress NTSC-U/C: 0x005eddb8
     * @ghidraAddress PAL: 0x0062ff00
     */
    static void
    Unpack4(const unsigned char *pSource, unsigned char *pDest, int nCount, int bStartHighNibble);
};

/**
 * Palette every canvas falls back to when neither its bitmap nor its source supplies one.
 *
 * Read by ACanvas8::SetColor8() and every other slot that resolves a palette index, always
 * after the bitmap palette is found null.
 *
 * No writer was located. The fallback may be permanently null.
 *
 * @ghidraAddress NTSC-U/C: 0x0086f6f0
 * @ghidraAddress PAL: 0x008b3dd0
 */
extern APalette *g_pDefaultPalette;

/**
 * One row of indices, shared by every block copy that has to expand a row before writing it.
 *
 * ACanvasLin4 unpacks a four-bit row into it, both four-bit layout classes decode a run length
 * encoded row into it, and several ACanvas block copies use it the same way. Each writes the row
 * and consumes it before returning, so the buffer carries nothing between calls.
 *
 * The bound is not recovered and no definition is written for it. The buffer is a plain static with
 * no allocation to read a size from, and the nearest referenced address sits more than 0x400 bytes
 * above it, which limits the space it could occupy without establishing what it does occupy. Which
 * translation unit defines it is also unsettled. It is therefore declared without a bound rather
 * than defined with an invented one.
 *
 * @ghidraAddress NTSC-U/C: 0x008f09f0
 * @ghidraAddress PAL: 0x00935a00
 */
extern unsigned char g_abCanvasRowScratch[];

/**
 * Pack an 8888 colour into 1555.
 *
 * The alpha bit comes from bit 31 and the low three bits of each channel are dropped. The helper
 * is out of line because eleven call sites across ACanvas8, ACanvas15, and ACanvasLin15 share it,
 * while the same packing inlined into ACanvas15::SetColor32() is written out there instruction for
 * instruction.
 *
 * @param nColor The 8888 colour.
 * @return The 1555 colour.
 * @ghidraAddress NTSC-U/C: 0x00618f38
 * @ghidraAddress PAL: 0x00659ac8
 */
unsigned short APackRgb1555From8888(unsigned int nColor);
