#pragma once

#include "os/hxstr.h"
#include "stream/hxstream.h"

/**
 * Read-only HxStream over a caller-supplied byte range.
 *
 * `11HxMemStream` in the RTTI descriptor at `0x008f00c0`, single inheritance from HxStream at
 * offset 0. The type function is at `0x00405c80` and its vtable is at `0x00816e08`. The object is
 * 0x24 bytes: the HxStream subobject occupies `+0x00` through `+0x0f`, the name occupies `+0x10`
 * and `+0x14`, and the three pointers follow at `+0x18`, `+0x1c`, and `+0x20`.
 *
 * The class adds no virtual of its own and overrides six of HxStream's seven, retaining only
 * Unknown7(). The range belongs to the caller, and the destructor releases only the name.
 *
 * Write() always reports a fatal error, which is what makes the stream read-only. One call site
 * constructs one, at `0x001e676c`.
 */
class HxMemStream : public HxStream {
public:
    /**
     * Open a stream over a caller-owned byte range.
     *
     * @param pszName The stream title, which appears in the diagnostic Read() produces at the end
     * of the data. A copy is stored.
     * @param pData The first byte of the range. The stream does not take ownership.
     * @param nSize The range length in bytes.
     * @ghidraAddress 0x00405cf8
     */
    HxMemStream(const char *pszName, char *pData, int nSize);

    /**
     * Close the stream and release the title.
     *
     * @ghidraAddress 0x00405d98
     */
    virtual ~HxMemStream();

    /**
     * Move the read position, clamping to the range.
     *
     * The status word is cleared on every call, and a target past the end then clamps to the end
     * and sets it to 1. A target below the start clamps to the start.
     *
     * @param nOffset The signed distance to move.
     * @param nWhence The origin, one of the HxStreamSeekOrigin values. An origin outside 0 to 2 is
     * treated as the start.
     * @ghidraAddress 0x00405e00
     */
    virtual void Seek(int nOffset, int nWhence);

    /** @ghidraAddress 0x00405e80 */
    virtual int Tell();

    /** @ghidraAddress 0x00405e90 */
    virtual int Size();

    /**
     * Report a fatal error, since the stream is read-only.
     *
     * Both arguments are discarded.
     *
     * @param pSrc The source buffer.
     * @param nSize The number of bytes to move.
     * @return This stream, which the routine never arrives at.
     * @ghidraAddress 0x00405ea0
     */
    virtual HxStream &Write(const void *pSrc, int nSize);

    /**
     * Move up to nSize bytes into pDest.
     *
     * A short read is clamped rather than reported. Once the read position is at the end of the
     * range, the status word becomes 1, and when the inherited fatal flag is set the routine also
     * reports a fatal error.
     *
     * @param pDest The destination buffer.
     * @param nSize The number of bytes to move.
     * @return This stream.
     * @ghidraAddress 0x00405ed0
     */
    virtual HxStream &Read(void *pDest, int nSize);

private:
    HxStr mName;
    char *mStart;
    char *mEnd;
    char *mCur;
};
