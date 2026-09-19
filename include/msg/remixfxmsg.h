#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `10RemixFXMsg` in the RTTI descriptor at `0x008ef640`, with Message as its one base. The object
 * is 0x18 bytes and its vtable is at `0x00812b28`. The allocation in New() and the allocation in
 * Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 */
class RemixFXMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 308.
     *
     * @return The message.
     * @ghidraAddress 0x003d7070
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x001a6250
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nRemixFXMsgType.
     * @ghidraAddress 0x001a62b8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `RemixFXMsg`.
     * @ghidraAddress 0x001a62c8
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
    int mUnknown10; // +0x10
    int mUnknown14; // +0x14
};

/**
 * Identity that RemixFXMsg::Type() reports.
 *
 * This word belongs to RemixFXMsg because RemixFXMsg::Type() at `0x001a62b8` returns it, and the
 * registration at `0x003d9818` passes the same value, 308, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d022c
 */
extern int g_nRemixFXMsgType;
