#pragma once

#include "rndartt/abitmap.h"
#include "rndartt/arect.h"
#include "rndartt/arlereader.h"

class APalette;
struct AFont;
struct APoint;
struct ARowSpan;
struct AStretchBlit;
struct AStretchSpan;

/** Fractional bits in the coordinates DrawLine() and TextureRowIndexed() take. */
constexpr int kACanvasFractionBits = 8;

/**
 * Bits ACanvas::ClipCodeForPoint() returns.
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
 * The name comes from the RTTI descriptor at 0x0086f660, whose mangled form is `7ACanvas`. The
 * descriptor records no base class. The virtual function table pointer sits after the data
 * members at offset 0x20, the position the toolchain uses for a class with no base.
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
 * takes the NoClip suffix. For the pixel accessors and the row, column, and rectangle fills the
 * unclipped form is pure virtual here and the clipped form is implemented here in terms of it. For
 * the copy and read slots both forms are implemented here, the clipped one calling ClipBlitToRect()
 * and then the unclipped one.
 *
 * Six colour formats arrive at a pixel. The pen colour form uses no argument. The indexed form
 * takes a
 * palette index, the 1555 form a halfword, the RGB form three bytes, the 8888 form a word, and the
 * native form a value in the canvas storage width. On a 32 bit canvas the 8888 and native forms
 * coincide, and ACanvas32 implements the native pair by forwarding to the 8888 pair.
 *
 * The analysis program titles the 8888 accessors PutPixel32 and GetPixel32, because a Ghidra
 * symbol cannot be overloaded. The names here are the overload set the toolchain compiled.
 *
 * Both data members are public. Rnd::Font::ComputeCharUV() reads mBitmap.mWidth and
 * mBitmap.mHeight from outside the hierarchy and the image supplies no accessor, so the access
 * rule gives public. A friend declaration for Rnd::Font fits the image equally well. mClip has no
 * reader outside the hierarchy and would otherwise be protected, and it shares the public section
 * so that the recovered order of the two survives.
 *
 * One further member is recovered but not written here.
 *
 * `ClipBlitSpan` at 0x005eb418 clips one source row against the clip rectangle and writes the
 * start and end columns plus the destination row into three small output records. Its six
 * arguments are recovered. The records they address are not, and the signature is therefore
 * unsettled.
 */
class ACanvas {
public:
    /**
     * Adopt a pixel description.
     *
     * The description is copied whole and the clip rectangle is set to the full bitmap, from the
     * origin to mWidth by mHeight. Every subclass constructor calls this first and then installs
     * its own virtual function table.
     *
     * @param bitmap The description to adopt.
     * @ghidraAddress 0x005eb1a0
     */
    explicit ACanvas(const ABitmap &bitmap);

    /**
     * Construct the linear canvas subclass that draws a bitmap's format.
     *
     * Copies the description. When asked to allocate, the copy gains a row stride derived from
     * its format, `(mWidth + 2) / 2` for kABitmapFormatLinear4 and mWidth times the matching entry
     * of g_abBitmapBytesPerPixel otherwise, and a pixel rectangle allocated with the tag
     * "abitmap.h" and line 0x47. The copy's mByteCount is not updated. The format code then
     * selects the subclass through the jump table at 0x00837d90.
     *
     * The byte count passed to the allocator is a 64-bit product of the height and the stride,
     * computed through the helper at 0x00600270 and truncated to 32 bits.
     *
     * @param bitmap The description to copy.
     * @param bAllocatePixels Whether to allocate a fresh pixel rectangle for the copy.
     * @return The new canvas, or null for a format code of kABitmapFormatCount or more, or when
     *         the allocation returns null.
     * @ghidraAddress 0x005e8bc8
     */
    static ACanvas *CreateForBitmap(const ABitmap &bitmap, bool bAllocatePixels);

    /**
     * Construct a canvas over a fresh pixel rectangle shaped like a bitmap.
     *
     * Rewrites a format code of kABitmapFormatRle8 to kABitmapFormatLinear8 in a copy, then calls
     * CreateForBitmap() with allocation requested. The program lists no caller.
     *
     * @param bitmap The description to copy.
     * @return The new canvas, or null.
     * @ghidraAddress 0x005eb200
     */
    static ACanvas *CreateWithOwnedPixels(const ABitmap &bitmap);

