#pragma once

#include <vector>

#include "rnd/stream.h"

namespace Rnd {

/** Bytes the constructor reserves before the first write. */
constexpr int kMemStreamReserve = 0x1000;

/**
 * Stream over a buffer the stream owns and grows.
 *
 * Its RTTI descriptor is at `0x008ef190`. It has single inheritance from `Rnd::Stream` at offset 0.
 * The object is 0x1c bytes and its vtable is at `0x00826178`. The end and failure states are stored
 * rather than derived. Storing them separates this class from `Rnd::BufStream`.
 */
class MemStream : public Stream {
public:
    /**
     * Construct an empty stream with kMemStreamReserve bytes reserved.
     *
     * @ghidraAddress NTSC-U/C: 0x0050f288
     * @ghidraAddress PAL: 0x0054e870
     */
    MemStream();

    /**
     * @ghidraAddress NTSC-U/C: 0x0050fca0
     * @ghidraAddress PAL: 0x0054f288
     */
    virtual ~MemStream();

    /**
     * @ghidraAddress NTSC-U/C: 0x005100c0
     * @ghidraAddress PAL: 0x0054f6a8
     */
    virtual Stream &Read(void *pDest, int nSize);

    /**
     * @ghidraAddress NTSC-U/C: 0x0050f448
     * @ghidraAddress PAL: 0x0054ea30
     */
    virtual Stream &Write(const void *pSrc, int nSize);

    /**
     * @ghidraAddress NTSC-U/C: 0x0050fd48
     * @ghidraAddress PAL: 0x0054f330
     */
    virtual Stream &Flush();

    /**
     * @ghidraAddress NTSC-U/C: 0x00510140
     * @ghidraAddress PAL: 0x0054f728
     */
    virtual Stream &Seek(int nOffset, int nWhence);

    /**
     * @ghidraAddress NTSC-U/C: 0x0050fd50
     * @ghidraAddress PAL: 0x0054f338
     */
    virtual int Tell();

    /**
     * @ghidraAddress NTSC-U/C: 0x0050fd58
     * @ghidraAddress PAL: 0x0054f340
     */
    virtual int Eof();

    /**
     * @ghidraAddress NTSC-U/C: 0x0050fd60
     * @ghidraAddress PAL: 0x0054f348
     */
    virtual int Fail();

    /**
     * Drop the bytes already read from the front of the buffer and rewind the cursor.
     *
     * The unread tail moves to the start of the buffer, and the buffer shrinks by the old cursor
     * position. No call site survives in the shipped program.
     *
     * @ghidraAddress NTSC-U/C: 0x005101c8
     * @ghidraAddress PAL: 0x0054f7b0
     */
    void Compact();

private:
    int mEof;                  // +0x04
    int mFail;                 // +0x08
    int mPos;                  // +0x0c
    std::vector<char> mBuffer; // +0x10
};

} // namespace Rnd
