#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `16MetStartPauseMsg` in the RTTI descriptor at `0x00901e50`, with Message as its one base. The
 * object is 0x4 bytes and its vtable is at `0x00811bb0`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The destructor at `0x003e29d0` is compiler-generated and has no declaration here.
 */
class MetStartPauseMsg : public Message {
public:
    /**
     * Produce a message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. The program had titled it as a
     * copy of Clone() until the registration identified it.
     *
     * @return The message.
     * @ghidraAddress 0x003d7d10
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e2ac0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nMetStartPauseMsgType.
     * @ghidraAddress 0x003e2af8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `MetStartPauseMsg`.
     * @ghidraAddress 0x003e2b08
     */
    virtual const char *Name();

    /**
     * Write the literal `MetStartPauseMsg` to a diagnostic stream.
     *
     * The body was not claimed as a routine by the disassembler until this reconstruction, because
     * only the vtable reaches it.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e4488
     */
    virtual void Print(std::ostream &stream);
};

/**
 * Identity that MetStartPauseMsg::Type() reports.
 *
 * This word belongs to MetStartPauseMsg because MetStartPauseMsg::Type() at `0x003e2af8` returns
 * it. Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d03e4
 */
extern int g_nMetStartPauseMsgType;
