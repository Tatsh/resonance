#pragma once

/** Value of ARleReader::mTransparentValue that stores every decoded byte. */
constexpr int kARleReaderNoTransparentValue = -1;

/**
 * Cursor over a run length encoded eight bit pixel stream.
 *
 * The record is not polymorphic and has no RTTI, and the image retains no title for it. The name
 * here is inferred from the two routines that consume it.
 *
 * The two routines sit at 0x0060da78 and 0x0060dc10, well outside the canvas cluster, and a third
 * caller named DecompressRlePixelsToBuffer at 0x0060dcb8 sits between them. 13 of their 15 callers
 * are ACanvas slots, so the routines are described here. Which source file they belong to is not
 * settled.
 *
 * The encoding is one control byte followed by data. The low seven bits are the run length. A set
 * top bit introduces that many literal bytes, and a clear top bit introduces one byte repeated
 * that many times.
 *
 * A control byte of zero terminates the stream, and both routines examine it only at a row
 * boundary. A row ends when its pixel count is exhausted rather than on a zero, and a zero control
 * byte reached part way through a row is decoded as a run of length zero.
 *
 * mTransparentValue changes what a run does to the destination rather than what it consumes. A
 * value of zero or more skips a byte equal to it, and kARleReaderNoTransparentValue stores every
 * byte.
 */
struct ARleReader {
    /**
     * Decode one row into a buffer.
     *
     * @param pDest The row to write.
     * @return pDest advanced past the row.
     * @ghidraAddress 0x0060da78
     */
    unsigned char *DecodeRow(unsigned char *pDest);

    /**
     * Advance past a number of rows without writing.
     *
     * @param nRows The number of rows to consume.
     * @ghidraAddress 0x0060dc10
     */
    void SkipRows(int nRows);

    const unsigned char *mSource; /*!< The next control byte. +0x00 */
    short mWidth;                 /*!< Pixels in one row. +0x04 */
    short mPad06;                 /*!< Padding no caller writes. +0x06 */
    int mTransparentValue;        /*!< The byte to skip, or kARleReaderNoTransparentValue. +0x08 */
};
