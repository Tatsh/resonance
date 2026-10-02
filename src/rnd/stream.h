#pragma once

#include "os/failsink.h"

class HxStr;

namespace Rnd {

/** Origin a Stream::Seek() offset is measured from. The values pass straight to `fseek`. */
enum SeekOrigin { kSeekSet = 0, kSeekCur = 1, kSeekEnd = 2 };

/**
 * Byte stream that every renderer object serialises itself through.
 *
 * `Q23Rnd6Stream` in the RTTI descriptor at `0x0086f670`, a leaf class with no base and no data
 * member. A stream stores its vtable pointer at offset 0 and every field belongs to a subclass.
 * The class emits no standalone `GetTypeInfo` accessor, because each subclass accessor initialises
 * this descriptor inline before its own; the base list in `rtti.json` attributes
 * `RndBufStream__GetTypeInfo` to it, which is an extraction artefact rather than a real accessor.
 *
 * The four subclasses attested by RTTI are `Rnd::FileStream`, `Rnd::MemStream`, `Rnd::BufStream`,
 * and `Rnd::ToolStream`, all single inheritance at offset 0. Every vtable in the family is eleven
 * entries, and the declaration order below reproduces that order, so the recovery is complete
 * rather than partial.
 *
 * ReadBytes() through Fail() are pure, since all four subclasses supply a distinct body for each.
 * Read() and Write() are the exception: one shared base body appears at slot 8 and slot 9 of all
 * four tables and does nothing beyond forwarding to ReadBytes() and WriteBytes(). Both pairs are
 * therefore interchangeable on this target, and a caller that wants byte-order correction for a
 * multi-byte scalar still uses the second pair because the file format is big-endian like the EE.
 */
class Stream {
public:
    /**
     * Move nSize bytes into pDest with no conversion.
     *
     * Vtable slot 1.
     *
     * @param pDest The destination buffer.
     * @param nSize The number of bytes to move.
     * @return This stream.
     */
    virtual Stream &ReadBytes(void *pDest, int nSize) = 0;

    /**
     * Move nSize bytes out of pSrc with no conversion.
     *
     * Vtable slot 2.
     *
     * @param pSrc The source buffer.
     * @param nSize The number of bytes to move.
     * @return This stream.
     */
    virtual Stream &WriteBytes(const void *pSrc, int nSize) = 0;

    /**
     * Service the underlying transport.
     *
     * Vtable slot 3. `Rnd::FileStream` passes the request to its `FILE`, `Rnd::MemStream` and
     * `Rnd::BufStream` do nothing, and `Rnd::ToolStream::ReadBytes()` invokes this whenever it has
     * drained its buffer, which makes it the refill point for a streamed source.
     *
     * @return This stream.
     */
    virtual Stream &Flush() = 0;

    /**
     * Move the read and write position.
     *
     * Vtable slot 4. An offset that would pass either end is clamped rather than reported.
     *
     * @param nOffset The signed distance to move.
     * @param nWhence The origin, one of the SeekOrigin values.
     * @return This stream.
     */
    virtual Stream &Seek(int nOffset, int nWhence) = 0;

    /**
     * Report the current position.
     *
     * Vtable slot 5. `Rnd::ToolStream` always reports 0.
     *
     * @return The position in bytes from the start.
     */
    virtual int Tell() = 0;

    /**
     * Test whether the position has arrived at the end.
     *
     * Vtable slot 6.
     *
     * @return Non-zero at the end.
     */
    virtual int Eof() = 0;

    /**
     * Test whether a transfer has failed.
     *
     * Vtable slot 7. A short read sets this on the memory streams. `Rnd::ToolStream` never fails.
     *
     * @return Non-zero once a transfer has failed.
     */
    virtual int Fail() = 0;

    /**
     * Move nSize bytes into pDest, correcting the byte order for the host.
     *
     * Vtable slot 8. The one shared implementation forwards to ReadBytes(), and no subclass
     * overrides it.
     *
     * @param pDest The destination buffer.
     * @param nSize The number of bytes to move.
     * @return This stream.
     * @ghidraAddress 0x0050fb60
     */
    virtual Stream &Read(void *pDest, int nSize);

    /**
     * Move nSize bytes out of pSrc, correcting the byte order for the file.
     *
     * Vtable slot 9. The one shared implementation forwards to WriteBytes(), and no subclass
     * overrides it.
     *
     * @param pSrc The source buffer.
     * @param nSize The number of bytes to move.
     * @return This stream.
     * @ghidraAddress 0x0050fb88
     */
    virtual Stream &Write(const void *pSrc, int nSize);

    /**
     * Close the stream.
     *
     * Vtable slot 10, which is the last slot rather than the first, because the destructor is
     * declared after the transfer virtuals. `Rnd::BufStream` stores a null there, so a BufStream
     * destroyed through a base pointer jumps to zero; every other subclass supplies a body.
     */
    virtual ~Stream();

    /**
     * Read a NUL-terminated name and append it to a string.
     *
     * The name arrives one byte at a time through ReadBytes() into a 256-byte static buffer at
     * `0x008952e0`. The string is emptied through HxStr::Clear() first, so the argument need not
     * start empty. A name of 256
     * characters or more fills the buffer with no terminator before the flush, and the flush then
     * measures it with `strlen`, which reads past the buffer.
     *
     * @param name The string to append the name to.
     * @return This stream, which Rnd::Manager's reader at `0x0051b450` chains into a second read.
     * @ghidraAddress 0x0050f140
     */
    Stream &ReadString(HxStr &name);
};

} // namespace Rnd
