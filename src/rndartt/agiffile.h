#pragma once

#include "rndartt/agfxfile.h"

/**
 * Reader for GIF files.
 *
 * The name comes from the RTTI descriptor whose mangled form is `8AGifFile`, with the single
 * public base AGfxFile at offset zero. Its table at 0x0083fdd0 overrides slots 1, 3, 4, and 5, and
 * the class adds no member. AGfxFile::Open() allocates 0x18 bytes for it.
 *
 * Every piece of decoding state lives in file-scope statics rather than in the object: the
 * logical screen descriptor, the current image descriptor and graphic control block, the
 * decoder tables, and a static APalette the global colour table is read into. A local colour
 * table overwrites that palette. Two readers in use at once therefore share one state.
 */
class AGifFile : public AGfxFile {
public:
    /**
     * Construct over an open file.
     *
     * Inline. AGfxFile::Open() compiles it.
     *
     * @param pFile The open file.
     */
    explicit AGifFile(FILE *pFile) : AGfxFile(pFile) {
    }

    /**
     * Slot 1. Read the logical screen descriptor and the global colour table.
     *
     * A signature that does not start `GIF` gives kAGfxFileBadFormat. The graphic control block
     * is cleared. The version and the screen extent are read but not used.
     *
     * @return An AGfxFileResult code.
     * @ghidraAddress 0x0062a9c0
     */
    virtual int ReadHeader();

    /**
     * Slot 3. Read blocks up to and including the next image.
     *
     * Extension blocks are consumed through ReadExtensionBlock(). An image produces an eight bit
     * bitmap the size of its descriptor, with a new palette copied from the static palette, and
     * sets mBounds to the image rectangle. A transparent colour from the last graphic control
     * block is applied to the bitmap.
     *
     * A block introducer of one is accepted when the next three bytes are 1, 0, and `!`, and an
     * extension follows.
     *
     * @param pImage Receives the image.
     * @param pbEnd Set to one at the trailer, and also for a negative introducer byte.
     * @return An AGfxFileResult code.
     * @ghidraAddress 0x0062aaf8
     */
    virtual int ReadImage(ABitmap *pImage, int *pbEnd);

    /**
     * Slot 5. Writing is not implemented.
     *
     * @param bitmap The bitmap, not read.
     * @return kAGfxFileUnsupported.
     * @ghidraAddress 0x0062b6f0
     */
    virtual int Write(const ABitmap &bitmap);

private:
    /**
     * Read one extension block after its introducer.
     *
     * A graphic control block is stored and its delay, in hundredths of a second, is added to
     * mDuration in milliseconds. A plain text, comment, or application block is skipped with its
     * data sub-blocks. Any other label skips one block and no sub-blocks. Every length byte is
     * read signed, so a length above 127 ends the skip.
     *
     * @ghidraAddress 0x0062ae10
     */
    void ReadExtensionBlock();

    /**
     * Decode the LZW data of one image into a pixel rectangle.
     *
     * Rows are written at the image width, in interlaced order when the image descriptor asks
     * for it.
     *
     * @param pFile The file, positioned after the minimum code size.
     * @param nCodeSize The minimum code size, from 2 to 8.
     * @param pDest The pixel rectangle, one byte per pixel.
     * @return One when the image decoded to its end code or its last row, zero on any error.
     * @ghidraAddress 0x0062b008
     */
    static int DecodeLzwImage(FILE *pFile, int nCodeSize, unsigned char *pDest);
};
