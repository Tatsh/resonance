#pragma once

#include <iostream>

#include "msg/musemsg.h"

class IBStream;
class OBStream;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008ef3f0`. It has MuseMsg as its one base. The object is 0xc bytes
 * and its vtable is at `0x00812ed8`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable. The fields through
 * `+0x07` belong to MuseMsg and are declared there.
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
     * Identity that Type() reports.
     *
     * @ghidraAddress NTSC-U/C: 0x006d01c4
     * @ghidraAddress PAL: 0x0071395c
     */
    static unsigned int sID;

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
     * Inline, with no address of its own. NotePlayer::NoteOn() at `0x001b3fb8` expands it
     * on its stack, storing the position and then the three bytes in order.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param nStatus The status byte.
     * @param nData1 The first data byte.
     * @param nData2 The second data byte.
     */
    StdMidiMsg(int nTick, unsigned char nStatus, unsigned char nData1, unsigned char nData2)
        : MuseMsg(nTick), mStatus(nStatus), mData1(nData1), mData2(nData2) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the song position is
     * initialised, by the MuseMsg constructor, and the three bytes are left unset.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d6d40
     * @ghidraAddress PAL: 0x0040ec30
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003dbfa0
     * @ghidraAddress PAL: 0x004143d8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return sID.
     * @ghidraAddress NTSC-U/C: 0x003dc010
     * @ghidraAddress PAL: 0x00414448
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `StdMidiMsg`.
     * @ghidraAddress NTSC-U/C: 0x003dc020
     * @ghidraAddress PAL: 0x00414458
     */
    virtual const char *GetName() const;

    /**
     * Write the message to a diagnostic stream.
     *
     * Writes the song position, a short label for the kind in the status byte's high nibble, both
     * data bytes as numbers, and the channel from its low nibble after ` n`. The labels are `off`,
     * `on`, `poly`, `ctl`, `prg`, `pres`, `pb`, and `sys` for the kinds 0x80 through 0xf0 in order.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003d7ec0
     * @ghidraAddress PAL: 0x0040fff8
     */
    virtual void PrintExtra(std::ostream &stream) const;

    /**
     * Write the three bytes to a stream, one byte each through OBStream::Write().
     *
     * The song position is not written.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e3658
     * @ghidraAddress PAL: 0x0041b9f8
     */
    virtual void saveGuts(OBStream &stream) const;

    /**
     * Read the three bytes back in place, one byte each through IBStream::Read().
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x003e36e8
     * @ghidraAddress PAL: 0x0041ba88
     */
    virtual void restoreGuts(IBStream &stream);

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
     * note status makes the first a note number. Their titles use the MIDI specification's terms,
     * the first and second data bytes, because a single meaning for either would be wrong.
     */
    unsigned char mStatus;
    unsigned char mData1;
    unsigned char mData2;
};
