#pragma once

#include "rndartt/abitmap.h"

class CircBuff;

namespace Rnd {

/**
 * Reader that plays a chunked movie file into per-track handlers.
 *
 * The class emits no RTTI and the image records no name for it, so the name is inferred from the
 * callback classes MovieAsyncCallback and MovieStreamingAsyncCallback that share its unit. The
 * object is 0x380 bytes and is allocated untagged. Rnd::Movie plays its `.mmv` file through one,
 * and the synth plays its sound bank movie through another (0x00462908).
 *
 * A file begins with a MOVS chunk and one MOVT chunk per track, which ParseHeader() copies into
 * mTracks. The chunks that follow carry palettes (PALL), frames (FRAM), blank frames (BLAK), loop
 * points (LOOP), and three kinds of sound data (SNDH, SNDB, SNDP). Update() consumes every
 * chunk whose tick has arrived and passes it to the handler SetTrackHandler() installed for its
 * track. A whole file is read into memory at once. A streaming file is read in 0x8000 byte and
 * smaller requests into a CircBuff in the zone "movieStreamBuff", which must be at least 512K.
 */
class MovieStream {
public:
    /** Tracks a file can describe, and so the handler slots. */
    static constexpr int kTrackCount = 16;

    /** Header of every chunk in a movie file, followed by mSize payload bytes. */
    struct ChunkHeader {
        unsigned int mTag; /*!< Chunk type, a four character code such as FRAM. */
        int mTrackId;      /*!< Track the chunk belongs to, an index into the handler slots. */
        int mSize;         /*!< Payload bytes after the header. */
        int mTicks;        /*!< Tick at which the chunk is due, relative to the last loop. */
    };

    /**
     * Track description a MOVT chunk supplies, 0x2c bytes.
     *
     * ParseHeader() copies the payload whole. Rnd::Movie reads the frame size and nothing else, so
     * the rest of the layout is not recovered. The empty constructor matches the loop the stream's
     * constructor runs over the array without storing anything.
     */
    struct Track {
        Track() {
        }

        unsigned char mReserved00[4];    // +0x00
        short mWidth;                    /*!< Frame width in pixels. +0x04 */
        short mHeight;                   /*!< Frame height in pixels. +0x06 */
        unsigned char mReserved08[0x24]; // +0x08
    };

    /** Payload of a PALL chunk. */
    struct PaletteChunk {
        int mCount;               /*!< Entries that follow. */
        int mFlags;               /*!< Bit 0 records that Update() has swapped the entries. */
        unsigned int mEntries[1]; /*!< The entries, mCount of them. */
    };

    /** Payload of a FRAM chunk, followed by the frame's pixels. */
    struct FrameChunk {
        short mX;        /*!< Column the frame is copied to. */
        short mY;        /*!< Row the frame is copied to. */
        int mReserved04; // +0x04
        int mFlags;      /*!< Bit 0 records that Update() has swapped the colours. */
        ABitmap mBitmap; /*!< Description of the pixels that follow. */
    };

    /** Payload of a BLAK chunk. */
    struct BlankChunk {
        unsigned int mColor; /*!< Colour the whole frame is filled with. */
    };

    /**
     * Receiver of one track's chunks.
     *
     * @param pHeader The chunk.
     * @param pPayload The bytes after the header.
     * @param pData The value installed with the handler.
     */
    typedef void (*ChunkHandler)(ChunkHeader *pHeader, void *pPayload, void *pData);

    /**
     * Open a movie file and queue its first read.
     *
     * A whole file is read into a zone block of its uncompressed length through
     * AsyncLoadFileByPath(). A streaming file takes every byte the zone "movieStreamBuff" has
     * left, which a missing zone, a zone under 512K, or a file that will not open reports as fatal,
     * and its first 0x8000 bytes are read through AsyncSubmitRequest(). Either way the stream joins
     * the list MovieAsyncCallback searches, and ParseHeader() runs once the read completes.
     *
     * @param pszPath The file.
     * @param bStreaming Non-zero to stream rather than read the whole file.
     * @param pnError Receives 0, -1 when the buffer could not be allocated, or -2 when the file
     *                has no length.
     * @ghidraAddress NTSC-U/C: 0x0057f7b8
     * @ghidraAddress PAL: 0x005c27f0
     */
    MovieStream(const char *pszPath, int bStreaming, int *pnError);

    /**
     * Close the file, cancel an outstanding read, and destroy the ring buffer.
     *
     * The data buffer belongs to its zone and is not released, and a stream destroyed before its
     * header arrives stays in the pending list.
     *
     * @ghidraAddress NTSC-U/C: 0x00580858
     * @ghidraAddress PAL: 0x005c3890
     */
    ~MovieStream();

    /**
     * Consume every chunk due by nTick and top up a streaming buffer.
     *
     * A tick earlier than the last one rewinds to the first data chunk. At most one video chunk
     * and one sound chunk are dispatched per call, and a LOOP chunk advances mLoopTicks by its
     * length. A LOOP chunk in a whole file moves the read pointer back to the start. An unknown
     * chunk type is fatal.
     *
     * @param nTick The current tick.
     * @param nReadSize The largest streaming read to queue.
     * @ghidraAddress NTSC-U/C: 0x0057fac8
     * @ghidraAddress PAL: 0x005c2b00
     */
    void Update(int nTick, int nReadSize);

