#pragma once

#include "rndartt/agfxfile.h"

class APalette;

/**
 * Reader and writer for Windows bitmap files.
 *
 * The name comes from the RTTI descriptor whose mangled form is `8ABmpFile`, with the single
 * public base AGfxFile at offset zero. Its table at 0x0083d1c0 overrides slots 1, 3, 4, and 5.
 * AGfxFile::Open() allocates 0x3c bytes for it.
 *
 * The reader accepts a 40 byte information header with one plane at 4, 8, 16, or 24 bits per
 * pixel, uncompressed or run length encoded. A 24 bit file is read into a
 * kABitmapFormatLinear32 bitmap with full alpha. A 16 or 24 bit row has its red and blue fields
 * exchanged, and a 4 bit row has the two nibbles of each byte exchanged, which gives the nibble
 * order ACanvasLin4 uses.
 */
class ABmpFile : public AGfxFile {
public:
    /**
     * Construct over an open file.
     *
     * Inline. AGfxFile::Open() compiles it.
     *
     * @param pFile The open file.
     */
    explicit ABmpFile(FILE *pFile) : AGfxFile(pFile) {
    }

    /**
     * Slot 1. Read both headers and record the image geometry.
     *
     * Both headers are read into file-scope buffers first, at 0x008e68e0 and 0x008e68f0. A file
     * type other than `BM`, an information header size other than 40, a plane count other than
     * one, or a one bit image gives kAGfxFileBadFormat. A negative height marks a top-down file,
     * and mBounds receives the width and the height as the file records them, before the sign is
     * removed.
     *
     * @return An AGfxFileResult code.
     * @ghidraAddress 0x0061c688
     */
    virtual int ReadHeader();

    /**
     * Slot 3. Read the one image the file holds.
     *
     * A second call reports the end. An unsupported pixel width returns kAGfxFileBadFormat
     * without releasing the palette already read, and a failed read releases the pixel rectangle
     * through the single-object path although the allocation came from the tagged allocator.
     *
     * @param pImage Receives the image and its palette.
     * @param pbEnd Set to one on a second call.
     * @return An AGfxFileResult code.
     * @ghidraAddress 0x0061c860
     */
    virtual int ReadImage(ABitmap *pImage, int *pbEnd);

    /**
     * Slot 5. Write a bitmap as an uncompressed file.
     *
     * Accepts kABitmapFormatLinear8, kABitmapFormatLinear15, and kABitmapFormatLinear24, and
     * returns kAGfxFileBadFormat for any other format. An eight bit bitmap writes 256 palette
     * entries, zero when the bitmap has no palette. Rows are written bottom-up, each padded to a
     * multiple of four bytes, and neither the 16 nor the 24 bit rows have their red and blue
     * fields exchanged, unlike the reader.
     *
     * @param bitmap The bitmap to write.
     * @return An AGfxFileResult code.
     * @ghidraAddress 0x0061ca78
     */
    virtual int Write(const ABitmap &bitmap);

private:
    /**
     * Read the colour table into a new palette.
     *
     * @return The palette, holding mColorCount entries with full alpha.
     * @ghidraAddress 0x0061cde0
     */
    APalette *ReadPalette();

    /**
     * Seek to the pixels and read them with the reader mCompression selects.
     *
     * @param pImage The bitmap to fill.
     * @return An AGfxFileResult code.
     * @ghidraAddress 0x0061d5c0
     */
    int ReadPixels(ABitmap *pImage);

    /**
     * Read uncompressed rows, in file order.
     *
     * @param pImage The bitmap to fill.
     * @return kAGfxFileOk.
     * @ghidraAddress 0x0061cef8
     */
    int ReadUncompressedPixels(ABitmap *pImage);

    /**
     * Read run length encoded rows, in file order.
     *
     * Handles both the eight bit and the four bit encoding. The pixel rectangle is cleared first,
     * so pixels a delta or an early end of line skip read as zero.
     *
     * @param pImage The bitmap to fill.
     * @return kAGfxFileOk, or kAGfxFileBadFormat when a run overflows its row.
     * @ghidraAddress 0x0061d0e8
     */
    int ReadRlePixels(ABitmap *pImage);

    /**
     * Widen a row of three byte pixels to four bytes in place, with full alpha.
     *
     * Walks from the last pixel back to the first, so the widened row can overlap the source.
     *
     * @param pPixels The row.
     * @param nCount The number of pixels.
     * @ghidraAddress 0x0061d620
     */
    static void ExpandRow24To32(unsigned char *pPixels, int nCount);

    int mPixelOffset;         // +0x18 File offset of the pixels.
    int mPaletteOffset;       // +0x1c File offset of the colour table.
    int mCompression;         // +0x20 Zero for uncompressed rows.
    unsigned short mBitCount; // +0x24 Bits per pixel.
    int mWidth;               // +0x28
    int mHeight;              // +0x2c Always positive.
    int mRowStep;             // +0x30 -1 for a bottom-up file, 1 for a top-down one.
    int mColorCount;          // +0x34 Colour table entries, zero above eight bits per pixel.
    int mImageRead;           // +0x38 Set once ReadImage() has returned the image.
};
