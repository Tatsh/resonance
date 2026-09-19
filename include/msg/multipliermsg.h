#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `13MultiplierMsg` in the RTTI descriptor at `0x008eec78`, with Message as its one base. The
 * object is 0x10 bytes and its vtable is at `0x007e44e0`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 */
class MultiplierMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 116.
     *
     * @return The message.
     * @ghidraAddress 0x003d6c60
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x001cab30
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nMultiplierMsgType.
     * @ghidraAddress 0x001cab88
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `MultiplierMsg`.
     * @ghidraAddress 0x001cab98
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
};

/**
 * Identity that MultiplierMsg::Type() reports.
 *
 * This word belongs to MultiplierMsg because MultiplierMsg::Type() at `0x001cab88` returns it, and
 * the registration at `0x003d9818` passes the same value, 116, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d019c
 */
extern int g_nMultiplierMsgType;
