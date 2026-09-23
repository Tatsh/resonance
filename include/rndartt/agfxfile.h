#pragma once

#include <stdio.h>

#include "rndartt/arect.h"

struct ABitmap;

/** Results the AGfxFile members return. */
enum AGfxFileResult {
    kAGfxFileOk = 0,           /*!< The operation succeeded. */
    kAGfxFileNoMemory = -1,    /*!< The pixel rectangle could not be allocated. */
    kAGfxFileOpenFailed = -2,  /*!< The file could not be opened. */
    kAGfxFileUnsupported = -4, /*!< The format does not implement the operation. */
    kAGfxFileBadFormat = -6    /*!< The file content is not in a form the reader accepts. */
};

/**
 * Image file reader and writer, one subclass per file format.
 *
 * The name comes from the RTTI descriptor whose mangled form is `8AGfxFile`, which records no base
 * class. ABmpFile, ATgaFile, and AGifFile derive from it. The virtual function table pointer sits
 * after the data members at offset 0x14, and the object is 0x18 bytes, the size AGifFile
 * allocates with no member of its own.
 *
 * The table has six slots. Slot 0 is the type function, and slots 1, 3, and 5 point at the shared
 * pure virtual stub. Slot 2 and the destructor at slot 4 are inline, so every translation unit that
 * needs the table emits its own copy of it, at 0x0083d218, 0x0083d928, and 0x0083fe28.
 *
 * mFile and mDuration are written by the subclasses and mBounds by their header readers, so all
 * three are protected. mUnknown04 is written only by Open() and SetUnknown04(), and no reader of
 * it was located.
 */
class AGfxFile {
public:
    /**
     * Open an image file and construct the reader for its extension.
     *
     * The extension selects the class through ExtensionCode(): `BMP`, `DIB`, and `RLE` give
     * ABmpFile, `TGA` gives ATgaFile, and `GIF` gives AGifFile. Each is allocated through the
     * plain allocator and receives the open file.
     *
     * An unrecognised extension leaves the result null and then writes mDuration through it,
     * which dereferences null, and the open file is not closed. The program lists one caller,
     * WriteBitmap().
     *
     * @param pszPath The file to open.
     * @param pnError Receives kAGfxFileOk, or kAGfxFileOpenFailed when the file cannot be opened.
     * @param bRead True to open for reading, false to open for writing.
     * @return The reader, or null when the file cannot be opened.
     * @ghidraAddress 0x005f9bc8
     */
    static AGfxFile *Open(const char *pszPath, int *pnError, bool bRead);

    /**
     * Write a bitmap to a file in the format its extension selects.
     *
     * VramTable::Screendump() is the one caller.
     *
     * @param pszPath The file to write.
     * @param bitmap The bitmap to write.
     * @return An AGfxFileResult code.
     * @ghidraAddress 0x005f9d18
     */
    static int WriteBitmap(const char *pszPath, const ABitmap &bitmap);

    /**
     * Return the first three characters after the last full stop of a path, packed into a word.
     *
     * Each character is converted to upper case through the ctype table, and the first
     * character lands in the low byte. Zero results when the path has no full stop or when a path
     * separator follows it, and a shorter extension leaves the remaining bytes zero.
     *
     * @param pszPath The path.
     * @return The packed extension, `BMP` being 0x504d42.
     * @ghidraAddress 0x0062f560
     */
    static int ExtensionCode(const char *pszPath);

    /**
     * Construct over an open file.
     *
     * Inline. Open() compiles it for every subclass.
     *
     * @param pFile The open file.
     */
    explicit AGfxFile(FILE *pFile) : mFile(pFile), mUnknown04(-1) {
    }

    /**
     * Read the file header.
     *
     * @return An AGfxFileResult code.
     */
    virtual int ReadHeader() = 0;

    /**
     * Set the word at offset 0x04.
     *
     * Inline, emitted once at 0x0061d4c0. No reader of the word was located, so its meaning is
     * not recovered.
     *
     * @param nValue The value to store.
     * @ghidraAddress 0x0061d4c0
     */
    virtual void SetUnknown04(int nValue) {
        mUnknown04 = nValue;
    }

    /**
     * Read the next image in the file.
     *
     * @param pImage Receives the image, with a freshly allocated pixel rectangle.
     * @param pbEnd Set to one when the file holds no further image.
     * @return An AGfxFileResult code.
     */
    virtual int ReadImage(ABitmap *pImage, int *pbEnd) = 0;

    /**
     * Close the file.
     *
     * Inline. Each subclass destructor is the same body emitted again, at 0x0061d4c8, 0x00620408,
     * and 0x0062b5f8.
     *
     * @ghidraAddress 0x0061d6c0
     */
    virtual ~AGfxFile() {
        if (mFile != nullptr) {
            fclose(mFile);
        }
    }

    /**
     * Write a bitmap to the file.
     *
     * @param bitmap The bitmap to write.
     * @return An AGfxFileResult code.
     */
    virtual int Write(const ABitmap &bitmap) = 0;

protected:
    FILE *mFile; // +0x00 The open file.

private:
    int mUnknown04; // +0x04 Set to -1 on construction and by SetUnknown04().

protected:
    ARect mBounds; // +0x08 The image rectangle the last header or image descriptor gave.
    int mDuration; // +0x10 Display time accumulated from GIF graphic control blocks, in ms.
};
