#pragma once

#include "stream/hxchunkheader.h"

class HxStream;

/**
 * Cursor over the chunks inside one RIFF list or Standard MIDI File.
 *
 * The class is not polymorphic and has no RTTI. Its name comes from the debugging symbols of the
 * North American demo release. The reader walks the chunk headers between mStart and mEnd on a
 * shared HxStream, seeking to each header in turn.
 * Opening a nested reader or an HxIDataChunk on the current chunk locks the parent reader, and
 * destroying the nested object unlocks it. No routine tests mLocked.
 *
 * Every member is public, because HxIDataChunk and Mid::Reader read the fields directly and
 * the image exposes no accessor.
 */
class HxIListChunk {
public:
    /**
     * Open a reader on a stream at its read position.
     *
     * With bReadHeader set, the reader reads the chunk header at the read position and walks the
     * chunks inside it. Otherwise the reader treats the rest of the stream as one `LIST` whose
     * size is HxStream::Size() less HxStream::GetMarker().
     *
     * @param pStream The stream to read. The reader does not take ownership.
     * @param bReadHeader Read an enclosing chunk header first.
     * @ghidraAddress NTSC-U/C: 0x00145c40
     * @ghidraAddress PAL: 0x00146758
     */
    HxIListChunk(HxStream *pStream, bool bReadHeader);

    /**
     * Open a reader on the chunks inside the current chunk of another reader, and lock it.
     *
     * The enclosing header is a heap copy of pParent's current header.
     *
     * @param pParent The reader whose current chunk is a list.
     * @ghidraAddress NTSC-U/C: 0x00146220
     * @ghidraAddress PAL: 0x00146d38
     */
    explicit HxIListChunk(HxIListChunk *pParent);

    /**
     * Unlock the parent reader, if any, and free the enclosing header.
     *
     * @ghidraAddress NTSC-U/C: 0x00146308
     * @ghidraAddress PAL: 0x00146e20
     */
    ~HxIListChunk();

    /**
     * Compute mEnd from the enclosing header, lock the parent reader, if any, and reset.
     *
     * @ghidraAddress NTSC-U/C: 0x00146360
     * @ghidraAddress PAL: 0x00146e78
     */
    void Init();

    /**
     * Seek back to the first chunk and forget the current one.
     *
     * @ghidraAddress NTSC-U/C: 0x001463b0
     * @ghidraAddress PAL: 0x00146ec8
     */
    void Reset();

    /**
     * Advance to the next chunk and read its header.
     *
     * The next chunk begins after the header and payload of this one. Every chunk except an `MTrk`
     * track is padded to an even size.
     *
     * @return The new current header, or null once the enclosing chunk is exhausted.
     * @ghidraAddress NTSC-U/C: 0x00145db8
     * @ghidraAddress PAL: 0x001468d0
     */
    HxChunkHeader *Next();

    /**
     * Report the current chunk header.
     *
     * @return The current header, or null before the first Next() or after the last.
     * @ghidraAddress NTSC-U/C: 0x00146408
     * @ghidraAddress PAL: 0x00146f20
     */
    HxChunkHeader *CurSubChunkHeader();

    /**
     * Advance until a chunk with the given name is current.
     *
     * The search starts from the chunk after the current one and does not rewind.
     *
     * @param name The chunk name, or a list's form type.
     * @return The matching header, or null when no later chunk matches.
     * @ghidraAddress NTSC-U/C: 0x00146428
     * @ghidraAddress PAL: 0x00146f40
     */
    HxChunkHeader *Next(const HxChunkName &name);

    /**
     * Mark the reader as having a nested reader or chunk open.
     *
     * @ghidraAddress NTSC-U/C: 0x001464a0
     * @ghidraAddress PAL: 0x00146fb8
     */
    void Lock();

    /**
     * Clear the mark Lock() sets.
     *
     * @ghidraAddress NTSC-U/C: 0x001464b0
     * @ghidraAddress PAL: 0x00146fc8
     */
    void Unlock();

    HxIListChunk *mParent;  /*!< The enclosing reader, or null for a top-level reader. +0x00 */
    HxStream *mStream;      /*!< The stream every chunk lies in. +0x04 */
    HxChunkHeader *mHeader; /*!< Heap header of the enclosing chunk. +0x08 */
    int mStart;             /*!< Stream position of the first chunk. +0x0c */
    int mEnd;               /*!< Stream position after the last chunk. +0x10 */
    int mLocked;            /*!< Non-zero while a nested reader or chunk is open. +0x14 */
    HxChunkHeader mCurrent; /*!< Header of the current chunk. +0x18 */
    int mHasCurrent;        /*!< Non-zero while mCurrent is valid. +0x24 */
    int mAtStart;           /*!< Non-zero from Reset() until the first Next(). +0x28 */
    int mNext;              /*!< Stream position of the next chunk header. +0x2c */
};
