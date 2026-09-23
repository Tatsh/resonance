#pragma once

#include <iostream>

#include "msg/musemsg.h"

class IBStream;
class OBStream;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `14SustainNoteMsg` in the RTTI descriptor at `0x00901e20`, with MuseMsg as its one base. The
 * object is 0xc bytes and its vtable is at `0x00812db0`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable. The fields through `+0x07` belong to MuseMsg and are declared
 * there.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * Clone() copies only as far as `0x9` of the 0xc bytes it allocates, so the remaining 3 are
 * either alignment padding or a field the copy omits.
 *
 * The destructor at `0x003dc658` is compiler-generated and has no declaration here.
 */
class SustainNoteMsg : public MuseMsg {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the song position is
     * initialised, by the MuseMsg constructor.
     *
     * @return The message.
     * @ghidraAddress 0x003d6e50
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003dc778
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_dwSustainNoteMsgType.
     * @ghidraAddress 0x003dc7d8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `SustainNoteMsg`.
     * @ghidraAddress 0x003dc7e8
     */
    virtual const char *Name();

    /**
     * Write the song position and the byte, as a character, to a diagnostic stream.
     *
     * The byte is loaded sign-extended and written through the character inserter rather than the
     * integer one, so a note number appears as the character with that code.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e3a18
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the byte to a stream through OBStream::WriteBytes().
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e3a70
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the byte back in place through IBStream::ReadBytes().
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003e3ab0
     */
    virtual void Load(IBStream &stream);

    /**
     * Undetermined, and the one byte the message carries. +0x08
     *
     * Public because SynthSustainer::HandleSustainNote() at `0x001d20a0` reads it through a
     * SustainNoteMsg pointer from outside the hierarchy, searching two held-note lists for it and
     * appending it to one. The image exposes no accessor. A friend declaration fits equally well.
     * The value is a note number on that evidence, but nothing in the image titles it, so the
     * placeholder stands.
     */
    unsigned char mUnknown08;
};

/**
 * Identity that SustainNoteMsg::Type() reports.
 *
 * This word belongs to SustainNoteMsg because SustainNoteMsg::Type() at `0x003dc7d8` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d01e4
 */
extern unsigned int g_dwSustainNoteMsgType;
