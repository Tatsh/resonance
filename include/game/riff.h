#pragma once

#include <cstddef>
#include <iostream>

#include "gs/multimuse.h"
#include "mid/mbt.h"

/**
 * One riff of a track at one difficulty level, as a sequence of MIDI messages.
 *
 * `4Riff` in the RTTI descriptor, with MultiMuse as its one base. Its vtable at `0x007e4f78` runs
 * four entries: the type function, the destructor at `0x001cead0`, the retained
 * Attachment::Destroy(), and the Print() override below. The object is 0x1c bytes, the 0x14-byte
 * MultiMuse followed by the two members below.
 *
 * mId is the difficulty level the riff belongs to. RiffSet stores a riff at the index mId names,
 * and Print() labels it `riff[id=`.
 *
 * The destructor is implicitly declared. It runs MultiMuse's destructor and releases the object
 * under the tag `Riff`.
 *
 * Both members are private apart from mId. RiffSet and TrackData read mId directly.
 */
class Riff : public MultiMuse {
public:
    /**
     * Allocate a riff from the tagged heap under the tag `Riff`.
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     * @ghidraAddress 0x001ceb70
     */
    void *operator new(size_t nSize);

    /**
     * Release a riff to the tagged heap.
     *
     * @param pBlock The block.
     * @ghidraAddress 0x001ceb90
     */
    void operator delete(void *pBlock);

    /**
     * Construct an empty riff for one difficulty level.
     *
     * @param nId The difficulty level.
     * @ghidraAddress 0x001cebb0
     */
    explicit Riff(int nId);

    /**
     * Write `riff[id=` and the level, then the sequence through MultiMuse::Print().
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x001cec58
     */
    virtual void Print(std::ostream &stream);

    /**
     * Schedule a NoteMsg built from four values.
     *
     * The body is not written, because NoteMsg declares its payload private and no constructor
     * that takes it. The position is clamped to zero below 3 and moved one tick later above 30,
     * and the message is added through MultiMuse::Add() with appending allowed.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param nUnknown09 The byte stored at the message's `+0x09`.
     * @param nUnknown0a The byte stored at the message's `+0x0a`.
     * @param nUnknown0c The word stored at the message's `+0x0c`.
     * @param nUnknown08 The byte stored at the message's `+0x08`.
     * @ghidraAddress 0x001ce778
     */
    void AddNoteMsg(int nTick,
                    unsigned char nUnknown09,
                    unsigned char nUnknown0a,
                    int nUnknown0c,
                    unsigned char nUnknown08);

    /**
     * Schedule a StdMidiMsg built from a status byte and two data bytes.
     *
     * The body is not written, for the reason recorded on AddNoteMsg(). The position is clamped
     * the same way, and the status is the bitwise or of the second and the fifth arguments.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param nStatus The status byte without its channel.
     * @param nData1 The first data byte.
     * @param nData2 The second data byte.
     * @param nChannel The channel, combined into the status with a bitwise or.
     * @ghidraAddress 0x001ce8d0
     */
    void AddMidiMsg(int nTick,
                    unsigned char nStatus,
                    unsigned char nData1,
                    unsigned char nData2,
                    unsigned char nChannel);

    int mId; /*!< The difficulty level, 0 through 3. +0x14 */

private:
    // AutoRiffer::PlayRiff() at 0x00199758 reads mLength directly.
    friend class AutoRiffer;
    // LevelConverter::NextRiff() at 0x001e7eb0 writes mLength directly.
    friend class LevelConverter;

    // The length the riff repeats over. Zero on construction.
    Mid::MBT mLength; // +0x18
};
