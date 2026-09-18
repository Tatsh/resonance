#pragma once

#include "msg/cmdmsg.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `12AutoCatchMsg` in the RTTI descriptor at `0x008f0010`, with CmdMsg as its one base. The
 * object is 0x14 bytes and its vtable is at `0x00811c88`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable. The fields through `+0x0f` belong to CmdMsg and are declared
 * there.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The class overrides Message::Print() at `0x003e4208`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class AutoCatchMsg : public CmdMsg {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e2580
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nAutoCatchMsgType.
     * @ghidraAddress 0x003e25f0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `AutoCatchMsg`.
     * @ghidraAddress 0x003e2600
     */
    virtual const char *Name();

private:
    int mUnknown10; // +0x10
};

/**
 * Identity that AutoCatchMsg::Type() reports.
 *
 * @ghidraAddress 0x006d03cc
 */
extern int g_nAutoCatchMsgType;
