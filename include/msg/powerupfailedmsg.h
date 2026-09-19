#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `16PowerupFailedMsg` in the RTTI descriptor at `0x008ef340`, with Message as its one base. The
 * object is 0xc bytes and its vtable is at `0x007e45b8`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 */
class PowerupFailedMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 307.
     *
     * @return The message.
     * @ghidraAddress 0x003d7038
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x001ca778
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPowerupFailedMsgType.
     * @ghidraAddress 0x001ca7c8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PowerupFailedMsg`.
     * @ghidraAddress 0x001ca7d8
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
};

/**
 * Identity that PowerupFailedMsg::Type() reports.
 *
 * This word belongs to PowerupFailedMsg because PowerupFailedMsg::Type() at `0x001ca7c8` returns
 * it, and the registration at `0x003d9818` passes the same value, 307, as the identity of this
 * class's factory.
 *
 * @ghidraAddress 0x006d0224
 */
extern int g_nPowerupFailedMsgType;
