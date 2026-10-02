#pragma once

#include <cstddef>
#include <iostream>

#include "os/mem.h"

class Riff;

/** Difficulty levels a RiffSet stores a riff for. */
constexpr int kRiffSetLevelCount = 4;

/**
 * The riffs of one phrase, one per difficulty level.
 *
 * The class emits no RTTI and is not polymorphic. Its title is attested by the allocation tag
 * `RiffSet`, under which TrackData allocates it and its destructor releases it. The object is 0x10
 * bytes of four riff pointers indexed by Riff::mId.
 *
 * TrackData deletes every RiffSet it creates through TrackData::mRiffSetsOwned, and each bar the
 * set covers refers to it from the bar's riff list.
 */
struct RiffSet {
    /**
     * Allocate a set from the tagged heap under the tag `RiffSet`.
     *
     * No out-of-line body exists. TrackData expands the call inline.
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     */
    void *operator new(size_t nSize) {
        return AllocateTaggedMemory(nSize, "RiffSet");
    }

    /**
     * Release a set to the tagged heap.
     *
     * No out-of-line body exists. The deleting destructor expands the call inline.
     *
     * @param pBlock The block.
     */
    void operator delete(void *pBlock) {
        FreeTaggedMemory(pBlock, "RiffSet");
    }

    /**
     * Construct a set with no riff at any level.
     *
     * @ghidraAddress 0x001cecc0
     */
    RiffSet();

    /**
     * Give back the reference to every riff the set has.
     *
     * @ghidraAddress 0x001cecf0
     */
    ~RiffSet();

    /**
     * Write every level to a diagnostic stream, one line each, with `[empty]` for a missing riff.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x001ced70
     */
    void Print(std::ostream &stream);

    Riff *mRiffs[kRiffSetLevelCount]; /*!< The riff of each level, or null. +0x00 */
};
