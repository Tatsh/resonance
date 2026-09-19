#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `12AxeButtonMsg` in the RTTI descriptor at `0x008ef450`, with Message as its one base. The
 * object is 0x10 bytes and its vtable is at `0x007dd3f0`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 */
class AxeButtonMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 412.
     *
     * @return The message.
     * @ghidraAddress 0x003d76b8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x0019a748
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nAxeButtonMsgType.
     * @ghidraAddress 0x0019a7a0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `AxeButtonMsg`.
     * @ghidraAddress 0x0019a7b0
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
};

/**
 * Identity that AxeButtonMsg::Type() reports.
 *
 * This word belongs to AxeButtonMsg because AxeButtonMsg::Type() at `0x0019a7a0` returns it, and
 * the registration at `0x003d9818` passes the same value, 412, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d0304
 */
extern int g_nAxeButtonMsgType;
