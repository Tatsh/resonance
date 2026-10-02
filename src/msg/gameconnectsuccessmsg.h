#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `21GameConnectSuccessMsg` in the RTTI descriptor at `0x008efd10`, with Message as its one base.
 * The object is 0x4 bytes and its vtable is at `0x00811f58`. The members below are the whole of
 * the class: everything recovered comes from them, and no other routine in the image refers to
 * this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The destructor at `0x003e14b8` is compiler-generated and has no declaration here.
 */
class GameConnectSuccessMsg : public Message {
public:
    /**
     * Produce a message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. The body is byte-identical to
     * Clone(), because the class has no payload, and the program had titled it as a copy of
     * Clone() until the registration identified it.
     *
     * @return The message.
     * @ghidraAddress 0x003d7a20
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e15a8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nGameConnectSuccessMsgType.
     * @ghidraAddress 0x003e15e0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `GameConnectSuccessMsg`.
     * @ghidraAddress 0x003e15f0
     */
    virtual const char *Name();

    /**
     * Write nothing.
     *
     * Slot 5. The empty body lies among the other message Print() bodies at `0x003e2fb0` through
     * `0x003e4500` rather than after this class's destructor, where the re-emitted
     * Message::Print() stub of each translation unit sits, so the override is this class's own.
     *
     * @param stream The stream, which is not written.
     * @ghidraAddress 0x003e4040
     */
    virtual void Print(std::ostream &stream);
};

/**
 * Identity that GameConnectSuccessMsg::Type() reports.
 *
 * This word belongs to GameConnectSuccessMsg because GameConnectSuccessMsg::Type() at
 * `0x003e15e0` returns it. Several handlers elsewhere read the same word to compare against it,
 * which is the expected shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d037c
 */
extern int g_nGameConnectSuccessMsgType;
