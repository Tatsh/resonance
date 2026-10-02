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
 * bytes, one vector of one-byte notes kept in ascending order. LevelConverter builds one note at a
 * time through AddNote() and hands it to TrackData::AddHarmony(), which copies it.
 *
 * The notes are unsigned. The two binary searches and every reader load them zero-extended and
 * compare them unsigned. Print() alone loads them sign-extended, which is the conversion to `char`
 * that printing a note performs.
 *
 * The constructors, the destructor, and the assignment are implicitly declared. The copy
 * constructor is the vector copy constructor instantiation at `0x001d73f8`.
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
     * Insert a note in ascending order, after any equal note.
     *
     * The position comes from std::upper_bound(), instantiated at `0x001a4f80`. The title is
     * inferred.
     *
     * @param nNote The note.
     * @ghidraAddress 0x001a4db0
     */
    void AddNote(unsigned char nNote);

    /**
     * Snap a note to the nearer note of the harmony.
     *
     * The routine finds the first note not below nNote with std::lower_bound(), instantiated at
     * `0x001a4fd0`, stepping back one when the search runs off the end, and takes the note below
     * that one as well when one exists. A note above the midpoint of the two resolves to the higher
     * one, and a note at or below it to the lower one. The title is inferred.
     *
     * @param nNote The note.
     * @return The nearer note, or nNote when the harmony is empty.
     * @ghidraAddress 0x001a4e28
     */
    unsigned char Snap(unsigned char nNote);

    /**
     * Report the lowest and highest notes.
     *
     * The title is inferred.
     *
     * @param pLow Receives the first note, or 20 when the harmony is empty.
     * @param pHigh Receives the last note, or 120 when the harmony is empty.
     * @ghidraAddress 0x001a4eb0
     */
    void GetRange(int *pLow, int *pHigh);

    /**
     * Write the notes to a diagnostic stream as `(` followed by each note and a space, then `)`.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x001a4ee8
     */
    void Print(std::ostream &stream);

    std::vector<unsigned char> mNotes; /*!< The notes, one byte each, ascending. +0x00 */
};
