#pragma once

#include <iostream>

#include "stream/hxchunkname.h"

class HxStream;

/**
 * Header of one RIFF or Standard MIDI File chunk: its name, its size, and whether it is a list.
 *
 * The class is not polymorphic and has no RTTI. Its name comes from the debugging symbols of the
 * North American demo release. The twelve-byte size comes from the heap copies HxIListChunk and
 * HxIDataChunk allocate. A header named `LIST` or `RIFF`
 * is a list. Read() replaces its name with the form type in the next four bytes, marks the header
 * as a list, and takes the four bytes out of the size.
 *
 * Every member is public, because HxIListChunk, HxIDataChunk, and Mid::Reader read the
 * fields directly and the image exposes no accessor.
 */
struct HxChunkHeader {
    /**
     * Construct an unread header named `????`.
     *
     * Inline. HxIListChunk's constructors and HxIDataChunk's stream constructor expand it,
     * copying the name from the literal at `0x007d3eb8`.
     */
    HxChunkHeader() : mName("????"), mSize(0), mIsList(0) {
    }

    /**
     * Construct a header from its three fields.
     *
     * Inline. HxIListChunk's stream constructor expands it for a stream with no header of its
     * own, naming the whole stream a list.
     *
     * @param name The name.
     * @param nSize The size.
     * @param nIsList Non-zero for a list.
     */
    HxChunkHeader(const HxChunkName &name, int nSize, int nIsList)
        : mName(name), mSize(nSize), mIsList(nIsList) {
    }

    /**
     * Report the chunk name by value.
     *
     * Inline. Every comparison outside Read() compares a stack copy of the name, the temporary this
     * accessor returns.
     *
     * @return A copy of mName.
     */
    HxChunkName Name() const {
        return mName;
    }

    /**
     * Read the header from a stream.
     *
     * Reads the name raw and the size through HxStream::ReadSwapped(). A `LIST` or `RIFF` header
     * then reads its form type over the name, is marked a list, and loses four bytes of size.
     * Any other header is marked not a list.
     *
     * @param stream The stream to read from.
     * @return The stream.
     * @ghidraAddress NTSC-U/C: 0x00145fe0
     * @ghidraAddress PAL: 0x00146af8
     */
    HxStream &Read(HxStream &stream);

    /**
     * Write the header to a diagnostic stream as `LIST:` for a list, the four characters, and the
     * size in angle brackets.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x00145868
     * @ghidraAddress PAL: 0x00146380
     */
    void Print(std::ostream &stream);

    HxChunkName mName; /*!< The chunk name, or a list's form type. +0x00 */
    int mSize;         /*!< Bytes of payload after the header. +0x04 */
    int mIsList;       /*!< Non-zero for a `LIST` or `RIFF` header. +0x08 */
};

/**
 * Read a chunk header from a stream.
 *
 * A second out-of-line body with the same steps as HxChunkHeader::Read(), taking the stream first.
 * HxIListChunk::Next() reads through it.
 *
 * @param stream The stream to read from.
 * @param id The header to fill.
 * @return The stream.
 * @ghidraAddress NTSC-U/C: 0x001464b8
 * @ghidraAddress PAL: 0x00146fd0
 */
HxStream &operator>>(HxStream &stream, HxChunkHeader &id);
