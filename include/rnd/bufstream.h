#pragma once

#include "rnd/stream.h"

namespace Rnd {

/**
 * Stream over a fixed buffer the caller owns.
 *
 * `Q23Rnd9BufStream` in the RTTI descriptor at `0x008ef1e0`, single inheritance from `Rnd::Stream`
 * at offset 0. The object is 0x14 bytes and its vtable is at `0x008260c0`.
 *
 * The buffer neither grows nor is released here. A transfer that would pass mSize is shortened to
 * the remainder and sets mFail, so an overrun reports itself rather than corrupting memory. Slot
 * 10 of the vtable is null, so a BufStream must not be destroyed through a `Rnd::Stream` pointer.
 */
class BufStream : public Stream {
public:
    /** @ghidraAddress 0x005104e0 */
    virtual Stream &ReadBytes(void *pDest, int nSize);

    /** @ghidraAddress 0x00510558 */
    virtual Stream &WriteBytes(const void *pSrc, int nSize);

    /** @ghidraAddress 0x0050fe68 */
    virtual Stream &Flush();

    /** @ghidraAddress 0x005105c8 */
    virtual Stream &Seek(int nOffset, int nWhence);

    /** @ghidraAddress 0x0050fe70 */
    virtual int Tell();

    /** @ghidraAddress 0x0050fe78 */
    virtual int Eof();

    /** @ghidraAddress 0x0050fe90 */
    virtual int Fail();

private:
    char *mBuffer; // +0x04
    int mFail;     // +0x08
    int mPos;      // +0x0c
    int mSize;     // +0x10
};

} // namespace Rnd
