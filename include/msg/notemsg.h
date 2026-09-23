#pragma once

#include <iostream>

#include "mid/mbt.h"
#include "msg/musemsg.h"

class IBStream;
class OBStream;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `7NoteMsg` in the RTTI descriptor at `0x008f0000`, with MuseMsg as its one base. The object is
 * 0x10 bytes and its vtable is at `0x00812e90`. The members below are the whole of the class:
 * everything recovered comes from them, and no other routine in the image refers to this type by
 * anything but its vtable. The fields through `+0x07` belong to MuseMsg and are declared there.
 *
 * The payload layout comes from the run of field copies in Clone(). AxePhraseMaker::OnStdMidi()
 * builds one on its stack at `0x0019bad4` from its channel, a held note's number and velocity,
 * and the note's clamped length, which names the fields. Print() writes mChannel after the label
 * ` n`, the label StdMidiMsg::Print() places ahead of its channel. mLength at `+0x0c` is a tick
 * count, handed to Mid::MBT::Print() in place and initialised to kMBTInfinity by New(). Save() and
 * Load() move only its low sixteen bits.
 *
 * The destructor at `0x003dc0e8` is compiler-generated and has no declaration here.
 */
class NoteMsg : public MuseMsg {
public:
    /**
     * Construct a message with the position and the length at kMBTInfinity.
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
            Mid::MBT length)
        : MuseMsg(nTick), mChannel(nChannel), mNote(nNote), mVelocity(nVelocity), mLength(length) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory.
     *
     * @return The message.
     * @ghidraAddress 0x003d6d80
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003dc208
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_dwNoteMsgType.
     * @ghidraAddress 0x003dc280
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `NoteMsg`.
     * @ghidraAddress 0x003dc290
     */
    virtual const char *Name();

    /**
     * Write the message to a diagnostic stream.
     *
     * Writes the song position, both data bytes as numbers, the second position, and the byte at
     * `+0x08` after ` n`.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e3760
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the three bytes and the low sixteen bits of the second position to a stream.
     *
     * The bytes go one at a time through OBStream::WriteBytes() and the position through
     * OBStream::Write(). The song position MuseMsg provides is not written.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003d80c0
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the three bytes and the second position back from a stream.
     *
     * The position arrives as an unsigned sixteen-bit value and is widened into the member.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003e3808
     */
    virtual void Load(IBStream &stream);

private:
    unsigned char mChannel; // +0x08

public:
    /**
     * The note number. +0x09
     *
     * Public because RiffRangeFinder::HandleMessage() at `0x001c4574` reads it directly with `lbu`,
     * with no accessor in the image, widening its range of notes to include it.
     */
    unsigned char mNote;

private:
    unsigned char mVelocity; // +0x0a

public:
    /**
     * The length of the note, in MIDI ticks. +0x0c
     *
     * Public because NoteFinder::HandleMessage() at `0x001023b0` reads it directly with no
     * accessor in the image, adding it to the inherited mTick to find where the note ends.
     */
    Mid::MBT mLength;
};

/**
 * Identity that NoteMsg::Type() reports.
 *
 * This word belongs to NoteMsg because NoteMsg::Type() at `0x003dc280` returns it. Several
 * handlers elsewhere read the same word to compare against it, which is the expected shape for a
 * registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d01cc
 */
extern unsigned int g_dwNoteMsgType;
