#pragma once

#include "stream/hxstream.h"

class HxDataChunkReader;
struct HxDataChunkId;

/**
 * Input stream over the payload of one RIFF or Standard MIDI File chunk.
 *
 * `12HxIDataChunk` in the RTTI, derived from HxStream. Its vtable is at `0x007d3ed8`. Offsets and
 * sizes are relative to the payload. The payload lies in another stream between mStart and mEnd.
 * Both constructors set mFatalOnEnd and copy HxStream::mSwapBytes from that stream.
 *
 * Seek() sets HxStream::kStatusRange for an offset outside the payload and then overwrites the
 * status with HxStream::kStatusOk on every path. Tell() and Seek() still test the range flag.
 */
class HxIDataChunk : public HxStream {
public:
    /**
     * Open the current chunk of a reader, and lock the reader.
     *
     * The header is a heap copy of the reader's current header, and the payload starts at the
     * source stream's read position.
     *
     * @param pReader The reader positioned on the chunk.
     * @ghidraAddress 0x00145908
     */
    explicit HxIDataChunk(HxDataChunkReader *pReader);

    /**
     * Read a chunk header at a stream's read position and open that chunk.
     *
     * @param pSource The stream positioned on the chunk header. The chunk does not take
     * ownership.
     * @ghidraAddress 0x00145a10
     */
    explicit HxIDataChunk(HxStream *pSource);

    /**
     * Unlock the reader, if any, and free the header.
     *
     * @ghidraAddress 0x00146080
     */
    ~HxIDataChunk() override;

    /**
     * Move the read position within the payload.
     *
     * Does nothing while HxStream::kStatusRange or HxStream::kStatusFailed is set.
     *
     * @param nOffset The signed distance to move.
     * @param nWhence The origin, one of the HxStreamSeekOrigin values.
     * @ghidraAddress 0x00145b20
     */
    void Seek(int nOffset, int nWhence) override;

    /**
     * Report the read position within the payload.
     *
     * @return The position from the start of the payload, or -1 while HxStream::kStatusRange or
     * HxStream::kStatusFailed is set.
     * @ghidraAddress 0x001460f0
     */
    int Tell() override;

    /**
     * Report the payload size from the chunk header.
     *
     * @return The size in bytes.
     * @ghidraAddress 0x00145fc0
     */
    int Size() override;

    /**
     * Move up to nSize bytes of the payload into pDest.
     *
     * Does nothing unless the status is HxStream::kStatusOk. A read that would cross the end of
     * the payload moves only the bytes before it and sets HxStream::kStatusEnd.
     *
     * @param pDest The destination buffer.
     * @param nSize The number of bytes to move.
     * @return This stream.
     * @ghidraAddress 0x00146160
     */
    HxStream &Read(void *pDest, int nSize) override;

    /**
     * Report the stream the payload lies in.
     *
     * @return The source stream.
     * @ghidraAddress 0x00145fd8
     */
    HxStream *Unknown7() override;

private:
    HxDataChunkReader *mReader; // Reader to unlock on destruction, or null.
    HxStream *mSource;          // Stream the payload lies in.
    HxDataChunkId *mId;         // Heap copy of the chunk header.
    int mStart;                 // Source position of the first payload byte.
    int mEnd;                   // Source position after the last payload byte.
};
