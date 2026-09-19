#pragma once

/**
 * Origin an IBStream::Seek() offset is measured from.
 *
 * The three values agree with the C library, and IBFileStream::Seek() still indexes a local table
 * to translate them. The enumeration titles are inferred, since the original spelling does not
 * survive in the image.
 */
enum StreamSeekOrigin {
    kStreamSeekSet = 0, /*!< Measure from the start of the data. */
    kStreamSeekCur = 1, /*!< Measure from the read position. */
    kStreamSeekEnd = 2  /*!< Measure from the end of the data. */
};

/**
 * Input half of the byte stream interface.
 *
 * `8IBStream` in the RTTI descriptor at `0x0086f5e0`, with no base. The class has no data member,
 * so the object is four bytes of vtable pointer, and every field belongs to a subclass. The
 * descriptor emits no standalone accessor, because each derived accessor builds it inline before
 * its own.
 *
 * Three implementations are attested. IBFileStream derives from this class alone, while
 * IOBStream and IOBPreallocMemStream derive from this class at offset 0 and from OBStream at
 * offset 4. Every vtable in the family places these eight slots first, in the declaration order
 * below, so the recovery is complete rather than partial. IBFileStream's table ends at slot 8,
 * which is what fixes the count.
 *
 * Read() is the one slot with a shared base body. All three implementations retain it, and it
 * does nothing beyond dispatching ReadBytes(), so the two entry points are interchangeable on
 * this target.
 *
 * Fail() is declared with the same signature on OBStream. A class deriving from both therefore
 * overrides the two with one body, which is what IOBPreallocMemStream and IOBStream do.
 */
class IBStream {
public:
    /**
     * Move nSize bytes into pDest.
     *
     * Vtable slot 1. A short transfer is clamped rather than reported through the return value.
     *
     * @param pDest The destination buffer.
     * @param nSize The number of bytes to move.
     * @return This stream.
     */
    virtual IBStream &ReadBytes(void *pDest, int nSize) = 0;

    /**
     * Move the read position.
     *
     * Vtable slot 2. An origin outside 0 to 2 returns with no change.
     *
     * @param nOffset The signed distance to move.
     * @param nWhence The origin, one of the StreamSeekOrigin values.
     * @return This stream.
     */
    virtual IBStream &Seek(int nOffset, int nWhence) = 0;

    /**
     * Report the read position.
     *
     * Vtable slot 3.
     *
     * @return The position in bytes from the start.
     */
    virtual int Tell() = 0;

    /**
     * Test whether the read position has arrived at the end of the data.
     *
     * Vtable slot 4.
     *
     * @return Non-zero at the end.
     */
    virtual int Eof() = 0;

    /**
     * Test whether a transfer has failed.
     *
     * Vtable slot 5.
     *
     * @return Non-zero once a transfer has failed.
     */
    virtual int Fail() = 0;

    /**
     * Move nSize bytes into pDest through the virtual ReadBytes().
     *
     * Vtable slot 6. One shared body serves all three implementations and none overrides it.
     *
     * @param pDest The destination buffer.
     * @param nSize The number of bytes to move.
     * @return This stream.
     * @ghidraAddress 0x004ed838
     */
    virtual IBStream &Read(void *pDest, int nSize);

    /**
     * Service the underlying transport.
     *
     * Vtable slot 7. Every implementation returns the stream with no effect, and each supplies its
     * own trivial body rather than sharing one, which is why the slot is pure here. The title is
     * inferred from the position and from the shape. OBStream declares a separate slot with the
     * same shape, and the two cannot share a title, because the return types differ.
     *
     * @return This stream.
     */
    virtual IBStream &Flush() = 0;

    /**
     * Close the stream.
     *
     * Vtable slot 8, which is the last of the interface rather than the first, because the
     * destructor is declared after the transfer virtuals. Every implementation supplies a body and
     * none calls a base body, so the body here is empty and was inlined.
     */
    virtual ~IBStream() {
    }
};
