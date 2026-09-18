#pragma once

#include "app/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `12BarStatusMsg` in the RTTI descriptor at `0x008ef420`, with Message as its one base. The
 * object is 0x30 bytes and its vtable is at `0x00812660`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * Clone() copies only as far as `0x2c` of the 0x30 bytes it allocates, so the remaining 4 are
 * either alignment padding or a field the copy omits.
 *
 * The class overrides Message::Print() at `0x003d8670`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class BarStatusMsg : public Message {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003defd0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nBarStatusMsgType.
     * @ghidraAddress 0x003df050
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `BarStatusMsg`.
     * @ghidraAddress 0x003df060
     */
    virtual const char *Name();

private:
    int mUnknown04;       // +0x04
    int mUnknown08;       // +0x08
    int mUnknown0c;       // +0x0c
    int mUnknown10;       // +0x10
    int mUnknown14;       // +0x14
    int mUnknown18;       // +0x18
    long long mUnknown20; // +0x20
    int mUnknown28;       // +0x28
};

/**
 * Identity that BarStatusMsg::Type() reports.
 *
 * @ghidraAddress 0x006d02b4
 */
extern int g_nBarStatusMsgType;
