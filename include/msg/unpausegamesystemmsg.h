#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `20UnpauseGameSystemMsg` in the RTTI descriptor at `0x00901b60`, with Message as its one base.
 * The object is 0x4 bytes and its vtable is at `0x00801c80`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * This class adds no field of its own, and the size is exactly the size of the base, which is what
 * fixes the size of the base.
 */
class UnpauseGameSystemMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 435.
     *
     * @return The message.
     * @ghidraAddress 0x003d7bf8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x00311c68
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nUnpauseGameSystemMsgType.
     * @ghidraAddress 0x00311ca0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `UnpauseGameSystemMsg`.
     * @ghidraAddress 0x00311cb0
     */
    virtual const char *Name();
};

/**
 * Identity that UnpauseGameSystemMsg::Type() reports.
 *
 * This word belongs to UnpauseGameSystemMsg because UnpauseGameSystemMsg::Type() at `0x00311ca0`
 * returns it, and the registration at `0x003d9818` passes the same value, 435, as the identity of
 * this class's factory.
 *
 * @ghidraAddress 0x006d03bc
 */
extern int g_nUnpauseGameSystemMsgType;
