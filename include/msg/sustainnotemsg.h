#pragma once

#include "msg/musemsg.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `14SustainNoteMsg` in the RTTI descriptor at `0x00901e20`, with MuseMsg as its one base. The
 * object is 0xc bytes and its vtable is at `0x00812db0`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable. The fields through `+0x08` belong to MuseMsg and are declared
 * there.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * Clone() copies only as far as `0x4` of the 0xc bytes it allocates, so the remaining 8 are
 * either alignment padding or a field the copy omits.
 *
 * The class overrides Message::Print() at `0x003e3a18`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class SustainNoteMsg : public MuseMsg {
public:
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
     * @return g_dwMsgIdMidiMsgAlt.
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
};

/**
 * Identity that SustainNoteMsg::Type() reports.
 *
 * This word was already titled `g_dwMsgIdMidiMsgAlt` in the program by another subsystem, and the
 * reconstruction follows that name rather than imposing its own. The two disagree:
 * SustainNoteMsg::Type() is the only routine that reads the word, so the existing title describes
 * a handler that compares against it rather than the class that reports it, and it is likely
 * wrong.
 *
 * @ghidraAddress 0x006d01e4
 */
extern unsigned int g_dwMsgIdMidiMsgAlt;
