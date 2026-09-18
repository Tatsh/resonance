#pragma once

#include "msg/cmdmsg.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `13NeutralizeMsg` in the RTTI descriptor at `0x008eecb8`, with CmdMsg as its one base. The
 * object is 0x14 bytes and its vtable is at `0x008122b8`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable. The fields through `+0x0f` belong to CmdMsg and are declared
 * there.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The class overrides Message::Print() at `0x003e3ea8`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class NeutralizeMsg : public CmdMsg {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e03a8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nNeutralizeMsgType.
     * @ghidraAddress 0x003e0418
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `NeutralizeMsg`.
     * @ghidraAddress 0x003e0428
     */
    virtual const char *Name();

private:
    int mUnknown10; // +0x10
};

/**
 * Identity that NeutralizeMsg::Type() reports.
 *
 * @ghidraAddress 0x006d031c
 */
extern int g_nNeutralizeMsgType;
