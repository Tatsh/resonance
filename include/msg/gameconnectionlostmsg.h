#pragma once

#include "msg/message.h"
#include "os/hxstr.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `21GameConnectionLostMsg` in the RTTI descriptor at `0x008efd30`, with Message as its one base.
 * The object is 0xc bytes and its vtable is at `0x00811ec8`, so the whole payload is the single
 * HxStr at `+0x04`. Clone() copies it inline through HxStr::HxStr(const HxStr &) rather than
 * delegating, and the cleanup block that follows is the compiler unwinding that copy if it
 * throws.
 *
 * The class overrides Message::Print() at `0x003e4070`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class GameConnectionLostMsg : public Message {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e19b0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nGameConnectionLostMsgType.
     * @ghidraAddress 0x003e1a50
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `GameConnectionLostMsg`.
     * @ghidraAddress 0x003e1a60
     */
    virtual const char *Name();

private:
    HxStr mUnknown04; // +0x04
};

/**
 * Identity that GameConnectionLostMsg::Type() reports.
 *
 * This word belongs to GameConnectionLostMsg because GameConnectionLostMsg::Type() at
 * `0x003e1a50` returns it.
 *
 * @ghidraAddress 0x006d038c
 */
extern int g_nGameConnectionLostMsgType;
