#pragma once

#include "rnd/stream.h"

namespace Rnd {

/**
 * Stream over a refilling transport.
 *
 * `Q23Rnd10ToolStream` in the RTTI descriptor at `0x008eefe8`, single inheritance from
 * `Rnd::Stream` at offset 0. Its vtable is at `0x00826118`.
 *
 * The stream reads a source that arrives in instalments. ReadBytes() spins on Eof() until data is
 * present, copies what the buffer already holds, then calls Flush() to refill and spins again,
 * repeating until the request is satisfied. Eof() compares mUnknown10 with mUnknown14, and the
 * destructor releases mBuffer.
 *
 * Flush() resets mCursor and copies mArrived into mConsumed, which makes Eof() true again, so the
 * pair is a delivery counter the transport advances and the snapshot of it taken at the last
 * refill. The object is at least 0x18 bytes; nothing past `+0x14` is recovered.
 *
 * The stream is read-only in the shipped build. WriteBytes() reports
 * "Can't write to a PS ToolStream" and stops the machine, and both Tell() and Fail() always
 * report 0.
 */
class ToolStream : public Stream {
public:
    /** @ghidraAddress 0x00510288 */
    virtual ~ToolStream();

    /** @ghidraAddress 0x00510308 */
    virtual Stream &ReadBytes(void *pDest, int nSize);

    /** @ghidraAddress 0x00510400 */
    virtual Stream &WriteBytes(const void *pSrc, int nSize);

    /** @ghidraAddress 0x00510468 */
    virtual Stream &Flush();

    /** @ghidraAddress 0x0050fde0 */
    virtual Stream &Seek(int nOffset, int nWhence);

    /** @ghidraAddress 0x0050fde8 */
    virtual int Tell();

    /** @ghidraAddress 0x005102f0 */
    virtual int Eof();

    /** @ghidraAddress 0x005102e8 */
    virtual int Fail();

private:
    int mCursor;   // +0x04 bytes of mBuffer already handed out
    char *mBuffer; // +0x08
    int mFill;     // +0x0c bytes of mBuffer that are valid
    int mArrived;  // +0x10 advanced by the transport as instalments land
    int mConsumed; // +0x14 mArrived as it stood at the last Flush()
};

} // namespace Rnd
