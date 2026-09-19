#pragma once

#include "msg/message.h"
#include "os/hxstr.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `7TextMsg` in the RTTI descriptor at `0x008ef7b0`, with Message as its one base. The object is
 * 0x10 bytes and its vtable is at `0x007dc478`. The allocation in New() and the allocation in
 * Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The HxStr at `+0x04` comes from the copy constructor at `0x00193e10`, which copy-constructs it
 * rather than copying its words.
 */
class TextMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 311.
     *
     * @return The message.
     * @ghidraAddress 0x003d7118
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x00193e10
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nTextMsgType.
     * @ghidraAddress 0x00193eb8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `TextMsg`.
     * @ghidraAddress 0x00193ec8
     */
    virtual const char *Name();

private:
    HxStr mUnknown04; // +0x04
    int mUnknown0c;   // +0x0c
};

/**
 * Identity that TextMsg::Type() reports.
 *
 * This word belongs to TextMsg because TextMsg::Type() at `0x00193eb8` returns it, and the
 * registration at `0x003d9818` passes the same value, 311, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d0244
 */
extern int g_nTextMsgType;