    /**
     * Release the canvas.
     *
     * The compiled body reinstalls this class's virtual function table before releasing. That is
     * what the toolchain emits for a base destructor. The pixel rectangle is not released here.
     *
     * Every subclass destructor compiles to the same three instructions and adds no source, so no
     * subclass declares one.
     *
     * @ghidraAddress 0x005ead68
     */
    virtual ~ACanvas();

    /**
     * Set the pen colour from a palette index.
     *
     * @param nIndex The palette index.
     */
    virtual void SetColorIndex(int nIndex) = 0;

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
    virtual void SetColorRGB(const unsigned char *pRGB) = 0;

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
    virtual int GetColorIndex() = 0;

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
    virtual void GetColorRGB(unsigned char *pRGB) = 0;

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
    virtual void BuildAlphaFromColorKey(unsigned int nColorKey) = 0;

    /**
     * Store the pen colour at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     */
    virtual void PutPixelNoClip(int nX, int nY) = 0;

    /**
     * Store the pen colour at one point.
     *
     * Returns with no store for a point outside the clip rectangle.
     *
     * @param nX The column.
     * @param nY The row.
     * @ghidraAddress 0x005eb520
     */
    virtual void PutPixel(int nX, int nY);

    /**
     * Store one palette index at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nIndex The palette index.
     */
    virtual void PutPixelIndexedNoClip(int nX, int nY, int nIndex) = 0;

    /**
     * Store one palette index at one point, clipped.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nIndex The palette index.
     * @ghidraAddress 0x005eb590
     */
    virtual void PutPixelIndexed(int nX, int nY, int nIndex);

    /**
     * Store one 1555 colour at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nColor The colour.
     */
    virtual void PutPixel15NoClip(int nX, int nY, unsigned short nColor) = 0;

    /**
     * Store one 1555 colour at one point, clipped.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nColor The colour.
     * @ghidraAddress 0x005eb608
     */
    virtual void PutPixel15(int nX, int nY, unsigned short nColor);

    /**
     * Store one red, green, blue triple at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param pRGB The three channel bytes.
     */
    virtual void PutPixelRGBNoClip(int nX, int nY, const unsigned char *pRGB) = 0;

    /**
     * Store one red, green, blue triple at one point, clipped.
     *
     * @param nX The column.
     * @param nY The row.
     * @param pRGB The three channel bytes.
     * @ghidraAddress 0x005eb680
     */
    virtual void PutPixelRGB(int nX, int nY, const unsigned char *pRGB);

    /**
     * Store one 8888 colour at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nColor The colour.
     */
    virtual void PutPixelNoClip(int nX, int nY, unsigned int nColor) = 0;

    /**
     * Store one 8888 colour at one point, clipped.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nColor The colour.
     * @ghidraAddress 0x005eb6f0
     */
    virtual void PutPixel(int nX, int nY, unsigned int nColor);

    /**
     * Store one value in the canvas storage width at one point, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nColor The colour in the canvas pixel format.
     */
    virtual void PutPixelNativeNoClip(int nX, int nY, unsigned int nColor) = 0;

    /**
     * Store one value in the canvas storage width at one point, clipped.
     *
     * @param nX The column.
     * @param nY The row.
     * @param nColor The colour in the canvas pixel format.
     * @ghidraAddress 0x005eb760
     */
    virtual void PutPixelNative(int nX, int nY, unsigned int nColor);

    /**
     * Read one point as a palette index, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The palette index.
     */
    virtual int GetPixelIndexedNoClip(int nX, int nY) = 0;

    /**
     * Read one point as a palette index, clipped.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The palette index, or zero when the point is clipped away.
     * @ghidraAddress 0x005eb7d0
     */
    virtual int GetPixelIndexed(int nX, int nY);

    /**
     * Read one point as a 1555 colour, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The colour.
     */
    virtual unsigned short GetPixel15NoClip(int nX, int nY) = 0;

    /**
     * Read one point as a 1555 colour, clipped.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The colour, or zero when the point is clipped away.
     * @ghidraAddress 0x005eb848
     */
    virtual unsigned short GetPixel15(int nX, int nY);

    /**
     * Read one point into three bytes in red, green, blue order, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @param pRGB The three channel bytes to write.
     */
    virtual void GetPixelRGBNoClip(int nX, int nY, unsigned char *pRGB) = 0;

