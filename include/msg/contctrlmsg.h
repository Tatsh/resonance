#pragma once

#include "app/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `11ContCtrlMsg` in the RTTI descriptor at `0x008efd00`, with Message as its one base. The
 * object is 0x10 bytes and its vtable is at `0x00812420`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The class overrides Message::Print() at `0x003e3e80`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class ContCtrlMsg : public Message {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003dfc28
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nContCtrlMsgType.
     * @ghidraAddress 0x003dfc80
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `ContCtrlMsg`.
     * @ghidraAddress 0x003dfc90
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
};

/**
 * Identity that ContCtrlMsg::Type() reports.
 *
 * @ghidraAddress 0x006d02f4
 */
extern int g_nContCtrlMsgType;
