#pragma once

#include <vector>

#include "os/hxstr.h"

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
 * to the pool, and then releases unknown54. The element of that vector is 20 bytes with a virtual
 * function table pointer at `+0x10` and sixteen bytes of data before it. Its class is not
 * recovered, so the vector is recorded as a reserved span rather than declared. The same element
 * type appears in the vector SaveRemixMCT stores at `+0x68`.
 *
 * The record is a plain data aggregate, so it is a `struct` with public members.
 */
struct RemixIndexElement {
    char LevelName[kRemixIndexLevelNameSize]; /*!< Level the remix was built over. +0x00 */
    char RemixName[kRemixIndexRemixNameSize]; /*!< Name the player gave the remix. +0x20 */
    /** Decimal file name of the remix payload inside the directory. +0x40 */
    char FileName[kRemixIndexFileNameSize];
    char GameOK;     /*!< Read as a signed byte by the dump. +0x48 */
    int Version;     /*!< +0x4c */
    int AlbumNum;    /*!< +0x50 */
    HxStr unknown54; /*!< Released by the destructor. Purpose not recovered. +0x54 */
    /**
     * A `std::vector` of the 20-byte polymorphic element described in the class documentation.
     * Recorded as a reserved span, because the element class is not recovered. +0x5c
     */
    unsigned char reserved5c[0xc];
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
 */
struct RemixIndex {
    int version;                             /*!< First word of the file. +0x00 */
    std::vector<RemixIndexElement> elements; /*!< +0x04 */
};
