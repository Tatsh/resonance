#pragma once

#include <cstddef>
#include <iostream>
#include <vector>

#include "os/mem.h"

/**
 * The notes of one harmony a track plays from one song position onward.
 *
 * The class emits no RTTI, because it is not polymorphic. Its title is attested by the allocation
 * tag `Harmony`, under which TrackData allocates it and its deleter releases it. The object is 12
 * bytes, one vector of one-byte notes. LevelConverter fills that vector from the track's name map
 * and hands it to TrackData::AddHarmony().
 *
 * The constructor copies the notes through the vector copy constructor instantiation at
 * `0x001d73f8`. The destructor is implicitly declared.
 */
struct Harmony {
    /**
     * Allocate a harmony from the tagged heap under the tag `Harmony`.
     *
     * No out-of-line body exists. TrackData::AddHarmony() expands the call inline.
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     */
    void *operator new(size_t nSize) {
        return AllocateTaggedMemory(nSize, "Harmony");
    }

    /**
     * Release a harmony to the tagged heap.
     *
     * No out-of-line body exists. TrackData's deleter expands the call inline.
     *
     * @param pBlock The block.
     */
    void operator delete(void *pBlock) {
        FreeTaggedMemory(pBlock, "Harmony");
    }

    /**
     * Copy a list of notes.
     *
     * @param notes The notes.
     */
    explicit Harmony(const std::vector<char> &notes) : mNotes(notes) {
    }

    /**
     * Write the notes to a diagnostic stream as `(` followed by each note and a space, then `)`.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x001a4ee8
     */
    void Print(std::ostream &stream);

    std::vector<char> mNotes; /*!< The notes, one byte each. +0x00 */
};
