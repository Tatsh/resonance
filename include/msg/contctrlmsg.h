#pragma once

#include <iostream>

#include "mid/mbt.h"
#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `11ContCtrlMsg` in the RTTI descriptor at `0x008efd00`, with Message as its one base. The
 * object is 0x10 bytes and its vtable is at `0x00812420`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). New() initialises `+0x04` to
 * kMBTInfinity, the one store it makes, which marks that word as a position. Print() writes only
 * the word at `+0x0c`. Readers of the fields have not been traced, so they are private by default.
 *
 * The destructor at `0x003dfb38` is compiler-generated and has no declaration here.
 */
class ContCtrlMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the position is
     * initialised.
     *
     * @return The message.
     * @ghidraAddress 0x003d7640
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003dfc28
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nContCtrlMsgType.
     * @ghidraAddress 0x003dfc80
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `ContCtrlMsg`.
     * @ghidraAddress 0x003dfc90
     */
    virtual const char *Name();

    /**
     * Write the word at `+0x0c` to a diagnostic stream as a number.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e3e80
     */
    virtual void Print(std::ostream &stream);

private:
    Mid::MBT mUnknown04; // +0x04
    int mUnknown08;      // +0x08
    int mUnknown0c;      // +0x0c
};

/**
 * Identity that ContCtrlMsg::Type() reports.
 *
 * This word belongs to ContCtrlMsg because ContCtrlMsg::Type() at `0x003dfc80` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d02f4
 */
extern int g_nContCtrlMsgType;
