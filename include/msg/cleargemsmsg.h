#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `12ClearGemsMsg` in the RTTI descriptor at `0x00901de0`, with Message as its one base. The
 * object is 0xc bytes and its vtable is at `0x007ddb98`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 */
class ClearGemsMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 403.
     *
     * @return The message.
     * @ghidraAddress 0x003d7480
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x0019d590
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nClearGemsMsgType.
     * @ghidraAddress 0x0019d5e0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `ClearGemsMsg`.
     * @ghidraAddress 0x0019d5f0
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
};

/**
 * Identity that ClearGemsMsg::Type() reports.
 *
 * This word belongs to ClearGemsMsg because ClearGemsMsg::Type() at `0x0019d5e0` returns it, and
 * the registration at `0x003d9818` passes the same value, 403, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d02bc
 */
extern int g_nClearGemsMsgType;
