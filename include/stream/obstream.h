#pragma once

/**
 * Output half of the byte stream interface.
 *
 * `8OBStream` in the RTTI descriptor at `0x0086f5e8`, with no base. The class has no data member,
 * so the object is four bytes of vtable pointer, and every field belongs to a subclass. The
 * descriptor emits no standalone accessor, because each derived accessor builds it inline before
 * its own.
 *
 * Three implementations are attested. OBFileStream derives from this class alone, while
 * IOBStream and IOBPreallocMemStream derive from IBStream at offset 0 and from this class at
 * offset 4. The four slots below are the whole interface. Two independent measurements agree on
 * the count: OBFileStream's table runs to slot 6 with its destructor at slot 5 and a new Tell()
 * at slot 6, and the secondary tables of IOBMemStream and IOBPreallocMemStream both end after
 * slot 4. There is therefore no virtual destructor on this side of a stream, and a bidirectional
 * stream is destroyed through its IBStream subobject.
 *
 * Write() is the one slot with a shared base body. Both implementations of it retain the body, and
 * it does nothing beyond dispatching WriteBytes(), so the two entry points are interchangeable on
 * this target.
 *
 * Fail() is declared with the same signature on IBStream. A class deriving from both therefore
 * overrides the two with one body, which is what IOBPreallocMemStream and IOBStream do.
 */
class OBStream {
public:
    /**
     * Move nSize bytes out of pSrc.
     *
     * Vtable slot 1.
     *
     * @param pSrc The source buffer.
     * @param nSize The number of bytes to move.
     * @return This stream.
     */
    virtual OBStream &WriteBytes(const void *pSrc, int nSize) = 0;

    /**
     * Discard everything written so far.
     *
     * Vtable slot 2. OBFileStream and IOBMemStream return the stream with no effect, and only
     * IOBPreallocMemStream does work, where it zeroes both positions and both flags. The title is
     * inferred from that one implementation. Flush() was rejected, because the file
     * implementation would then be the one that does nothing while Rnd::FileStream::Flush()
     * forwards to `fflush`.
     *
     * @return This stream.
     */
    virtual OBStream &Reset() = 0;

    /**
     * Test whether a transfer has failed.
     *
     * Vtable slot 3.
     *
     * @return Non-zero once a transfer has failed.
     */
    virtual int Fail() = 0;

    /**
     * Move nSize bytes out of pSrc through the virtual WriteBytes().
     *
     * Vtable slot 4. One shared body serves both implementations and neither overrides it.
     *
     * @param pSrc The source buffer.
     * @param nSize The number of bytes to move.
     * @return This stream.
     * @ghidraAddress 0x004ed860
     */
    virtual OBStream &Write(const void *pSrc, int nSize);
};

/**
 * Write a truth value as one byte.
 *
 * The routine stores the low byte of its argument to the stack and moves a single byte through
 * WriteBytes(). Its counterpart reads that byte back and stores a four-byte word, so the value is
 * one byte on the wire and four bytes in memory. Whether the original declared the parameter as an
 * int or as a bool of the four-byte width some builds of this compiler used cannot be settled from
 * the transfer alone, and int is written here to match the width of the store.
 *
 * @param stream The stream to write to.
 * @param bValue The value, of which only the low byte reaches the stream.
 * @return The stream.
 * @ghidraAddress 0x004edc80
 */
OBStream &operator<<(OBStream &stream, int bValue);
