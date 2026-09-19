#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `23AdvanceSectionToggleMsg` in the RTTI descriptor at `0x008ef7a0`, with Message as its one
 * base. The object is 0xc bytes and its vtable is at `0x007ce768`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 */
class AdvanceSectionToggleMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 314.
     *
     * @return The message.
     * @ghidraAddress 0x003d71d0
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x00115f90
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nAdvanceSectionToggleMsgType.
     * @ghidraAddress 0x00115fe0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `AdvanceSectionToggleMsg`.
     * @ghidraAddress 0x00115ff0
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
};

/**
 * Identity that AdvanceSectionToggleMsg::Type() reports.
 *
 * This word belongs to AdvanceSectionToggleMsg because AdvanceSectionToggleMsg::Type() at
 * `0x00115fe0` returns it, and the registration at `0x003d9818` passes the same value, 314, as the
 * identity of this class's factory.
 *
 * @ghidraAddress 0x006d025c
 */
extern int g_nAdvanceSectionToggleMsgType;
