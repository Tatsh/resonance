#pragma once

#include <iostream>

#include "gs/multimuse.h"
#include "msg/musemsg.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `12MultiMuseMsg` in the RTTI descriptor at `0x008eec98`, with MuseMsg as its one base. The object
 * is 0xc bytes and its vtable is at `0x00812df8`. Clone() delegates to the copy constructor at
 * `0x003e38a8` rather than copying inline, which is why this class is recovered alongside the
 * packets rather than with its three MuseMsg siblings.
 *
 * The word at `+0x08` is what settles MuseMsg's payload. This class copies a word there while
 * NoteMsg and StdMidiMsg copy bytes, and a single word store cannot straddle a base and a derived
 * class in a member-wise copy, so `+0x08` belongs to each derived class rather than to MuseMsg.
 *
 * That word is a MultiMuse, and the class is how a sequence of timed messages moves between a
 * MsgSource and a MsgSink. The copy constructor at `0x003e38a8` retains it, the destructor
 * releases it, Print() writes it through MultiMuse::Print(), Save() writes it through
 * MultiMuse::SaveFields(), and Load() replaces it with a freshly allocated sequence.
 *
 * Print() also settles MuseMsg's own member. It copies that word into a temporary and writes the
 * temporary through Mid::MBT::Print(), so MuseMsg's payload is a song position. Both this class
 * and StdMidiMsg initialise it to `0x2aaaaaab`, which Mid::MBT documents as inside its positive
 * infinity range.
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
     * @ghidraAddress 0x003d6e08
     */
    static MultiMuseMsg *New();

    /**
     * @param pMuse The sequence, retained when it is not null. It is stored either way.
     * @ghidraAddress 0x003e38e8
     */
    MultiMuseMsg(MultiMuse *pMuse);

    /**
     * @param other The message to copy.
     * @ghidraAddress 0x003e38a8
     */
    MultiMuseMsg(const MultiMuseMsg &other);

    /**
     * @ghidraAddress 0x003e3920
     */
    virtual ~MultiMuseMsg();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003dc590
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_dwMultiMuseMsgType.
     * @ghidraAddress 0x003dc608
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `MultiMuseMsg`.
     * @ghidraAddress 0x003dc618
     */
    virtual const char *Name();

    /**
     * Write the message to a diagnostic stream.
     *
     * Writes the inherited song position, a literal, and then the sequence.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e3990
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the sequence to an output stream.
     *
     * The inherited song position is not written.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e39f8
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the sequence back from an input stream.
     *
     * Allocates a fresh MultiMuse with one reference and reads into it. Whatever sequence the
     * message already stored is replaced without being released, which is faithful and leaks that
     * sequence when the message already had one.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003d8180
     */
    virtual void Load(IBStream &stream);

private:
    MultiMuse *mMuse; // +0x08
};

/**
 * Identity that MultiMuseMsg::Type() reports.
 *
 * This word belongs to MultiMuseMsg because MultiMuseMsg::Type() at `0x003dc608` returns it. The
 * program still titles it `g_dwMsgIdAddLightPoint`, which describes a handler comparing against it
 * rather than the class reporting it, so the two disagree until that label is corrected.
 *
 * @ghidraAddress 0x006d01dc
 */
extern unsigned int g_dwMultiMuseMsgType;
