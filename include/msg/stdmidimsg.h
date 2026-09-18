#pragma once

#include "msg/musemsg.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `10StdMidiMsg` in the RTTI descriptor at `0x008ef3f0`, with MuseMsg as its one base. The object
 * is 0xc bytes and its vtable is at `0x00812ed8`. The members below are the whole of the class:
 * everything recovered comes from them, and no other routine in the image refers to this type by
 * anything but its vtable. The fields through `+0x08` belong to MuseMsg and are declared there.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * Clone() copies only as far as `0xb` of the 0xc bytes it allocates, so the remaining 1 are
 * either alignment padding or a field the copy omits.
 *
 * The class overrides Message::Print() at `0x003d7ec0`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class StdMidiMsg : public MuseMsg {
public:
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
     * @return g_dwMsgIdMidiMsg.
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

private:
    unsigned char mUnknown09; // +0x09
    unsigned char mUnknown0a; // +0x0a
};

/**
 * Identity that StdMidiMsg::Type() reports.
 *
 * This word was already titled `g_dwMsgIdMidiMsg` in the program by another subsystem, and the
 * reconstruction follows that name rather than imposing its own. The two disagree:
 * StdMidiMsg::Type() is the only routine that reads the word, so the existing title describes a
 * handler that compares against it rather than the class that reports it, and it is likely wrong.
 *
 * @ghidraAddress 0x006d01c4
 */
extern unsigned int g_dwMsgIdMidiMsg;
