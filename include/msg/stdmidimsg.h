#pragma once

#include <iostream>

#include "msg/musemsg.h"

class IBStream;
class OBStream;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `10StdMidiMsg` in the RTTI descriptor at `0x008ef3f0`, with MuseMsg as its one base. The object
 * is 0xc bytes and its vtable is at `0x00812ed8`. The members below are the whole of the class:
 * everything recovered comes from them, and no other routine in the image refers to this type by
 * anything but its vtable. The fields through `+0x07` belong to MuseMsg and are declared there.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * Clone() copies only as far as `0xb` of the 0xc bytes it allocates, so the remaining 1 are
 * either alignment padding or a field the copy omits.
 *
 * The destructor at `0x003dbe80` is compiler-generated and has no declaration here.
 */
class StdMidiMsg : public MuseMsg {
public:
    /**
     * Construct a message with the song position at kMBTInfinity and the three bytes unset.
     *
     * Inline. New() and the Mixer's stack builds expand it. A declaration is required because the
     * class declares a second constructor.
     */
    StdMidiMsg() {
    }

    /**
     * Construct one channel message at a song position.
     *
     * Inline, with no address of its own. NotePlayer::PostStdMidiMsg() at `0x001b3fb8` expands it
     * on its stack, storing the position and then the three bytes in order.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param nStatus The status byte.
     * @param nData1 The first data byte.
     * @param nData2 The second data byte.
     */
    StdMidiMsg(int nTick, unsigned char nStatus, unsigned char nData1, unsigned char nData2)
        : MuseMsg(nTick), mUnknown08(nStatus), mUnknown09(nData1), mUnknown0a(nData2) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the song position is
     * initialised, by the MuseMsg constructor, and the three bytes are left unset.
     *
     * @return The message.
     * @ghidraAddress 0x003d6d40
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003dbfa0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_dwStdMidiMsgType.
     * @ghidraAddress 0x003dc010
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `StdMidiMsg`.
     * @ghidraAddress 0x003dc020
     */
    virtual const char *Name();

    /**
     * Write the message to a diagnostic stream.
     *
     * Writes the song position, a short label for the kind in the status byte's high nibble, both
     * data bytes as numbers, and the channel from its low nibble after ` n`. The labels are `off`,
     * `on`, `poly`, `ctl`, `prg`, `pres`, `pb`, and `sys` for the kinds 0x80 through 0xf0 in order.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003d7ec0
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the three bytes to a stream, one byte each through OBStream::WriteBytes().
     *
     * The song position is not written.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e3658
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the three bytes back in place, one byte each through IBStream::ReadBytes().
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003e36e8
     */
    virtual void Load(IBStream &stream);

    /**
     * The three bytes of one Standard MIDI channel message. +0x08, +0x09, and +0x0a
     *
     * Public because SynthSustainer reads the first two through a StdMidiMsg pointer from outside
     * the hierarchy at `0x001d20f8` and `0x001d2100`, and the image exposes no accessor. A friend
     * declaration fits equally well.
     *
     * The first byte is the status, a message kind in its high nibble over a channel in its low
     * one, which four inlined constructions in the sequencer confirm by composing it as a kind
     * combined with a channel. The two that follow are the data bytes, and their meaning depends
     * on that kind: a control-change status makes them a controller number and a value, while a
     * note status makes the first a note number. Two bands read them under those two different
     * titles and both readings were right for the status each had in view, which is why the
     * placeholders stand. A single title for either data byte would be wrong.
     */
    unsigned char mUnknown08;
    unsigned char mUnknown09;
    unsigned char mUnknown0a;
};

/**
 * Identity that StdMidiMsg::Type() reports.
 *
 * This word belongs to StdMidiMsg because StdMidiMsg::Type() at `0x003dc010` returns it. Several
 * handlers elsewhere read the same word to compare against it, which is the expected shape for a
 * registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d01c4
 */
extern unsigned int g_dwStdMidiMsgType;
