#pragma once

#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * Bidirectional stream over a caller-supplied byte buffer.
 *
 * `20IOBPreallocMemStream` in the RTTI descriptor at `0x008ef160`. The base list at `0x008242c8`
 * records two bases, IBStream at offset 0 and OBStream at offset 4, both public and non-virtual.
 * IOBStream shares that base list and is not in the chain, so this class is a sibling of
 * IOBStream rather than a subclass, which the disassembly of the type function at `0x004eda70`
 * confirms.
 *
 * The object is 0x20 bytes. Two vtable pointers occupy `+0x00` and `+0x04`, addressing
 * `0x00824110` for the IBStream subobject and `0x008240e0` for the OBStream subobject, whose
 * entries adjust `this` by `-0x04`. The six data members follow at `+0x08` through `+0x1f`.
 *
 * Nothing is allocated. The constructor records the buffer and its capacity, and the destructor
 * releases nothing, so the caller owns the storage for the life of the stream. Reads consume up
 * to the write position rather than up to the capacity, which makes the stream a first-in
 * first-out buffer with a separate read cursor.
 *
 * The two halves report failure differently, and the difference is in the binary rather than in
 * this reconstruction. A short read is clamped and sets both the end flag and the failure flag,
 * while a write that would pass the capacity transfers nothing and sets only the failure flag.
 *
 * Twelve call sites construct one. Globals::InitServices() allocates a stream over the 0x19000
 * byte log buffer at `0x0086f7d0`, and eleven memory card task constructors build one over the
 * 0xf000 byte buffer at `0x008f2aa0`. The memory card tasks embed the stream as a member and
 * destroy it inline.
 */
class IOBPreallocMemStream : public IBStream, public OBStream {
public:
    /**
     * Open a stream over a caller-owned buffer.
     *
     * @param pBuffer The buffer to read and write. The stream does not take ownership.
     * @param nCapacity The buffer size in bytes.
     * @ghidraAddress 0x004ee2f8
     */
    IOBPreallocMemStream(char *pBuffer, int nCapacity);

    /**
     * Close the stream.
     *
     * IBStream vtable slot 8. The buffer is not released.
     *
     * @ghidraAddress 0x004edb00
     */
    virtual ~IOBPreallocMemStream();

    /** @ghidraAddress 0x004ee330 */
    virtual IBStream &ReadBytes(void *pDest, int nSize);

    /** @ghidraAddress 0x004ee3a8 */
    virtual IBStream &Seek(int nOffset, int nWhence);

    /** @ghidraAddress 0x004edb48 */
    virtual int Tell();

    /** @ghidraAddress 0x004edb50 */
    virtual int Eof();

    /**
     * Test whether a transfer has failed.
     *
     * One body serves IBStream vtable slot 5 and OBStream vtable slot 3, which is what the two
     * tables record: both entries address the same routine, with the second adjusting `this` by
     * `-0x04`.
     *
     * @return Non-zero once a transfer has failed.
     * @ghidraAddress 0x004edb58
     */
    virtual int Fail();

    /** @ghidraAddress 0x004ee420 */
    virtual IBStream &Flush();

    /**
     * Record how many bytes a reader may consume.
     *
     * IBStream vtable slot 9, the first virtual this class adds. The write position is overwritten
     * with no bounds test against the capacity.
     *
     * @param nSize The new write position.
     * @ghidraAddress 0x004edb40
     */
    virtual void SetSize(int nSize);

    /**
     * Report the caller-owned buffer.
     *
     * IBStream vtable slot 10, the second virtual this class adds. GrooveWorld::FinishSong()
     * dispatches it at `0x0018e91c` to write the log's length back into its first word. Ghidra
     * reports no reference beyond the vtable entry, because the one call is through the table.
     *
     * @return The buffer passed to the constructor.
     * @ghidraAddress 0x004edb60
     */
    virtual char *Buffer();

    /**
     * Report how many bytes have been written.
     *
     * Not virtual, and inline. The out-of-line copy at the address below has no call site.
     * GrooveWorld::FinishSong() at `0x0018e90c` expands it.
     *
     * @return The write position.
     * @ghidraAddress 0x004edb68
     */
    int Size();

    /**
     * Report the buffer size passed to the constructor.
     *
     * Not virtual, and inlined at every call site in the same way as Size().
     *
     * @return The capacity in bytes.
     * @ghidraAddress 0x004edb70
     */
    int Capacity();

    /** @ghidraAddress 0x004ee428 */
    virtual OBStream &WriteBytes(const void *pSrc, int nSize);

    /** @ghidraAddress 0x004ee498 */
    virtual OBStream &Reset();

    /**
     * Caller-owned byte buffer, which the constructor records and the destructor does not release.
     *
     * Public because three memory card task constructors load it directly out of the embedded
     * stream immediately after constructing it, at `0x0017e5c8` in ListRemixesMCT, `0x0017bed0` in
     * LoadRemixMCT, and `0x0017cee4` in DeleteRemixMCT. Each one is a plain word load at offset 8
     * with no dispatch through Buffer(), and none of the three derives from this class.
     *
     * Two further readings fit the image equally well. A friend declaration per task class gives
     * the same instructions. And a devirtualised inline `Buffer()` call would also compile to the
     * one load, which is why the promotion is recorded as an inference rather than as a
     * measurement.
     *
     * +0x08
     */
    char *mBuffer;

private:
    int mCapacity;
    int mWritePos;
    int mReadPos;
    int mEof;
    int mFail;
};

// 0x004edb68, the out-of-line copy.
inline int IOBPreallocMemStream::Size() {
    return mWritePos;
}
