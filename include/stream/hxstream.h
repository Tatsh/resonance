#pragma once

class HxStr;

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
 * Only HxIDataChunk's two constructors write mFatalOnEnd, each setting it on the chunk itself. No
 * HxMemStream ever has it set, so the diagnostic HxMemStream::Read() guards on it is unreachable
 * in the shipped build.
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
     * Report the stream this one reads through, if any.
     *
     * Vtable slot 7. The default reports null and HxMemStream retains it. HxIDataChunk overrides
     * it at `0x00145fd8` and reports the stream its chunk lies in. That override types the result
     * but is not enough to title the slot.
     *
     * @return The underlying stream, or null by default.
     * @ghidraAddress 0x00145f40
     */
    virtual HxStream *Unknown7();

    /**
     * Move nSize bytes into pDest, reversing their order when the stream swaps bytes.
     *
     * With mSwapBytes clear, or for a single byte, this is one Read(). Otherwise it reads one byte
     * at a time from the last position of pDest to the first. Mid::FileReader reads every header
     * and track field through it, and HxDataChunkId reads chunk sizes through it. The title is
     * inferred.
     *
     * @param pDest The destination buffer.
     * @param nSize The number of bytes to move.
     * @return This stream.
     * @ghidraAddress 0x004059f8
     */
    HxStream &ReadSwapped(void *pDest, int nSize);

    /**
     * Move nSize bytes out of pSrc, reversing their order when the stream swaps bytes.
     *
     * The writing counterpart of ReadSwapped(). With mSwapBytes clear, or for a single byte, this
     * is one Write(). Otherwise it writes one byte at a time from the last position of pSrc to the
     * first. The shipped program calls it only from WriteVarLen(). The title is inferred.
     *
     * @param pSrc The source buffer.
     * @param nSize The number of bytes to move.
     * @return This stream.
     * @ghidraAddress 0x00405958
     */
    HxStream &WriteSwapped(const void *pSrc, int nSize);

    /**
     * Read a string with a variable-length size prefix into a fixed buffer.
     *
     * The length comes from ReadVarLen(). A string shorter than nDestSize is read whole and
     * terminated. A longer one is cut to `nDestSize - 1` bytes and terminated, and the read
     * position then skips the rest of the string. The shipped program does not call it. The title
     * is inferred.
     *
     * @param pszDest The destination buffer.
     * @param nDestSize The size of pszDest in bytes, terminator included.
     * @return This stream.
     * @ghidraAddress 0x004057c8
     */
    HxStream &ReadString(char *pszDest, int nDestSize);

    /**
     * Read a string with a variable-length size prefix into an HxStr.
     *
     * The bytes are read into a temporary untagged block one byte longer than the string, which
     * is terminated, assigned to str, and released. The shipped program does not call it. The
     * title is inferred.
     *
     * @param str Receives the string.
     * @return This stream.
     * @ghidraAddress 0x004058a8
     */
    HxStream &ReadString(HxStr &str);

    /**
     * Follow Unknown7() from this stream until a stream reports none, and report that stream.
     *
     * The shipped program does not call it. The title is inferred.
     *
     * @return The innermost stream, which is this stream when it reads through no other.
     * @ghidraAddress 0x00405a98
     */
    HxStream *BaseStream();

    /**
     * Status value of a stream with nothing wrong.
     *
     * HxIDataChunk loads the four status values from memory rather than as immediates. That places
     * them as class constants defined in this class's translation unit. HxMemStream writes the same
     * two values, 0 and 1, as immediates. The names of all four are inferred.
     *
     * @ghidraAddress 0x00816d40
     */
    static const int kStatusOk;

    /**
     * Status value of a stream whose read position arrived at the end of its data.
     *
     * @ghidraAddress 0x00816d44
     */
    static const int kStatusEnd;

    /**
     * Status bit a seek outside a chunk's bounds would set. Tell() reports -1 while it is set.
     *
     * @ghidraAddress 0x00816d48
     */
    static const int kStatusRange;

    /**
     * Second status bit Tell() and Seek() of HxIDataChunk test. No writer is recovered.
     *
     * @ghidraAddress 0x00816d4c
     */
    static const int kStatusFailed;

    /**
     * Non-zero when multi-byte values on the stream are in the opposite byte order.
     *
     * ReadSwapped() tests it and the constructor clears it. Public because HxIDataChunk copies it
     * from another stream object and the image exposes no accessor.
     *
     * +0x00
     */
    int mSwapBytes;

protected:
    /**
     * Construct a stream with a zeroed state.
     *
     * @ghidraAddress 0x004057a8
     */
    HxStream();

    // Status word. HxMemStream stores 1 once the read position arrives at the end of the data and
    // 0 when a seek clamps to the start.
    int mStatus;
    // When non-zero, HxMemStream::Read() reports a fatal error rather than a short read. Only
    // HxIDataChunk's constructors set it.
    int mFatalOnEnd;
};

/**
 * Read a variable-length quantity, seven bits per byte, most significant group first.
 *
 * The value is cleared and each byte read through HxStream::ReadSwapped() adds its low seven bits
 * after a seven-bit shift, until a byte with its top bit clear ends the quantity. That is the
 * Standard MIDI File encoding. Mid::FileReader reads delta times and meta lengths through it, and
 * the two string readers in HxStream's translation unit read their length prefixes through it.
 * The title is inferred.
 *
 * @param nValue Receives the quantity.
 * @param stream The stream to read from.
 * @return The stream.
 * @ghidraAddress 0x00405b70
 */
HxStream &ReadVarLen(int &nValue, HxStream &stream);

/**
 * Write a variable-length quantity, seven bits per byte, most significant group first.
 *
 * The encoding ReadVarLen() reads. The groups are packed into one word, lowest group in the lowest
 * byte and every higher group marked with the continuation bit, and the word's bytes are written
 * lowest first through HxStream::WriteSwapped(). The value is shifted arithmetically, so a
 * negative value never terminates. The shipped program does not call it. The title is inferred.
 *
 * @param nValue The quantity.
 * @param stream The stream to write to.
 * @return The stream.
 * @ghidraAddress 0x00405ad8
 */
HxStream &WriteVarLen(const int &nValue, HxStream &stream);
