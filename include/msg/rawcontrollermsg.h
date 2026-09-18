#pragma once

#include "app/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `16RawControllerMsg` in the RTTI descriptor at `0x008efce0`, with Message as its one base. The
 * object is 0x18 bytes and its vtable is at `0x00813478`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The class overrides Message::Print() at `0x003e2fb0`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class RawControllerMsg : public Message {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003da200
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nRawControllerMsgType.
     * @ghidraAddress 0x003da268
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `RawControllerMsg`.
     * @ghidraAddress 0x003da278
     */
    virtual const char *Name();

private:
    int mUnknown14; // +0x14
};

/**
 * Identity that RawControllerMsg::Type() reports.
 *
 * @ghidraAddress 0x006d0118
 */
extern int g_nRawControllerMsgType;
