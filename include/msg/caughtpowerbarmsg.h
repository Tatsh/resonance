#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `17CaughtPowerbarMsg` in the RTTI descriptor at `0x008eeca8`, with Message as its one base. The
 * object is 0xc bytes and its vtable is at `0x00812c90`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The destructor at `0x003dce20` is compiler-generated and has no declaration here.
 */
class CaughtPowerbarMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. The payload is left unset.
     *
     * @return The message.
     * @ghidraAddress 0x003d6f58
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003dcf10
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nCaughtPowerbarMsgType.
     * @ghidraAddress 0x003dcf60
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `CaughtPowerbarMsg`.
     * @ghidraAddress 0x003dcf70
     */
    virtual const char *Name();

    /**
     * Write the word at `+0x04` to a diagnostic stream as a number.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e3cb8
     */
    virtual void Print(std::ostream &stream);

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
};

/**
 * Identity that CaughtPowerbarMsg::Type() reports.
 *
 * This word belongs to CaughtPowerbarMsg because CaughtPowerbarMsg::Type() at `0x003dcf60`
 * returns it. Several handlers elsewhere read the same word to compare against it, which is the
 * expected shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d0204
 */
extern int g_nCaughtPowerbarMsgType;
