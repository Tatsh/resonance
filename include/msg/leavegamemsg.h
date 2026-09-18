#pragma once

#include "app/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `12LeaveGameMsg` in the RTTI descriptor at `0x008ef730`, with Message as its one base. The
 * object is 0x4 bytes and its vtable is at `0x00812078`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 */
class LeaveGameMsg : public Message {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e0f80
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nLeaveGameMsgType.
     * @ghidraAddress 0x003e0fb8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `LeaveGameMsg`.
     * @ghidraAddress 0x003e0fc8
     */
    virtual const char *Name();
};

/**
 * Identity that LeaveGameMsg::Type() reports.
 *
 * @ghidraAddress 0x006d035c
 */
extern int g_nLeaveGameMsgType;