    /**
     * Read one point into three bytes in red, green, blue order, clipped.
     *
     * Clears the three bytes for a point outside the clip rectangle.
     *
     * @param nX The column.
     * @param nY The row.
     * @param pRGB The three channel bytes to write.
     * @ghidraAddress 0x005eb8c0
     */
    virtual void GetPixelRGB(int nX, int nY, unsigned char *pRGB);

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
    virtual unsigned int GetPixelNoClip(int nX, int nY) = 0;

    /**
     * Read one point as an 8888 colour, clipped.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The colour, or zero when the point is clipped away.
     * @ghidraAddress 0x005eb948
     */
    virtual unsigned int GetPixel(int nX, int nY);

    /**
     * Read one point in the canvas storage width, with no clip test.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The value in the canvas pixel format.
     */
    virtual unsigned int GetPixelNativeNoClip(int nX, int nY) = 0;

    /**
     * Read one point in the canvas storage width, clipped.
     *
     * @param nX The column.
     * @param nY The row.
     * @return The value, or zero when the point is clipped away.
     * @ghidraAddress 0x005eb9c0
     */
    virtual unsigned int GetPixelNative(int nX, int nY);

    /**
     * Fill part of one row with the pen colour, with no clip test.
     *
     * @param nY The row.
     * @param nLeft The first column.
     * @param nRight One past the last column.
     * @ghidraAddress 0x005eba38
     */
    virtual void FillRowNoClip(int nY, int nLeft, int nRight);

    /**
     * Fill part of one row with the pen colour.
     *
     * Clamps the column range to the clip rectangle and returns for a row outside it.
     *
     * @param nY The row.
     * @param nLeft The first column.
     * @param nRight One past the last column.
     * @ghidraAddress 0x005ebab8
     */
    virtual void FillRow(int nY, int nLeft, int nRight);

    /**
     * Fill part of one column with the pen colour, with no clip test.
     *
     * @param nX The column.
     * @param nTop The first row.
     * @param nBottom One past the last row.
     * @ghidraAddress 0x005ebb30
     */
    virtual void FillColumnNoClip(int nX, int nTop, int nBottom);

    /**
     * Fill part of one column with the pen colour, clamped to the clip rectangle.
     *
     * @param nX The column.
     * @param nTop The first row.
     * @param nBottom One past the last row.
     * @ghidraAddress 0x005ebbb0
     */
    virtual void FillColumn(int nX, int nTop, int nBottom);

    /**
     * Fill a rectangle with the pen colour, with no clip test.
     *
     * @param rect The rectangle.
     * @ghidraAddress 0x005ebc28
     */
    virtual void FillRectNoClip(ARect rect);

    /**
     * Fill a rectangle with the pen colour.
     *
     * Intersects the rectangle with the clip rectangle in place and returns for an empty result.
     *
     * @param rect The rectangle.
     * @ghidraAddress 0x005ebc98
     */
    virtual void FillRect(ARect rect);

    /**
     * Draw the four edges of a rectangle in the pen colour, with no clip test.
     *
     * @param rect The rectangle.
     * @ghidraAddress 0x005e9378
     */
    virtual void FrameRectNoClip(ARect rect);

    /**
     * Draw the four edges of a rectangle in the pen colour, through the clipped fills.
     *
     * @param rect The rectangle.
     * @ghidraAddress 0x005e9440
     */
    virtual void FrameRect(ARect rect);

    /**
     * Rewrite every palette index inside a rectangle through a remap table.
     *
     * @param rect The rectangle.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress 0x005ebd40
     */
    virtual void RemapRectIndices(ARect rect, const unsigned char *pRemap);

