#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `17AdvanceSectionMsg` in the RTTI descriptor at `0x008eed08`, with Message as its one base. The
 * object is 0x10 bytes and its vtable is at `0x007cf530`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 */
class AdvanceSectionMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 113.
     *
     * @return The message.
     * @ghidraAddress 0x003d6ba8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x0011d698
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nAdvanceSectionMsgType.
     * @ghidraAddress 0x0011d6f0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `AdvanceSectionMsg`.
     * @ghidraAddress 0x0011d700
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
};

/**
 * Identity that AdvanceSectionMsg::Type() reports.
 *
 * This word belongs to AdvanceSectionMsg because AdvanceSectionMsg::Type() at `0x0011d6f0` returns
 * it, and the registration at `0x003d9818` passes the same value, 113, as the identity of this
 * class's factory.
 *
 * @ghidraAddress 0x006d0184
 */
extern int g_nAdvanceSectionMsgType;
