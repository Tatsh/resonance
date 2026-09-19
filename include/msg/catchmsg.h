#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `8CatchMsg` in the RTTI descriptor at `0x00901e00`, with Message as its one base. The object is
 * 0x20 bytes and its vtable is at `0x007e0a28`. The allocation in New() and the allocation in
 * Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 */
class CatchMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 408.
     *
     * @return The message.
     * @ghidraAddress 0x003d75c0
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x001b1068
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nCatchMsgType.
     * @ghidraAddress 0x001b10e0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `CatchMsg`.
     * @ghidraAddress 0x001b10f0
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
    int mUnknown10; // +0x10
    int mUnknown14; // +0x14
    int mUnknown18; // +0x18
    int mUnknown1c; // +0x1c
};

/**
 * Identity that CatchMsg::Type() reports.
 *
 * This word belongs to CatchMsg because CatchMsg::Type() at `0x001b10e0` returns it, and the
 * registration at `0x003d9818` passes the same value, 408, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d02e4
 */
extern int g_nCatchMsgType;
