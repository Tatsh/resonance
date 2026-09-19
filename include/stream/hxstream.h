#pragma once

/**
 * Origin an HxStream::Seek() offset is measured from.
 *
 * HxMemStream::Seek() treats every origin other than 1 and 2 as the start rather than rejecting
 * it, so the three values below are the whole set. The titles are inferred, and the enumeration is
 * separate from StreamSeekOrigin because the two class families are unrelated.
 */
enum HxStreamSeekOrigin {
    kHxSeekSet = 0, /*!< Measure from the start of the data. */
    kHxSeekCur = 1, /*!< Measure from the read position. */
    kHxSeekEnd = 2  /*!< Measure from the end of the data. */
};

/**
 * Seekable data source interface, unrelated to IBStream and OBStream.
 *
 * `8HxStream` in the RTTI descriptor at `0x0086f6c8`, with no base. The class has three data words
 * and stores its vtable pointer after them at `+0x0c`, which is where a class with no base puts
 * it, so the object is 0x10 bytes. Its vtable is at `0x00816d60`, emitted once per translation
 * unit that needs it, and the seven slots below are the whole interface.
 *
 * Every slot has a body here, and each does nothing beyond producing a neutral result, so the
 * class is an interface with defaults rather than an abstract one. Two subclasses are attested,
 * HxMemStream and HxIDataChunk. HxIDataChunk is a RIFF chunk reader and belongs to a different
 * subsystem.
 *
 * Nothing in the image writes mFatalOnEnd, so the diagnostic HxMemStream::Read() guards on it is
 * unreachable in the shipped build.
 */
class HxStream {
public:
    /**
     * Close the stream.
     *
     * Vtable slot 1, which is the first of the interface, because the destructor is declared ahead
     * of the transfer virtuals.
     *
     * @ghidraAddress 0x00145ee8
     */
    virtual ~HxStream();

    /**
     * Move the read position.
     *
     * Vtable slot 2. The return type is void rather than a stream reference, which the two
     * instruction body here and HxMemStream's override both establish.
     *
     * @param nOffset The signed distance to move.
     * @param nWhence The origin, one of the HxStreamSeekOrigin values.
     * @ghidraAddress 0x00145f18
     */
    virtual void Seek(int nOffset, int nWhence);

    /**
     * Report the read position.
     *
     * Vtable slot 3. The default reports 0.
     *
     * @return The position in bytes from the start.
     * @ghidraAddress 0x00145f20
     */
    virtual int Tell();

    /**
     * Report the length of the data.
     *
     * Vtable slot 4. The default reports 0.
     *
     * @return The length in bytes.
     * @ghidraAddress 0x00145f28
     */
    virtual int Size();

    /**
     * Move nSize bytes out of pSrc.
     *
     * Vtable slot 5. The default discards the request.
     *
     * @param pSrc The source buffer.
     * @param nSize The number of bytes to move.
     * @return This stream.
     * @ghidraAddress 0x00145f30
     */
    virtual HxStream &Write(const void *pSrc, int nSize);

    /**
     * Move nSize bytes into pDest.
     *
     * Vtable slot 6. The default discards the request.
     *
     * @param pDest The destination buffer.
     * @param nSize The number of bytes to move.
     * @return This stream.
     * @ghidraAddress 0x00145f38
     */
    virtual HxStream &Read(void *pDest, int nSize);

    /**
     * Report an integer whose meaning is undetermined.
     *
     * Vtable slot 7. The default reports 0 and HxMemStream retains it. HxIDataChunk overrides it
     * at `0x00145fd8` and reports a stored word, which is the only evidence of what the slot is
     * for, and it is not enough to title it.
     *
     * @return The stored value, or 0 by default.
     * @ghidraAddress 0x00145f40
     */
    virtual int Unknown7();

protected:
    /**
     * Construct a stream with a zeroed state.
     *
     * @ghidraAddress 0x004057a8
     */
    HxStream();

    int mUnknown00; // +0x00, zeroed by the constructor and read nowhere in the image
    // Status word. HxMemStream stores 1 once the read position arrives at the end of the data and
    // 0 when a seek clamps to the start.
    int mStatus;
    // When non-zero, HxMemStream::Read() reports a fatal error rather than a short read. Nothing
    // in the image sets it.
    int mFatalOnEnd;
};
