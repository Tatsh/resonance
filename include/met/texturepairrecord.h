#pragma once

#include <vector>

#include "os/hxstr.h"
#include "rnd/object.h"

/**
 * Pair of texture names with the objects resolved from them.
 *
 * The class is not polymorphic and emits no RTTI, so no descriptor, no vtable, and no allocation
 * tag identifies it. Every instance in the image is a by-value member of a front-end screen, so
 * the tag lever does not apply. The class name is inferred and agrees with the title the Ghidra
 * program already records for the constructor. No method name and no member name is attested
 * anywhere in the image, so every identifier below follows the required style rather than a
 * recovered spelling.
 *
 * Six screens embed a pair of these records, and each pair is built from the texture names
 * `gSongLogo1.tex` with `gSongLogo2.tex` and `gSongLabel1.tex` with `gSongLabel2.tex`.
 * MetArenasScreen, MetJukeboxBaseScreen, MetMultiEndRemixScreen, MetRemixDataScreen,
 * MetSoloEndRemixScreen, and MetSoloStagesScreen are the six. Three of their headers record the
 * record as a reserved 0x30-byte span, which predates this declaration.
 *
 * The record is 0x30 bytes. The element type of the object vector is inferred from the vectors of
 * the surrounding Met classes rather than recovered, because the destructor deallocates the buffer
 * without running an element destructor, which fixes the element as a pointer or a plain value of
 * four bytes and nothing further.
 */
class TexturePairRecord {
public:
    /**
     * Record the two texture names and start with no resolved object.
     *
     * Both names are copied into the record. The four integers below the object vector start at
     * zero except the word at `+0x10`, which starts at one.
     *
     * @param first The first texture name.
     * @param second The second texture name.
     * @ghidraAddress 0x00246de0
     */
    TexturePairRecord(const HxStr &first, const HxStr &second);

    /**
     * Release the two names and the object vector.
     *
     * @ghidraAddress 0x001fc568
     */
    ~TexturePairRecord();

    /**
     * Mark the record as needing its textures resolved again.
     *
     * Writes one to the word at `+0x2c` and does nothing else. The method name is inferred from
     * the one field the routine writes and from its call sites, which run it whenever a screen
     * becomes visible.
     *
     * @ghidraAddress 0x002498c0
     */
    void invalidate();

private:
    std::vector<Rnd::Object *> objects_; // +0x00
    int unknown0c_;                      // +0x0c
    int unknown10_;                      // +0x10, starts at 1
    int unknown14_;                      // +0x14
    int unknown18_;                      // +0x18
    HxStr firstName_;                    // +0x1c
    HxStr secondName_;                   // +0x24
    int unknown2c_;                      // +0x2c
};
