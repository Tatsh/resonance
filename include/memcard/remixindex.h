#pragma once

#include <vector>

#include "game/freqappearance.h"
#include "os/hxstr.h"

class IBStream;
class OBStream;

/** Characters of a level name a RemixIndexElement stores, terminator included. */
constexpr int kRemixIndexLevelNameSize = 32;

/** Characters of a remix name a RemixIndexElement stores, terminator included. */
constexpr int kRemixIndexRemixNameSize = 32;

/** Characters of a remix file name a RemixIndexElement stores, terminator included. */
constexpr int kRemixIndexFileNameSize = 8;

/**
 * One remix described by the index file of a remix save directory.
 *
 * The debug dump at `0x00136150` labels the record `********** RemixIndex element **********` and
 * writes six of its fields through per-field label strings, `" LevelName = "` at `0x007d2b48`,
 * `" RemixName = "` at `0x007d2b58`, `" FileName  = "` at `0x007d2b68`, `" GameOK    = "` at
 * `0x007d2b78`, `" Version   = "` at `0x007d2b88`, and `" AlbumNum  = "` at `0x007d2b98`. Those
 * six strings attest the six field names, so the six are reproduced verbatim rather than restyled.
 * The remaining two members are not attested anywhere and follow the required style. The class
 * name is inferred from the dump label, because the record emits no RTTI descriptor and no
 * `__FILE__` path survives for its translation unit.
 *
 * The record is 104 bytes. The three names are inline character arrays rather than strings, which
 * the dump proves by passing their addresses to the `const char *` insertion operator and which
 * the destructor confirms by releasing nothing below `+0x54`.
 *
 * The destructor at `0x00139410` is the authoritative member list. It releases the `std::vector`
 * at `+0x5c` by destroying each element through the element's own virtual slot, returns the buffer
 * to the pool, and then releases dateTime. The element of that vector is FreqAppearance, 20 bytes
 * with its vptr at `+0x10`, the element MetRemixRecord::appearances and SaveRemixMCT's vector at
 * `+0x68` store.
 *
 * The record is plain data with public members, so it is a `struct`.
 */
struct RemixIndexElement {
    /**
     * Construct an element with only AlbumNum cleared.
     *
     * The body is inline. RemixIndex::ReadFromStream() expands it at `0x00136790` and
     * SaveRemixMCT::WriteIndex() at `0x0017b4ec`, where Reset() follows it.
     */
    RemixIndexElement() : AlbumNum(0) {
    }

    /**
     * Clear the three names, clear GameOK, set Version to 2, and set dateTime to the empty string.
     *
     * Not reconstructed yet.
     *
     * @ghidraAddress 0x001397f0
     */
    void Reset();

    char LevelName[kRemixIndexLevelNameSize]; /*!< Level the remix was built over. +0x00 */
    char RemixName[kRemixIndexRemixNameSize]; /*!< Name the player gave the remix. +0x20 */
    /** Decimal file name of the remix payload inside the directory. +0x40 */
    char FileName[kRemixIndexFileNameSize];
    char GameOK;  /*!< Read as a signed byte by the dump. +0x48 */
    int Version;  /*!< +0x4c */
    int AlbumNum; /*!< +0x50 */
    /**
     * When the remix was saved, from FormatCurrentDateTime(), or `FIXME: default date` when the
     * clock cannot be read. SaveRemixMCT::WriteIndex() writes it. +0x54
     */
    HxStr dateTime;
    /** Appearances of the players who recorded the remix. +0x5c */
    std::vector<FreqAppearance> appearances;
};

/**
 * Index file of one remix save directory, as parsed off a card.
 *
 * RemixIndex::ReadFromStream at `0x001366e0` reads version as four bytes, reads the element count
 * as four more, destroys whatever the vector already stored, and then reads that many elements.
 * RemixIndex::DumpElements at `0x00139920` divides the vector's byte span by 104 for its count,
 * which is what pins the element size independently of the destructor.
 *
 * Neither the class name nor either member name is attested. The class name follows the dump label
 * of its element, and the first member is recorded as a version because it is a single word that
 * precedes the count in the file.
 *
 * The implicit constructor survives out of line at `0x00183ed0`, clearing the vector and leaving
 * version unwritten, and the implicit destructor at `0x00360df8`. The four remix tasks and
 * MetRemixManager construct the record on the stack.
 */
struct RemixIndex {
    /**
     * Read the index back from a stream.
     *
     * Not reconstructed yet.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x001366e0
     */
    void ReadFromStream(IBStream &stream);

    /**
     * Write the index to a stream in the form ReadFromStream() reads.
     *
     * Writes version, the element count, and then each element through `0x00136278`. Not
     * reconstructed yet.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00139858
     */
    void WriteToStream(OBStream &stream);

    /**
     * Print every element to the debug console.
     *
     * Not reconstructed yet.
     *
     * @ghidraAddress 0x00139920
     */
    void DumpElements();

    int version;                             /*!< First word of the file. +0x00 */
    std::vector<RemixIndexElement> elements; /*!< +0x04 */
};
