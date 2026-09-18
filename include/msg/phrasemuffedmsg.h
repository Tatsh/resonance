#pragma once

#include "app/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `15PhraseMuffedMsg` in the RTTI descriptor at `0x00902280`, with Message as its one base. The
 * object is 0x14 bytes and its vtable is at `0x00812348`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The class overrides Message::Print() at `0x003e4350`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class PhraseMuffedMsg : public Message {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e0038
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPhraseMuffedMsgType.
     * @ghidraAddress 0x003e0098
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PhraseMuffedMsg`.
     * @ghidraAddress 0x003e00a8
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
    int mUnknown10; // +0x10
};

/**
 * Identity that PhraseMuffedMsg::Type() reports.
 *
 * @ghidraAddress 0x006d030c
 */
extern int g_nPhraseMuffedMsgType;
