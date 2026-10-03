#pragma once

#include "rnd/stream.h"

namespace Rnd {

/**
 * Stream over a refilling transport.
 *
 * Its RTTI descriptor is at `0x008eefe8`. It has single inheritance from `Rnd::Stream` at offset 0.
 * Its vtable is at `0x00826118`.
 *
 * The stream reads a source that arrives in instalments. Read() spins on Eof() until data is
 * present, copies what the buffer already holds, then calls Flush() to refill and spins again,
 * repeating until the request is satisfied. Eof() compares mArrived with mConsumed, and the
 * destructor releases mBuffer.
 *
 * Flush() resets mCursor and copies mArrived into mConsumed, which makes Eof() true again. The
 * pair is a delivery counter the transport advances and the snapshot of it taken at the last
 * refill. The constructor's highest store is `+0x14`, so the object is 0x18 bytes.
 *
 * The stream is read-only in the shipped build. Write() reports
 * "Can't write to a PS ToolStream" and stops the machine, and both Tell() and Fail() always
 * report 0.
 */
class ToolStream : public Stream {
public:
    /**
     * Construct an empty stream over a 0x4000-byte buffer from the untagged heap.
     *
     * @ghidraAddress NTSC-U/C: 0x00510238
     * @ghidraAddress PAL: 0x0054f820
     */
    ToolStream();

    /**
     * Report to the debug console where the transport should deliver.
     *
     * Prints "ToolStream connect:" followed by the addresses of mFill, mArrived, and mConsumed
     * and of the buffer, so the host tool can write into them. No call site survives in the
     * shipped program, and the name is inferred from the text.
     *
     * @ghidraAddress NTSC-U/C: 0x00510480
     * @ghidraAddress PAL: 0x0054fa68
     */
    void Connect();

    /**
     * @ghidraAddress NTSC-U/C: 0x00510288
     * @ghidraAddress PAL: 0x0054f870
     */
    virtual ~ToolStream();

    /**
     * @ghidraAddress NTSC-U/C: 0x00510308
     * @ghidraAddress PAL: 0x0054f8f0
     */
    virtual Stream &Read(void *pDest, int nSize);

    /**
     * @ghidraAddress NTSC-U/C: 0x00510400
     * @ghidraAddress PAL: 0x0054f9e8
     */
    virtual Stream &Write(const void *pSrc, int nSize);

    /**
     * @ghidraAddress NTSC-U/C: 0x00510468
     * @ghidraAddress PAL: 0x0054fa50
     */
    virtual Stream &Flush();

    /**
     * @ghidraAddress NTSC-U/C: 0x0050fde0
     * @ghidraAddress PAL: 0x0054f3c8
     */
    virtual Stream &Seek(int nOffset, int nWhence);

    /**
     * @ghidraAddress NTSC-U/C: 0x0050fde8
     * @ghidraAddress PAL: 0x0054f3d0
     */
    virtual int Tell();

    /**
     * @ghidraAddress NTSC-U/C: 0x005102f0
     * @ghidraAddress PAL: 0x0054f8d8
     */
    virtual int Eof();

    /**
     * @ghidraAddress NTSC-U/C: 0x005102e8
     * @ghidraAddress PAL: 0x0054f8d0
     */
    virtual int Fail();

private:
    int mCursor;   // +0x04 bytes of mBuffer already handed out
    char *mBuffer; // +0x08
    int mFill;     // +0x0c bytes of mBuffer that are valid
    int mArrived;  // +0x10 advanced by the transport as instalments land
    int mConsumed; // +0x14 mArrived as it stood at the last Flush()
};

} // namespace Rnd
