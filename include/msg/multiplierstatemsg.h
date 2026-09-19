#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `18MultiplierStateMsg` in the RTTI descriptor at `0x008ef100`, with Message as its one base. The
 * object is 0x10 bytes and its vtable is at `0x007cffa8`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 */
class MultiplierStateMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 117.
     *
     * @return The message.
     * @ghidraAddress 0x003d6c98
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x00122610
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nMultiplierStateMsgType.
     * @ghidraAddress 0x00122668
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `MultiplierStateMsg`.
     * @ghidraAddress 0x00122678
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
};

/**
 * Identity that MultiplierStateMsg::Type() reports.
 *
 * This word belongs to MultiplierStateMsg because MultiplierStateMsg::Type() at `0x00122668`
 * returns it, and the registration at `0x003d9818` passes the same value, 117, as the identity of
 * this class's factory.
 *
 * @ghidraAddress 0x006d01a4
 */
extern int g_nMultiplierStateMsgType;
