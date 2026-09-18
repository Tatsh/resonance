#pragma once

#include <vector>

#include "rnd/stream.h"

namespace Rnd {

/** Bytes the constructor reserves before the first write. */
constexpr int kMemStreamReserve = 0x1000;

/**
 * Stream over a buffer the stream owns and grows.
 *
 * `Q23Rnd9MemStream` in the RTTI descriptor at `0x008ef190`, single inheritance from `Rnd::Stream`
 * at offset 0. The object is 0x1c bytes and its vtable is at `0x00826178`. The end and failure
 * states are stored rather than derived, which is what separates this class from `Rnd::BufStream`.
 */
class MemStream : public Stream {
public:
    /**
     * Construct an empty stream with kMemStreamReserve bytes reserved.
     *
     * @ghidraAddress 0x0050f288
     */
    MemStream();

    /** @ghidraAddress 0x0050fca0 */
    virtual ~MemStream();

    /** @ghidraAddress 0x005100c0 */
    virtual Stream &ReadBytes(void *pDest, int nSize);

    /** @ghidraAddress 0x0050f448 */
    virtual Stream &WriteBytes(const void *pSrc, int nSize);

    /** @ghidraAddress 0x0050fd48 */
    virtual Stream &Flush();

    /** @ghidraAddress 0x00510140 */
    virtual Stream &Seek(int nOffset, int nWhence);

    /** @ghidraAddress 0x0050fd50 */
    virtual int Tell();

    /** @ghidraAddress 0x0050fd58 */
    virtual int Eof();

    /** @ghidraAddress 0x0050fd60 */
    virtual int Fail();

private:
    int mEof;                  // +0x04
    int mFail;                 // +0x08
    int mPos;                  // +0x0c
    std::vector<char> mBuffer; // +0x10
};

} // namespace Rnd
