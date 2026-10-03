#pragma once

#include <vector>

#include "stream/iobstream.h"

class IBStream;
class OBStream;

/**
 * Bidirectional stream over a growing byte buffer.
 *
 * Its RTTI descriptor is at `0x008f00d0`. It has single inheritance from IOBStream at offset 0. The
 * type function is at `0x004ed780` and builds IOBStream's descriptor inline first.
 *
 * The object is 0x20 bytes. Two vtable pointers occupy `+0x00` and `+0x04`, addressing
 * `0x008241a0` for the IBStream subobject and `0x00824170` for the OBStream subobject, whose
 * entries adjust `this` by `-0x04`. The end flag, the failure flag, and the cursor follow at
 * `+0x08`, `+0x0c`, and `+0x10`, and the buffer is a `std::vector<char>` whose three pointers
 * occupy `+0x14`, `+0x18`, and `+0x1c`.
 *
 * One cursor serves both halves, so a write advances the position a read then continues from.
 * That is the difference from IOBPreallocMemStream, which has two positions and a fixed buffer.
 *
 * SetSize() through Buffer() are the three slots the primary vtable adds beyond IBStream's eight.
 * They are declared here rather than on IOBStream, and iobstream.h records why that split cannot
 * be settled.
 */
class IOBMemStream : public IOBStream {
public:
    /**
     * Open an empty stream.
     *
     * The buffer reserves one growth step up front.
     *
     * @ghidraAddress NTSC-U/C: 0x004ecca8
     * @ghidraAddress PAL: 0x0052b850
     */
    IOBMemStream();

    /**
     * Open a stream and write an initial block into it.
     *
     * The cursor is left after the block, so a reader has to seek back to the start. No call site
     * remains in the image.
     *
     * @param pData The bytes to write.
     * @param nSize The number of bytes to write.
     * @ghidraAddress NTSC-U/C: 0x004ece70
     * @ghidraAddress PAL: 0x0052ba18
     */
    IOBMemStream(const void *pData, int nSize);

    /**
     * Close the stream and release the buffer.
     *
     * IBStream vtable slot 8.
     *
     * @ghidraAddress NTSC-U/C: 0x004ed978
     * @ghidraAddress PAL: 0x0052c520
     */
    virtual ~IOBMemStream();

    /**
     * @ghidraAddress NTSC-U/C: 0x004ee0e8
     * @ghidraAddress PAL: 0x0052cc90
     */
    virtual IBStream &Read(void *pDest, int nSize);

    /**
     * @ghidraAddress NTSC-U/C: 0x004ee168
     * @ghidraAddress PAL: 0x0052cd10
     */
    virtual IBStream &Seek(int nOffset, int nWhence);

    /**
     * @ghidraAddress NTSC-U/C: 0x004eda38
     * @ghidraAddress PAL: 0x0052c5e0
     */
    virtual int Tell();

    /**
     * @ghidraAddress NTSC-U/C: 0x004eda40
     * @ghidraAddress PAL: 0x0052c5e8
     */
    virtual int Eof();

    /**
     * Test whether a transfer has failed.
     *
     * One body serves IBStream vtable slot 5 and OBStream vtable slot 3.
     *
     * @return Non-zero once a transfer has failed.
     * @ghidraAddress NTSC-U/C: 0x004eda48
     * @ghidraAddress PAL: 0x0052c5f0
     */
    virtual int Fail();

    /**
     * @ghidraAddress NTSC-U/C: 0x004eda50
     * @ghidraAddress PAL: 0x0052c5f8
     */
    virtual IBStream &Flush();

    /**
     * Grow the buffer by nSize bytes and write nSize bytes at the cursor.
     *
     * IBStream vtable slot 9. The cursor is not advanced.
     *
     * @param pSrc The bytes to write.
     * @param nSize The number of bytes to write.
     * @ghidraAddress NTSC-U/C: 0x004ee018
     * @ghidraAddress PAL: 0x0052cbc0
     */
    virtual void Fill(const void *pSrc, int nSize);

    /**
     * Resize the buffer and rewind the cursor.
     *
     * IBStream vtable slot 10. Growth is zero filled. The title is inferred from the shape, and no
     * call site remains in the image.
     *
     * @param nSize The new buffer size in bytes.
     * @ghidraAddress NTSC-U/C: 0x004ee260
     * @ghidraAddress PAL: 0x0052ce08
     */
    virtual void Resize(int nSize);

    /**
     * Report the buffer.
     *
     * IBStream vtable slot 11. The pointer is null while the buffer has never been allocated.
     *
     * @return The first byte of the buffer.
     * @ghidraAddress NTSC-U/C: 0x004eda58
     * @ghidraAddress PAL: 0x0052c600
     */
    virtual char *Buffer();

    /**
     * Drop the bytes already read from the front of the buffer and rewind the cursor.
     *
     * The unread tail moves to the start of the buffer, and the buffer shrinks by the old cursor
     * position. The routine is absent from the vtable, and no call site survives in the shipped
     * program.
     *
     * @ghidraAddress NTSC-U/C: 0x004ee1f0
     * @ghidraAddress PAL: 0x0052cd98
     */
    void Compact();

    /**
     * @ghidraAddress NTSC-U/C: 0x004ed068
     * @ghidraAddress PAL: 0x0052bc10
     */
    virtual OBStream &Write(const void *pSrc, int nSize);

    /**
     * @ghidraAddress NTSC-U/C: 0x004eda30
     * @ghidraAddress PAL: 0x0052c5d8
     */
    virtual OBStream &Reset();

private:
    int mEof;
    int mFail;
    int mPos;
    std::vector<char> mBuffer;
};
