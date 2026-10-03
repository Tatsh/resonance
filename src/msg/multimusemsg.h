#pragma once

#include <iostream>

#include "gs/multimuse.h"
#include "msg/musemsg.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008eec98`. It has MuseMsg as its one base. The object is 0xc bytes
 * and its vtable is at `0x00812df8`. Clone() delegates to the copy constructor at `0x003e38a8`
 * rather than copying inline.
 *
 * The word at `+0x08` is what settles MuseMsg's payload. This class copies a word there while
 * NoteMsg and StdMidiMsg copy bytes, and a single word store cannot straddle a base and a derived
 * class in a member-wise copy, so `+0x08` belongs to each derived class rather than to MuseMsg.
 *
 * That word is a MultiMuse, and the class is how a sequence of timed messages moves between a
 * MsgSource and a MsgSink. The copy constructor at `0x003e38a8` retains it, the destructor
 * releases it, PrintExtra() writes it through MultiMuse::Print(), saveGuts() writes it through
 * MultiMuse::SaveFields(), and restoreGuts() replaces it with a freshly allocated sequence.
 *
 * PrintExtra() also settles MuseMsg's own member. It copies that word into a temporary and writes
 * the temporary through Mid::MBT::Print(), so MuseMsg's payload is a song position. Both this
 * class and StdMidiMsg initialise it to `0x2aaaaaab`, which Mid::MBT documents as inside its
 * positive infinity range.
 */
class MultiMuseMsg : public MuseMsg {
public:
    /**
     * Produce a message with no sequence.
     *
     * New() takes 0xc bytes against the tag `MultiMuseMsg` and calls this with a null sequence,
     * and the registration table at `0x003d9818` records it as this class's factory.
     *
     * @return The new message.
     * @ghidraAddress NTSC-U/C: 0x003d6e08
     * @ghidraAddress PAL: 0x0040ecf8
     */
    static Message *New();

    /**
     * @param pMuse The sequence, retained when it is not null. It is stored either way.
     * @ghidraAddress NTSC-U/C: 0x003e38e8
     * @ghidraAddress PAL: 0x0041bc88
     */
    MultiMuseMsg(MultiMuse *pMuse);

    /**
     * @param other The message to copy.
     * @ghidraAddress NTSC-U/C: 0x003e38a8
     * @ghidraAddress PAL: 0x0041bc48
     */
    MultiMuseMsg(const MultiMuseMsg &other);

    /**
     * @ghidraAddress NTSC-U/C: 0x003e3920
     * @ghidraAddress PAL: 0x0041bcc0
     */
    virtual ~MultiMuseMsg();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003dc590
     * @ghidraAddress PAL: 0x004149c8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_dwMultiMuseMsgType.
     * @ghidraAddress NTSC-U/C: 0x003dc608
     * @ghidraAddress PAL: 0x00414a40
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `MultiMuseMsg`.
     * @ghidraAddress NTSC-U/C: 0x003dc618
     * @ghidraAddress PAL: 0x00414a50
     */
    virtual const char *GetName() const;

    /**
     * Write the message to a diagnostic stream.
     *
     * Writes the inherited song position, a literal, and then the sequence.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e3990
     * @ghidraAddress PAL: 0x0041bd30
     */
    virtual void PrintExtra(std::ostream &stream) const;

    /**
     * Write the sequence to an output stream.
     *
     * The inherited song position is not written.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e39f8
     * @ghidraAddress PAL: 0x0041bd98
     */
    virtual void saveGuts(OBStream &stream) const;

    /**
     * Read the sequence back from an input stream.
     *
     * Allocates a fresh MultiMuse with one reference and reads into it. Whatever sequence the
     * message already stored is replaced without being released, which is faithful and leaks that
     * sequence when the message already had one.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x003d8180
     * @ghidraAddress PAL: 0x004102e8
     */
    virtual void restoreGuts(IBStream &stream);

    /**
     * The sequence. +0x08
     *
     * Public because PitchPicker::OnMsg() at `0x001c2c84` reads it directly with no
     * accessor in the image.
     */
    MultiMuse *mMuse;
};

/**
 * Identity that MultiMuseMsg::Type() reports.
 *
 * This word belongs to MultiMuseMsg because MultiMuseMsg::Type() at `0x003dc608` returns it.
 *
 * @ghidraAddress NTSC-U/C: 0x006d01dc
 * @ghidraAddress PAL: 0x00713974
 */
extern unsigned int g_dwMultiMuseMsgType;
