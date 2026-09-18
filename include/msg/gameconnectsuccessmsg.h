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
 * The class overrides Message::Print() at `0x003e4040`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class GameConnectSuccessMsg : public Message {
public:
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
