#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `11ClearGemMsg` in the RTTI descriptor at `0x008ef7f0`, with Message as its one base. The object
 * is 0x10 bytes and its vtable is at `0x007e2818`. The allocation in New() and the allocation in
 * Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 */
class ClearGemMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 404.
     *
     * @return The message.
     * @ghidraAddress 0x003d74b8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x001bf8e0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nClearGemMsgType.
     * @ghidraAddress 0x001bf938
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `ClearGemMsg`.
     * @ghidraAddress 0x001bf948
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
};

/**
 * Identity that ClearGemMsg::Type() reports.
 *
 * This word belongs to ClearGemMsg because ClearGemMsg::Type() at `0x001bf938` returns it, and the
 * registration at `0x003d9818` passes the same value, 404, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d02c4
 */
extern int g_nClearGemMsgType;