    /**
     * Parse the MOVS and MOVT chunks at the start of a loaded buffer and set up the ring buffer.
     *
     * MovieAsyncCallback::Done() calls it once the first read completes.
     *
     * @param pBuffer The buffer the read filled.
     * @param nBytes The bytes read.
     * @return 0, -1 when the ring buffer could not be allocated, or -6 when the file does not begin
     *         with a MOVS chunk.
     * @ghidraAddress NTSC-U/C: 0x0057ffd0
     * @ghidraAddress PAL: 0x005c3008
     */
    int ParseHeader(char *pBuffer, int nBytes);

    /**
     * Install the handler for one track.
     *
     * @param nTrackId The track.
     * @param pfnHandler The handler, or null to drop the track's chunks.
     * @param pData The value passed to the handler.
     * @ghidraAddress NTSC-U/C: 0x005808d0
     * @ghidraAddress PAL: 0x005c3908
     */
    void SetTrackHandler(int nTrackId, ChunkHandler pfnHandler, void *pData);

    /**
     * Queue the next streaming read of up to nBytes, as Update() does inline.
     *
     * Nothing is queued unless the stream is streaming and loaded, no other streaming read is
     * outstanding, and more than 0x400 bytes are free. A read that would pass the end of the file
     * is shortened, or the file is rewound to the first data chunk when nothing is left. The
     * out-of-line copy has no caller, and its name is inferred.
     *
     * @param nBytes The largest read to queue.
     * @return The bytes queued, or 0.
     * @ghidraAddress NTSC-U/C: 0x005808e8
     * @ghidraAddress PAL: 0x005c3920
     */
    int RequestRead(int nBytes);

    int mFile;                           /*!< The open file, or -1. +0x00 */
    CircBuff *mCircBuff;                 /*!< Ring buffer over the chunk data. +0x04 */
    char *mBuffer;                       /*!< Start of the loaded data, headers included. +0x08 */
    int mBufferSize;                     /*!< Size of the data buffer. +0x0c */
    unsigned char mReserved10[8];        // +0x10
    Track mTracks[kTrackCount];          /*!< Track descriptions. +0x18 */
    ChunkHandler mHandlers[kTrackCount]; /*!< Handler per track. +0x2d8 */
    void *mHandlerData[kTrackCount];     /*!< Value passed to each handler. +0x318 */
    /**
     * Tick offset the LOOP chunks have accumulated. Public because the synth's sound bank starter
     * at 0x00462908 sets it to the song tick directly. +0x358
     */
    int mLoopTicks;
    int mLastTick;                 /*!< Tick of the previous Update(). +0x35c */
    int mDataStart;                /*!< File offset of the first data chunk. +0x360 */
    unsigned char mReserved364[4]; // +0x364
    int mStreaming;                /*!< Non-zero when the file is streamed. +0x368 */
    int mLoaded;                   /*!< Non-zero once ParseHeader() has succeeded. +0x36c */
    int mFileLength;               /*!< Uncompressed length of the file. +0x370 */
    int mFileOffset;               /*!< File offset the next streaming read starts at. +0x374 */
    int mAsyncHandle;              /*!< Identifier of the outstanding read, or 0. +0x378 */
    int mSoundHold;                /*!< Updates left before another sound chunk may play. +0x37c */
};

/**
 * Chunk type codes, four characters read as a word, built by the unit's static initialiser.
 *
 * Rnd::Movie's chunk handler compares against the three video codes as well.
 *
 * @ghidraAddress NTSC-U/C: 0x007678f0
 * @ghidraAddress PAL: 0x007ab620
 */
extern const unsigned int g_nMovsTag;
/**
 * @ghidraAddress NTSC-U/C: 0x007678f8
 * @ghidraAddress PAL: 0x007ab628
 */
extern const unsigned int g_nMovtTag;
/**
 * @ghidraAddress NTSC-U/C: 0x00767900
 * @ghidraAddress PAL: 0x007ab630
 */
extern const unsigned int g_nPallTag;
/**
 * @ghidraAddress NTSC-U/C: 0x00767908
 * @ghidraAddress PAL: 0x007ab638
 */
extern const unsigned int g_nFramTag;
/**
 * @ghidraAddress NTSC-U/C: 0x00767910
 * @ghidraAddress PAL: 0x007ab640
 */
extern const unsigned int g_nBlakTag;
/**
 * @ghidraAddress NTSC-U/C: 0x00767918
 * @ghidraAddress PAL: 0x007ab648
 */
extern const unsigned int g_nLoopTag;
/**
 * @ghidraAddress NTSC-U/C: 0x00767920
 * @ghidraAddress PAL: 0x007ab650
 */
extern const unsigned int g_nSndhTag;
/**
 * @ghidraAddress NTSC-U/C: 0x00767928
 * @ghidraAddress PAL: 0x007ab658
 */
extern const unsigned int g_nSndbTag;
/**
 * @ghidraAddress NTSC-U/C: 0x00767930
 * @ghidraAddress PAL: 0x007ab660
 */
extern const unsigned int g_nSndpTag;

/**
 * Text for each error a MovieStream reports, indexed by the negated error code.
 *
 * "NO ERROR", "Out of memory", "Can't open file", "Read error", "Write error", "Invalid filename or
 * extension", "File/data format is bad", "Missing driver", "Driver/file is wrong version",
 * "Invalid mode", "Can't access device", and "Exceeded limit". Rnd::Movie::OpenMovieFile() is the
 * one reader.
 *
 * @ghidraAddress NTSC-U/C: 0x007a8400
 * @ghidraAddress PAL: 0x007ec100
 */
extern const char *const g_apszMovieStreamErrors[];

} // namespace Rnd
