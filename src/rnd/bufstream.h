#pragma once

#include "rnd/stream.h"

namespace Rnd {

/**
 * Stream over a fixed buffer the caller owns.
 *
 * Its RTTI descriptor is at `0x008ef1e0`. It has single inheritance from `Rnd::Stream` at offset 0.
 * The object is 0x14 bytes and its vtable is at `0x008260c0`.
 *
 * The buffer neither grows nor is released here. A transfer that would pass mSize is shortened to
 * the remainder and sets mFail, so an overrun reports itself rather than corrupting memory. Slot
 * 10 of the vtable is null, so a BufStream must not be destroyed through a `Rnd::Stream` pointer.
 */
class BufStream : public Stream {
public:
    /**
     * Construct a stream over nSize bytes at pBuffer, positioned at the start.
     *
     * A null buffer starts the stream failed. RndAsyncLoader::PollAsyncLoads() builds one over
     * each completed read.
     *
     * @param pBuffer The buffer, which the caller retains.
     * @param nSize The buffer size in bytes.
     * @ghidraAddress NTSC-U/C: 0x005104b8
     * @ghidraAddress PAL: 0x0054faa0
     */
    BufStream(char *pBuffer, int nSize);

    /**
     * @ghidraAddress NTSC-U/C: 0x005104e0
     * @ghidraAddress PAL: 0x0054fac8
     */
    virtual Stream &ReadBytes(void *pDest, int nSize);

    /**
     * @ghidraAddress NTSC-U/C: 0x00510558
     * @ghidraAddress PAL: 0x0054fb40
     */
    virtual Stream &WriteBytes(const void *pSrc, int nSize);

    /**
     * @ghidraAddress NTSC-U/C: 0x0050fe68
     * @ghidraAddress PAL: 0x0054f450
     */
    virtual Stream &Flush();

    /**
     * @ghidraAddress NTSC-U/C: 0x005105c8
     * @ghidraAddress PAL: 0x0054fbb0
     */
    virtual Stream &Seek(int nOffset, int nWhence);

    /**
     * @ghidraAddress NTSC-U/C: 0x0050fe70
     * @ghidraAddress PAL: 0x0054f458
     */
    virtual int Tell();

    /**
     * @ghidraAddress NTSC-U/C: 0x0050fe78
     * @ghidraAddress PAL: 0x0054f460
     */
    virtual int Eof();

    /**
     * @ghidraAddress NTSC-U/C: 0x0050fe90
     * @ghidraAddress PAL: 0x0054f478
     */
    virtual int Fail();

private:
    char *mBuffer; // +0x04
    int mFail;     // +0x08
    int mPos;      // +0x0c
    int mSize;     // +0x10
};

} // namespace Rnd
