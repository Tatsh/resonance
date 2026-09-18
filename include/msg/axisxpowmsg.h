#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `11AxisXPowMsg` in the RTTI descriptor at `0x00901b20`, with Message as its one base. The
 * object is 0x10 bytes and its vtable is at `0x008131f0`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The class overrides Message::Print() at `0x003e3550`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class AxisXPowMsg : public Message {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003db060
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nAxisXPowMsgType.
     * @ghidraAddress 0x003db0b8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `AxisXPowMsg`.
     * @ghidraAddress 0x003db0c8
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
};

/**
 * Identity that AxisXPowMsg::Type() reports.
 *
 * This word belongs to AxisXPowMsg because AxisXPowMsg::Type() at `0x003db0b8` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d0164
 */
extern int g_nAxisXPowMsgType;
