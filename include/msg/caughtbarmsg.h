#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `12CaughtBarMsg` in the RTTI descriptor at `0x008f07a0`, with Message as its one base. The
 * object is 0xc bytes and its vtable is at `0x007e09e0`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 */
class CaughtBarMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 119.
     *
     * @return The message.
     * @ghidraAddress 0x003d6d08
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x001b11b0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nCaughtBarMsgType.
     * @ghidraAddress 0x001b1200
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `CaughtBarMsg`.
     * @ghidraAddress 0x001b1210
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
};

/**
 * Identity that CaughtBarMsg::Type() reports.
 *
 * This word belongs to CaughtBarMsg because CaughtBarMsg::Type() at `0x001b1200` returns it, and
 * the registration at `0x003d9818` passes the same value, 119, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d01b4
 */
extern int g_nCaughtBarMsgType;
