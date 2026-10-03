#pragma once

#include <iostream>

#include "mid/tick.h"
#include "msg/musemsg.h"

class IBStream;
class OBStream;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008f0000`. It has MuseMsg as its one base. The object is 0x10 bytes
 * and its vtable is at `0x00812e90`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable. The fields through
 * `+0x07` belong to MuseMsg and are declared there.
 *
 * The payload layout comes from the run of field copies in Clone(). AxePhraseMaker::OnStdMidi()
 * builds one on its stack at `0x0019bad4` from its channel, a held note's number and velocity,
 * and the note's clamped length, which identifies the fields. PrintExtra() writes mChannel after
 * the label ` n`, the label StdMidiMsg::PrintExtra() places ahead of its channel. mLength at
 * `+0x0c` is a tick count, handed to Sch::Tick::Print() in place and initialised to kTickInfinity
 * by New(). saveGuts() and restoreGuts() move only its low sixteen bits.
 *
 * The destructor at `0x003dc0e8` is compiler-generated and has no declaration here.
 */
class NoteMsg : public MuseMsg {
public:
    /**
     * Identity that Type() reports.
     *
     * @ghidraAddress NTSC-U/C: 0x006d01cc
     * @ghidraAddress PAL: 0x00713964
     */
    static unsigned int sID;

    /**
     * Construct a message with the position and the length at kTickInfinity.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    NoteMsg() {
    }

    /**
     * Report one note.
     *
     * Inline, with no address of its own. AxePhraseMaker::OnStdMidi() expands it on its stack at
     * `0x0019bad4`, and AxePhraseMaker::FinishPhrase() at `0x0019c270`.
     *
     * @param nTick The song position of the note-on, in MIDI ticks.
     * @param nChannel The MIDI channel.
     * @param nNote The note number.
     * @param nVelocity The note-on velocity.
     * @param length The length of the note.
     */
    NoteMsg(int nTick,
            unsigned char nChannel,
            unsigned char nNote,
            unsigned char nVelocity,
            Sch::Tick length)
        : MuseMsg(nTick), mChannel(nChannel), mNote(nNote), mVelocity(nVelocity), mLength(length) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d6d80
     * @ghidraAddress PAL: 0x0040ec70
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003dc208
     * @ghidraAddress PAL: 0x00414640
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return sID.
     * @ghidraAddress NTSC-U/C: 0x003dc280
     * @ghidraAddress PAL: 0x004146b8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `NoteMsg`.
     * @ghidraAddress NTSC-U/C: 0x003dc290
     * @ghidraAddress PAL: 0x004146c8
     */
    virtual const char *GetName() const;

    /**
     * Write the message to a diagnostic stream.
     *
     * Writes the song position, both data bytes as numbers, the second position, and the byte at
     * `+0x08` after ` n`.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e3760
     * @ghidraAddress PAL: 0x0041bb00
     */
    virtual void PrintExtra(std::ostream &stream) const;

    /**
     * Write the three bytes and the low sixteen bits of the second position to a stream.
     *
     * The bytes go one at a time through OBStream::Write() and the position through
     * OBStream::WriteLE(). The song position MuseMsg provides is not written.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003d80c0
     * @ghidraAddress PAL: 0x00410228
     */
    virtual void saveGuts(OBStream &stream) const;

    /**
     * Read the three bytes and the second position back from a stream.
     *
     * The position arrives as an unsigned sixteen-bit value and is widened into the member.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x003e3808
     * @ghidraAddress PAL: 0x0041bba8
     */
    virtual void restoreGuts(IBStream &stream);

    /**
     * The MIDI channel. +0x08
     *
     * Public because MuseSynth::StartNotePlayer() at `0x001aa6bc` reads it directly.
     */
    unsigned char mChannel;

    /**
     * The note number. +0x09
     *
     * Public because RiffRangeFinder::DispatchPriv() at `0x001c4574` reads it directly with `lbu`,
     * with no accessor in the image, widening its range of notes to include it.
     */
    unsigned char mNote;

    /**
     * The note-on velocity. +0x0a
     *
     * Public because MuseSynth::StartNotePlayer() at `0x001aa6c8` reads it directly.
     */
    unsigned char mVelocity;

    /**
     * The length of the note, in MIDI ticks. +0x0c
     *
     * Public because NoteFinder::DispatchPriv() at `0x001023b0` reads it directly with no
     * accessor in the image, adding it to the inherited mTick to find where the note ends.
     */
    Sch::Tick mLength;
};