    /**
     * Draw a line in the pen colour, with no clip test.
     *
     * Every coordinate is 24.8 fixed point. SetupLineSteps() supplies a step of one whole unit
     * along the major axis and the pixel count.
     *
     * @param nX0 The first column.
     * @param nY0 The first row.
     * @param nX1 The last column.
     * @param nY1 The last row.
     * @ghidraAddress 0x005ebec8
     */
    virtual void DrawLineNoClip(int nX0, int nY0, int nX1, int nY1);

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
     * @ghidraAddress 0x005ebf70
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
     * @ghidraAddress 0x005ec050
     */
    virtual void TextureRowIndexed(int nY,
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
     * @ghidraAddress 0x005ec280
     */
    virtual void Blit4NoClip(const ABitmap &source, int nX, int nY);

    /**
     * Copy a four bit source bitmap, clipped.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress 0x005ec3c0
     */
    virtual void Blit4(const ABitmap &source, int nX, int nY);

    /**
     * Copy an eight bit source bitmap, with no clip test.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress 0x005ec450
     */
    virtual void Blit8NoClip(const ABitmap &source, int nX, int nY);

    /**
     * Copy an eight bit source bitmap, clipped.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress 0x005ec570
     */
    virtual void Blit8(const ABitmap &source, int nX, int nY);

    /**
     * Copy a 1555 source bitmap, with no clip test.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress 0x005ec600
     */
    virtual void Blit15NoClip(const ABitmap &source, int nX, int nY);

    /**
     * Copy a 1555 source bitmap, clipped.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress 0x005ec738
     */
    virtual void Blit15(const ABitmap &source, int nX, int nY);

    /**
     * Copy a 24 bit source bitmap, with no clip test.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress 0x005ec7c8
     */
    virtual void Blit24NoClip(const ABitmap &source, int nX, int nY);

    /**
     * Copy a 24 bit source bitmap, clipped.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress 0x005ec928
     */
    virtual void Blit24(const ABitmap &source, int nX, int nY);

    /**
     * Copy a 32 bit source bitmap, with no clip test.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress 0x005ec9b8
     */
    virtual void Blit32NoClip(const ABitmap &source, int nX, int nY);

    /**
     * Copy a 32 bit source bitmap, clipped.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress 0x005ecad8
     */
    virtual void Blit32(const ABitmap &source, int nX, int nY);

    /**
     * Copy a run length encoded eight bit source bitmap, with no clip test.
     *
     * Decodes one row into g_abCanvasRowScratch, describes the decoded row as an
     * eight bit ABitmap of one row, and copies it with Blit8NoClip().
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress 0x005ecb68
     */
    virtual void BlitRle8NoClip(const ABitmap &source, int nX, int nY);

    /**
     * Copy a run length encoded eight bit source bitmap, clipped.
     *
     * Forwards to BlitRle8NoClip() when the whole source fits inside the clip rectangle, and
     * otherwise decodes and copies the surviving rows one at a time.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @ghidraAddress 0x005e9cb8
     */
    virtual void BlitRle8(const ABitmap &source, int nX, int nY);

    /**
     * Read a rectangle of the canvas into a four bit bitmap, clipped.
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress 0x005ecef0
     */
    virtual void ReadRect4(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into a four bit bitmap, with no clip test.
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress 0x005ecdb8
     */
    virtual void ReadRect4NoClip(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into an eight bit bitmap, clipped.
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress 0x005ed070
     */
    virtual void ReadRect8(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into an eight bit bitmap, with no clip test.
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress 0x005ecf80
     */
    virtual void ReadRect8NoClip(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into a 1555 bitmap, clipped.
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress 0x005ed208
     */
    virtual void ReadRect15(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into a 1555 bitmap, with no clip test.
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress 0x005ed100
     */
    virtual void ReadRect15NoClip(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into a 24 bit bitmap, clipped.
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress 0x005ed398
     */
    virtual void ReadRect24(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into a 24 bit bitmap, with no clip test.
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress 0x005ed298
     */
    virtual void ReadRect24NoClip(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into a 32 bit bitmap, clipped.
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress 0x005ed520
     */
    virtual void ReadRect32(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into a 32 bit bitmap, with no clip test.
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress 0x005ed428
     */
    virtual void ReadRect32NoClip(const ABitmap &dest, int nX, int nY);

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
     * @ghidraAddress 0x005e9ee8
     */
    virtual void DrawGlyphNoClip(int nCharCode, const AFont *pFont, int nX, int nY);

    /**
     * Draw one glyph of a font, clipped.
     *
     * Selects the clipped copy slot through the table at 0x0077dcc8.
     *
     * @param nCharCode The character code.
     * @param pFont The font.
     * @param nX The destination column.
     * @param nY The baseline row.
     * @ghidraAddress 0x005e9fc8
     */
    virtual void DrawGlyph(int nCharCode, const AFont *pFont, int nX, int nY);

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
     * @ghidraAddress 0x005ed5b0
     */
    virtual void DrawTextNoClip(const char *pText, const AFont *pFont, int nX, int nY);

    /**
     * Draw a null terminated string, clipped.
     *
     * @param pText The string.
     * @param pFont The font.
     * @param nX The starting column.
     * @param nY The baseline row.
     * @ghidraAddress 0x005ea120
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
     * @ghidraAddress 0x005ed858
     */
    virtual void BlitRemap4(const ABitmap &source, int nX, int nY, const unsigned char *pRemap);

    /**
     * Copy an eight bit source bitmap through a remap table, one row at a time.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress 0x005eda30
     */
    virtual void BlitRemap8(const ABitmap &source, int nX, int nY, const unsigned char *pRemap);

    /**
     * Store one row of palette indices through a remap table.
     *
     * Skips a source value equal to the span transparent colour.
     *
     * @param span The row to store.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress 0x005edd08
     */
    virtual void RemapRowIndexed(const ARowSpan &span, const unsigned char *pRemap);

    /**
     * Copy a four bit source bitmap through a blend table.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress 0x005edfb8
     */
    virtual void
    BlitBlend4(const ABitmap &source, int nX, int nY, const unsigned char *const *ppBlend);

    /**
     * Copy an eight bit source bitmap through a blend table.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress 0x005ee1c8
     */
    virtual void
    BlitBlend8(const ABitmap &source, int nX, int nY, const unsigned char *const *ppBlend);

    /**
     * Store one row of palette indices blended against the destination.
     *
     * Reads the destination index, then selects the replacement from the blend row of the source
     * index. Index zero is transparent when the span requests transparency, which differs from
     * RemapRowIndexed(), where the span transparent colour applies instead.
     *
     * @param span The row to store.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress 0x005ee4a0
     */
    virtual void BlendRowIndexed(const ARowSpan &span, const unsigned char *const *ppBlend);

    /**
     * Store one row of palette indices sampled along a fixed step.
     *
     * @param span The row to store.
     * @ghidraAddress 0x005ee968
     */
    virtual void StretchRowIndexed(const AStretchSpan &span);

    /**
     * Store one row of 1555 colours sampled along a fixed step.
     *
     * @param span The row to store.
     * @ghidraAddress 0x005eea18
     */
    virtual void StretchRow15(const AStretchSpan &span);

    /**
     * Store one row of red, green, blue triples sampled along a fixed step.
     *
     * @param span The row to store.
     * @ghidraAddress 0x005eeac8
     */
    virtual void StretchRow24(const AStretchSpan &span);

    /**
     * Store one row of 8888 colours sampled along a fixed step.
     *
     * @param span The row to store.
     * @ghidraAddress 0x005eebb8
     */
    virtual void StretchRow32(const AStretchSpan &span);

    /**
     * Store one row of palette indices sampled along a fixed step, through a remap table.
     *
     * @param span The row to store.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress 0x005eedb8
     */
    virtual void StretchRowRemap(const AStretchSpan &span, const unsigned char *pRemap);

    /**
     * Store one row of palette indices sampled along a fixed step, blended against the
     * destination.
     *
     * @param span The row to store.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress 0x005eefc0
     */
    virtual void StretchRowBlend(const AStretchSpan &span, const unsigned char *const *ppBlend);

    ABitmap mBitmap; /*!< The pixel rectangle this canvas draws into. +0x00 */
    ARect mClip;     /*!< The clip rectangle every clipped operation tests against. +0x18 */

protected:
    // Every member below is reached only from this class and its subclasses, so protected is the
    // narrowest specifier the image supports. No access from outside the hierarchy was located.

    /**
     * Classify a point against the clip rectangle.
     *
     * @param nX The horizontal coordinate.
     * @param nY The vertical coordinate.
     * @return The ACanvasClipCode bits, or zero when the point is inside.
     * @ghidraAddress 0x005eb3d0
     */
    int ClipCodeForPoint(int nX, int nY) const;

    /**
     * Clip a line against the clip rectangle, rewriting both endpoints in place.
     *
     * Every coordinate is 24.8 fixed point.
     *
     * @param pnX0 The first column.
     * @param pnY0 The first row.
     * @param pnX1 The last column.
     * @param pnY1 The last row.
     * @return Zero when nothing survives.
     * @ghidraAddress 0x005e8fd8
     */
    int ClipLineToRect(int *pnX0, int *pnY0, int *pnX1, int *pnY1) const;

    /**
     * Clip a source bitmap and a destination position against the clip rectangle.
     *
     * Advances the pixel pointer, shrinks the extent, and moves the destination position, all in
     * place. Every clipped copy and read slot calls this first.
     *
     * @param pBitmap The bitmap to clip.
     * @param pnX The destination column.
     * @param pnY The destination row.
     * @return Zero when nothing survives.
     * @ghidraAddress 0x005e91b8
     */
    int ClipBlitToRect(ABitmap *pBitmap, int *pnX, int *pnY) const;

    /**
     * Copy a source bitmap through a remap table, choosing the arm by source format.
     *
     * Non-virtual, and orphaned in the shipped image: it fills no slot of this class's table and
     * the program lists no caller and no data reference. Format code 0 goes to BlitRemap4(), code 1
     * to BlitRemap8(), and code 5 to BlitRemapRle8NoClip(). Every other code returns without
     * drawing, so only the three indexed formats are handled.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress 0x005ed6a8
     */
    void BlitRemapNoClip(const ABitmap &source, int nX, int nY, const unsigned char *pRemap);

    /**
     * Clip against the clip rectangle and then copy through a remap table.
     *
     * The clip runs against a stack copy of the source description, because ClipBlitToRect()
     * rewrites what it is given. A zero result returns without drawing.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress 0x005ed718
     */
    void BlitRemap(const ABitmap &source, int nX, int nY, const unsigned char *pRemap);

    /**
     * Copy a run length encoded source through a remap table.
     *
     * The run length encoded arm of BlitRemapNoClip(), reached only for format code 5. It decodes
     * each row into g_abCanvasRowScratch through ARleReader and stores it with RemapRowIndexed().
     *
     * The row loop advances nY rather than the span row, so its bound recedes with the row and a
     * source of one row or more never finishes. BlitRemapNoClip() has no caller in the image, so
     * the defect is unreachable.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress 0x005edbd8
     */
    void BlitRemapRle8NoClip(const ABitmap &source, int nX, int nY, const unsigned char *pRemap);

    /**
     * Clip and copy a run length encoded source through a remap table.
     *
     * The clipped counterpart of BlitRemapRle8NoClip(), reached only from BlitRemap(). Rows above
     * the clip rectangle are consumed through ARleReader::SkipRows() rather than decoded.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress 0x005ea280
     */
    void BlitRemapRle8(const ABitmap &source, int nX, int nY, const unsigned char *pRemap);

    /**
     * Copy a source bitmap through a table of blend tables, choosing the arm by source format.
     *
     * Non-virtual and orphaned, with the same shape as BlitRemapNoClip(). Format code 0 goes to
     * BlitBlend4(), code 1 to BlitBlend8(), and code 5 to BlitBlendRle8NoClip().
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress 0x005ede08
     */
    void
    BlitBlendNoClip(const ABitmap &source, int nX, int nY, const unsigned char *const *ppBlend);

    /**
     * Clip against the clip rectangle and then copy through a table of blend tables.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress 0x005ede78
     */
    void BlitBlend(const ABitmap &source, int nX, int nY, const unsigned char *const *ppBlend);

    /**
     * Copy a run length encoded source through a table of blend tables.
     *
     * The same code as BlitRemapRle8NoClip() with BlendRowIndexed() in place of RemapRowIndexed(),
     * including the row loop that never finishes. BlitBlendNoClip() has no caller in the image.
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress 0x005ee370
     */
    void
    BlitBlendRle8NoClip(const ABitmap &source, int nX, int nY, const unsigned char *const *ppBlend);

    /**
     * Clip and copy a run length encoded source through a table of blend tables.
     *
     * The same code as BlitRemapRle8() with BlendRowIndexed() in place of RemapRowIndexed().
     *
     * @param source The source bitmap.
     * @param nX The destination column.
     * @param nY The destination row.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress 0x005ea460
     */
    void BlitBlendRle8(const ABitmap &source, int nX, int nY, const unsigned char *const *ppBlend);

    /**
     * Read a rectangle of the canvas into a bitmap, choosing the slot by destination format.
     *
     * Non-virtual and orphaned, like BlitRemapNoClip(). Calls through the six entry table of
     * pointers to member functions at 0x0077dcf8, whose first five entries address the unclipped
     * read slots ReadRect4NoClip() through ReadRect32NoClip() and whose sixth is ReadRectRle8().
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress 0x005ecc68
     */
    void ReadRectNoClip(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into a bitmap, clipped, choosing the slot by destination
     * format.
     *
     * Non-virtual and orphaned. Calls through the table at 0x0077dd28, which addresses the clipped
     * read slots ReadRect4() through ReadRect32() and then ReadRectRle8().
     *
     * @param dest The destination bitmap, whose extent selects the rectangle.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress 0x005ecd10
     */
    void ReadRect(const ABitmap &dest, int nX, int nY);

    /**
     * Read a rectangle of the canvas into a run length encoded bitmap.
     *
     * Empty. The run length encoded entry of both read tables addresses this one body, so a read
     * into kABitmapFormatRle8 does nothing.
     *
     * @param dest The destination bitmap.
     * @param nX The source column.
     * @param nY The source row.
     * @ghidraAddress 0x005eb190
     */
    void ReadRectRle8(const ABitmap &dest, int nX, int nY);

    /**
     * Prepare a stretched copy of a source bitmap into a destination rectangle.
     *
     * Both steps are the source extent in 24.8 fixed point divided by the destination extent, and
     * each position starts at half its step, so the walk samples the centre of each source cell.
     * The destination rectangle is clamped to the clip rectangle, and the positions advance by one
     * step per row or column clamped away. AStretchSpan::mSource addresses the first source row
     * the walk samples.
     *
     * The palette resolves from the source, then the canvas, then g_pDefaultPalette.
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle, before clipping.
     * @param pBlit The record to fill.
     * @return Zero when the rectangle is empty before or after clipping.
     * @ghidraAddress 0x005ea780
     */
    int SetupStretchBlit(const ABitmap &source, const ARect &rect, AStretchBlit *pBlit) const;

    /**
     * Stretch a source bitmap into a destination rectangle, choosing the arm by source format.
     *
     * Non-virtual and orphaned. Calls through the six entry table of pointers to member functions
     * at 0x0077dd58, whose entries are StretchBlit4() through StretchBlit32() and then
     * StretchBlitRle8(). Every arm clips against the clip rectangle.
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @ghidraAddress 0x005ee580
     */
    void StretchBlit(const ABitmap &source, const ARect &rect);

    /**
     * Stretch a four bit source bitmap into a destination rectangle.
     *
     * Unpacks each sampled row into g_abCanvasRowScratch and stores it with StretchRowIndexed().
     * The row walk starts at the first source row rather than at the row SetupStretchBlit()
     * selected, so a rectangle clipped at the top samples rows from too high in the source.
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @ghidraAddress 0x005ea640
     */
    void StretchBlit4(const ABitmap &source, const ARect &rect);

    /**
     * Stretch an eight bit source bitmap into a destination rectangle, through StretchRowIndexed().
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @ghidraAddress 0x005ee628
     */
    void StretchBlit8(const ABitmap &source, const ARect &rect);

    /**
     * Stretch a 1555 source bitmap into a destination rectangle, through StretchRow15().
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @ghidraAddress 0x005ee6f8
     */
    void StretchBlit15(const ABitmap &source, const ARect &rect);

    /**
     * Stretch a 24 bit source bitmap into a destination rectangle, through StretchRow24().
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @ghidraAddress 0x005ee7c8
     */
    void StretchBlit24(const ABitmap &source, const ARect &rect);

    /**
     * Stretch a 32 bit source bitmap into a destination rectangle, through StretchRow32().
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @ghidraAddress 0x005ee898
     */
    void StretchBlit32(const ABitmap &source, const ARect &rect);

    /**
     * Stretch a run length encoded source into a destination rectangle.
     *
     * Decodes the first sampled row into g_abCanvasRowScratch, then decodes again only when the
     * sampled row changes, consuming any rows between through ARleReader::SkipRows().
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @ghidraAddress 0x005ea960
     */
    void StretchBlitRle8(const ABitmap &source, const ARect &rect);

    /**
     * Stretch an indexed source through a remap table, choosing the arm by source format.
     *
     * Non-virtual and orphaned. Format code 0 goes to StretchBlitRemap4(), code 1 to
     * StretchBlitRemap8(), and code 5 to StretchBlitRemapRle8(). Every other code draws nothing.
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress 0x005eec70
     */
    void StretchBlitRemap(const ABitmap &source, const ARect &rect, const unsigned char *pRemap);

    /**
     * Stretch a four bit source through a remap table. Empty.
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress 0x005eecd0
     */
    void StretchBlitRemap4(const ABitmap &source, const ARect &rect, const unsigned char *pRemap);

    /**
     * Stretch an eight bit source through a remap table, through StretchRowRemap().
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress 0x005eecd8
     */
    void StretchBlitRemap8(const ABitmap &source, const ARect &rect, const unsigned char *pRemap);

    /**
     * Stretch a run length encoded source through a remap table.
     *
     * The same code as StretchBlitRle8() with StretchRowRemap() in place of StretchRowIndexed().
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @param pRemap 256 replacement indices, one per source index.
     * @ghidraAddress 0x005eaa98
     */
    void
    StretchBlitRemapRle8(const ABitmap &source, const ARect &rect, const unsigned char *pRemap);

    /**
     * Stretch an indexed source blended against the destination, choosing the arm by source
     * format.
     *
     * Non-virtual and orphaned. The first two cases are exchanged relative to StretchBlitRemap().
     * Format code 0 goes to StretchBlitBlend8() and code 1 to the empty StretchBlitBlend4(), so a
     * four bit source is read as eight bit and an eight bit source draws nothing. Code 5 goes to
     * StretchBlitBlendRle8().
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress 0x005eee78
     */
    void
    StretchBlitBlend(const ABitmap &source, const ARect &rect, const unsigned char *const *ppBlend);

    /**
     * Stretch a four bit source blended against the destination. Empty.
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress 0x005eeed8
     */
    void StretchBlitBlend4(const ABitmap &source,
                           const ARect &rect,
                           const unsigned char *const *ppBlend);

    /**
     * Stretch an eight bit source blended against the destination, through StretchRowBlend().
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress 0x005eeee0
     */
    void StretchBlitBlend8(const ABitmap &source,
                           const ARect &rect,
                           const unsigned char *const *ppBlend);

    /**
     * Stretch a run length encoded source blended against the destination.
     *
     * The same code as StretchBlitRle8() with StretchRowBlend() in place of StretchRowIndexed().
     *
     * @param source The source bitmap.
     * @param rect The destination rectangle.
     * @param ppBlend 256 rows of 256 replacement indices, selected by source then destination.
     * @ghidraAddress 0x005eabe0
     */
    void StretchBlitBlendRle8(const ABitmap &source,
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
     * @ghidraAddress 0x005e9508
     */
    static int SetupLineSteps(int nX0, int nY0, int nX1, int nY1, int *pnStepX, int *pnStepY);

    /**
     * Unpack one row of four bit pixels into one byte each.
     *
     * The low nibble of a byte supplies the earlier pixel.
     *
     * @param pSource The packed row.
     * @param pDest The unpacked row.
     * @param nCount The number of pixels.
     * @param bStartHighNibble Whether the first pixel is the high nibble of the first byte.
     * @ghidraAddress 0x005eddb8
     */
    static void UnpackNibbleRow(const unsigned char *pSource,
                                unsigned char *pDest,
                                int nCount,
                                int bStartHighNibble);
};

/**
 * Palette every canvas falls back to when neither its bitmap nor its source supplies one.
 *
 * Read by ACanvas8::SetColorIndex() and every other slot that resolves a palette index, always
 * after the bitmap palette is found null.
 *
 * Every reference the analysis program lists is a read, 40 of them, and the listing may be
 * truncated at that count. No writer was located, so the fallback may be permanently null.
 *
 * @ghidraAddress 0x0086f6f0
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
 * @ghidraAddress 0x008f09f0
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
 * @ghidraAddress 0x00618f38
 */
unsigned short APackRgb1555From8888(unsigned int nColor);
