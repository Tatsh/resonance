#pragma once

/**
 * Ring buffer the movie stream reads its file through.
 *
 * The class is not polymorphic and emits no RTTI, so its name is inferred from its dump routine,
 * which prints "circbuff:" and the member names pRead, pWrite, pWrap, pBuff, and buffsz. The
 * movie stream's chunk error message calls the same object "pCircBuff". The buffer memory belongs
 * to the caller, and the class never frees it.
 *
 * Bytes are written at mWrite and consumed at mRead. mWrap marks the end of the usable region,
 * and a pointer that reaches it wraps back to mBuff. A write pointer equal to the read pointer
 * reports no free space rather than an empty buffer.
 */
class CircBuff {
public:
    /**
     * Take over a buffer of nSize bytes with both pointers at its start.
     *
     * @param pBuff The buffer.
     * @param nSize The buffer size in bytes.
     * @ghidraAddress 0x0060e8b0
     */
    CircBuff(char *pBuff, int nSize);

    /**
     * Report how many bytes can be written before the write pointer meets the read pointer.
     *
     * One byte is always held back. A write pointer equal to the read pointer reports zero.
     *
     * @return The free space in bytes.
     * @ghidraAddress 0x0060e930
     */
    int FreeSpace();

    /**
     * Report whether nBytes fit in one contiguous write. The routine has no caller.
     *
     * @param nBytes The size to test.
     * @return Non-zero when the bytes fit.
     * @ghidraAddress 0x0060e998
     */
    int HasSpace(int nBytes);

    /**
     * Move the write pointer back to mBuff once it has reached mWrap, unless the read pointer is
     * still at mBuff.
     *
     * @ghidraAddress 0x0060ea20
     */
    void WrapWrite();

    /**
     * Consume nBytes, wrapping the read pointer past mWrap.
     *
     * @param nBytes The number of bytes consumed.
     * @return The new read pointer.
     * @ghidraAddress 0x0060ea50
     */
    char *AdvanceRead(int nBytes);

    /**
     * Report whether the write pointer lies outside the nBytes starting at pStart.
     *
     * The range wraps past mWrap to mBuff.
     *
     * @param pStart The start of the range.
     * @param nBytes The length of the range.
     * @return Non-zero when the write pointer is outside the range.
     * @ghidraAddress 0x0060ea80
     */
    int IsClearOfWrite(const char *pStart, int nBytes) const;

    /**
     * Clamp nBytes to the space between the write pointer and mWrap.
     *
     * @param nBytes The size requested.
     * @return The size that can be written without wrapping.
     * @ghidraAddress 0x0060eaf0
     */
    int ContiguousWriteSize(int nBytes) const;

    /**
     * Commit nBytes written at the write pointer.
     *
     * The write pointer wraps to mBuff once it reaches mWrap, unless the read pointer is still at
     * mBuff.
     *
     * @param nBytes The number of bytes written.
     * @return The new write pointer.
     * @ghidraAddress 0x0060eb18
     */
    char *AdvanceWrite(int nBytes);

    /**
     * Copy nBytes into the buffer in one piece. The routine has no caller.
     *
     * The bytes go at the write pointer, or at mBuff when they do not fit before mWrap. Nothing is
     * copied unless they fit short of the read pointer.
     *
     * @param pSrc The bytes to copy.
     * @param nBytes The number of bytes.
     * @return nBytes, or 0 when nothing was copied.
     * @ghidraAddress 0x0060eb48
     */
    int Write(const void *pSrc, int nBytes);

    /**
     * Read nBytes from a file into the buffer in one piece. The routine has no caller.
     *
     * The placement rule is the one Write() uses. The count FileRead() returns is ignored, and the
     * write pointer advances by nBytes regardless.
     *
     * @param nFile The file to read.
     * @param nBytes The number of bytes.
     * @return nBytes, or 0 when nothing was read.
     * @ghidraAddress 0x0060ec08
     */
    int ReadFromFile(int nFile, int nBytes);

    /**
     * Log the pointers and the size under a label.
     *
     * @param pszLabel The label printed first.
     * @ghidraAddress 0x0060ecd0
     */
    void Dump(const char *pszLabel) const;

    // The deleting destructor at 0x0060e8d0 frees the object and nothing else, so the class
    // declares no destructor of its own.

    char *mBuff;   /*!< Start of the buffer. +0x00 */
    int mBuffSize; /*!< Size of the buffer in bytes. +0x04 */
    char *mWrite;  /*!< Where the next byte is written. +0x08 */
    char *mRead;   /*!< Where the next byte is consumed. +0x0c */
    char *mWrap;   /*!< End of the usable region, mBuff plus mBuffSize. +0x10 */
};
