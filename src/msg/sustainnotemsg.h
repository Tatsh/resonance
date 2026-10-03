#pragma once

#include <iostream>

#include "msg/musemsg.h"

class IBStream;
class OBStream;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x00901e20`. It has MuseMsg as its one base. The object is 0xc bytes
 * and its vtable is at `0x00812db0`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable. The fields through
 * `+0x07` belong to MuseMsg and are declared there.
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
     * Identity that Type() reports.
     *
     * @ghidraAddress NTSC-U/C: 0x006d01e4
     * @ghidraAddress PAL: 0x0071397c
     */
    static unsigned int sID;

    /**
     * Construct a message with the song position at kMBTInfinity and the byte unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    SustainNoteMsg() {
    }

    /**
     * Report a sustained note at a song position.
     *
     * Inline, with no address of its own. PitchPicker expands it on its stack at `0x001c2d70`,
     * storing the position and then the byte its routine at `0x001c3060` returns.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param nNote The byte at `+0x08`.
     */
    SustainNoteMsg(int nTick, unsigned char nNote) : MuseMsg(nTick), mNote(nNote) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the song position is
     * initialised, by the MuseMsg constructor.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d6e50
     * @ghidraAddress PAL: 0x0040ed40
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003dc778
     * @ghidraAddress PAL: 0x00414bb0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return sID.
     * @ghidraAddress NTSC-U/C: 0x003dc7d8
     * @ghidraAddress PAL: 0x00414c10
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `SustainNoteMsg`.
     * @ghidraAddress NTSC-U/C: 0x003dc7e8
     * @ghidraAddress PAL: 0x00414c20
     */
    virtual const char *GetName() const;

    /**
     * Write the song position and the byte, as a character, to a diagnostic stream.
     *
     * The byte is loaded sign-extended and written through the character inserter rather than the
     * integer one, so a note number appears as the character with that code.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e3a18
     * @ghidraAddress PAL: 0x0041bdb8
     */
    virtual void PrintExtra(std::ostream &stream) const;

    /**
     * Write the byte to a stream through OBStream::Write().
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e3a70
     * @ghidraAddress PAL: 0x0041be10
     */
    virtual void saveGuts(OBStream &stream) const;

    /**
     * Read the byte back in place through IBStream::Read().
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x003e3ab0
     * @ghidraAddress PAL: 0x0041be50
     */
    virtual void restoreGuts(IBStream &stream);

    /**
     * The note number, and the one byte the message includes. +0x08
     *
     * Public because SynthSustainer::HandleSustainNote() at `0x001d20a0` reads it through a
     * SustainNoteMsg pointer from outside the hierarchy, searching two held-note lists for it and
     * appending it to one. The image exposes no accessor. A friend declaration fits equally well.
     * That use is the evidence for the title. No string in the image supplies one.
     */
    unsigned char mNote;
};
