#pragma once

#include "rndartt/agfxfile.h"

/**
 * Reader for Truevision TGA files.
 *
 * The name comes from the RTTI descriptor whose mangled form is `8ATgaFile`, with the single
 * public base AGfxFile at offset zero. Its table at 0x0083d8d0 overrides slots 1, 3, 4, and 5.
 * AGfxFile::Open() allocates 0x30 bytes for it.
 *
 * The reader accepts uncompressed and run length encoded true colour images with no colour map,
 * image types 2 and 10, and always produces a kABitmapFormatLinear32 bitmap. A pixel depth other
 * than 32 is read as three bytes a pixel.
 */
class ATgaFile : public AGfxFile {
public:
    /**
     * Construct over an open file.
     *
     * Inline. AGfxFile::Open() compiles it.
     *
     * @param pFile The open file.
     */
    explicit ATgaFile(FILE *pFile) : AGfxFile(pFile) {
    }

    /**
     * Slot 1. Read the 18 byte header and skip the image identifier.
     *
     * A colour map, or an image type other than 2 and 10, gives kAGfxFileBadFormat.
     *
     * @return An AGfxFileResult code.
     * @ghidraAddress 0x0061fff8
     */
    virtual int ReadHeader();

    /**
     * Slot 3. Read the image.
     *
     * The end is reported only once mImageRead is set, and nothing sets it, so every call reads
     * again from the current file position. A failed read releases the pixel rectangle through
     * the single-object path.
     *
     * @param pImage Receives the image.
     * @param pbEnd Set to one when mImageRead is set.
     * @return An AGfxFileResult code.
     * @ghidraAddress 0x00620508
     */
    virtual int ReadImage(ABitmap *pImage, int *pbEnd);

    /**
     * Slot 5. Writing is not implemented.
     *
     * @param bitmap The bitmap, not read.
     * @return kAGfxFileUnsupported.
     * @ghidraAddress 0x00620500
     */
    virtual int Write(const ABitmap &bitmap);

private:
    /**
     * Read the pixels of every row into a 32 bit bitmap.
     *
     * Rows run bottom-up unless mTopDown is set. Each pixel is stored in red, green, blue, alpha
     * order with the red and blue bytes of the file exchanged. An uncompressed 24 bit pixel
     * gains full alpha, while a run length encoded 24 bit pixel takes its fourth byte from a
     * static buffer that no read fills, which stays zero unless a 32 bit read has filled it.
     *
     * @param pImage The bitmap to fill.
     * @return kAGfxFileOk.
     * @ghidraAddress 0x00620128
     */
    int ReadPixels(ABitmap *pImage);

    int mTopDown;    // +0x18 Set when either origin bit of the image descriptor is set.
    int mRle;        // +0x1c Set for image type 10.
    int mPixelDepth; // +0x20 Bits per pixel.
    int mWidth;      // +0x24
    int mHeight;     // +0x28
    int mImageRead;  // +0x2c Cleared by ReadHeader() and never set.
};
