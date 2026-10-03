#pragma once

#include "mid/tick.h"
#include "msg/message.h"

/**
 * Abstract base shared by a family of messages.
 *
 * Its RTTI descriptor is at `0x008ef330`. It has Message as its one base. Its vtable is at
 * `0x007dd628`. Every one of the 4 derived Clone() routines installs it before the derived
 * table. That table has eight entries and a zero terminator at index 8. Slots 2, 3, and 4 address
 * the pure-virtual handler, and slots 5, 6, and 7 retain Message::PrintExtra(),
 * Message::saveGuts(), and Message::restoreGuts(), so the class implements none of the virtuals
 * it inherits and is never instantiated. It is declared because its derived classes need it.
 *
 * The destructor at slot 1 is byte-identical to Message's, because it stores its own table pointer,
 * the inlined base destructor overwrites it, and the compiler drops the dead first store. It is
 * therefore the implicitly declared destructor, and this class owes no definition.
 *
 * Slot 0 of that table is `0x0019a988` and slot 1 the destructor at `0x0019a958`.
 *
 * The payload is inferred from the derived classes rather than from any routine of its own,
 * because Clone() in a derived class copies the whole object. Only this one field is copied
 * identically by all four derived classes. MultiMuseMsg copies a word at `+0x08` where NoteMsg
 * and StdMidiMsg copy bytes there, and a single word store cannot straddle a base and a derived
 * class in a member-wise copy, so `+0x08` belongs to each derived class rather than here.
 */
class MuseMsg : public Message {
public:
    /**
     * Start the song position at kTickInfinity.
     *
     * No address attaches to the constructor on its own. Every construction in the image expands
     * it in place, storing `0x2aaaaaab` at `+0x04`: the New() factories of StdMidiMsg at
     * `0x003d6d40`, NoteMsg at `0x003d6d80`, and SustainNoteMsg at `0x003d6e50`, and the
     * StdMidiMsg Mixer builds on its stack at `0x001a7340`.
     */
    MuseMsg() : mTick(kTickInfinity) {
    }

    /**
     * Start the song position at a tick.
     *
     * Inline, with no address of its own. The stack builds of StdMidiMsg in NotePlayer at
     * `0x001b3fb8` and of AllNotesOffMsg in AutoRiffer at `0x001993c4` store the position directly
     * at `+0x04`.
     *
     * @param nTick The song position, in MIDI ticks.
     */
    explicit MuseMsg(int nTick) : mTick(nTick) {
    }

    /**
     * Copy this message and move the copy to a song position.
     *
     * The copy comes from Clone(), and the caller owns it. Sequencer::Dispatch() is the caller.
     *
     * @param nTick The song position of the copy, in MIDI ticks.
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003e3620
     * @ghidraAddress PAL: 0x0041b9c0
     */
    MuseMsg *CloneAndShift(int nTick);

    /**
     * The song position, in MIDI ticks. +0x04
     *
     * Public because NoteFinder::DispatchPriv() at `0x001023b0` reads it directly from a NoteMsg
     * with no accessor in the image.
     */
    int mTick;
};

/**
 * First identity in the range the MuseMsg classes register, 200.
 *
 * The GsMuseUtil Shifter at `0x001ab9e4` copies any message whose identity lies from this word up
 * to g_nEndMuseMsgType.
 *
 * @ghidraAddress NTSC-U/C: 0x006d01bc
 * @ghidraAddress PAL: 0x00713954
 */
extern int g_nFirstMuseMsgType;

/**
 * Identity one past the MuseMsg range, 206.
 *
 * The GsMuseUtil Shifter at `0x001ab9f8` reads it as the end of the range.
 *
 * @ghidraAddress NTSC-U/C: 0x006d01c0
 * @ghidraAddress PAL: 0x00713958
 */
extern int g_nEndMuseMsgType;
