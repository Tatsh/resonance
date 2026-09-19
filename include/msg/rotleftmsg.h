#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `10RotLeftMsg` in the RTTI descriptor at `0x008eec58`, with Message as its one base. The object
 * is 0xc bytes and its vtable is at `0x007cf608`. The allocation in New() and the allocation in
 * Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 */
class RotLeftMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 101.
     *
     * @return The message.
     * @ghidraAddress 0x003d68a8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x0011d338
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nRotLeftMsgType.
     * @ghidraAddress 0x0011d388
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `RotLeftMsg`.
     * @ghidraAddress 0x0011d398
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
};

/**
 * Identity that RotLeftMsg::Type() reports.
 *
 * This word belongs to RotLeftMsg because RotLeftMsg::Type() at `0x0011d388` returns it, and the
 * registration at `0x003d9818` passes the same value, 101, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d0124
 */
extern int g_nRotLeftMsgType;
