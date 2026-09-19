#pragma once

#include "msg/message.h"
#include "os/hxstr.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `21GameConnectFailureMsg` in the RTTI descriptor at `0x008efd20`, with Message as its one base.
 * The object is 0xc bytes and its vtable is at `0x00811f10`, so the whole payload is the single
 * HxStr at `+0x04`. Clone() copies it inline through HxStr::HxStr(const HxStr &) rather than
 * delegating, and the cleanup block that follows is the compiler unwinding that copy if it
 * throws.
 *
 * The class overrides Message::Print() at `0x003e4048`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class GameConnectFailureMsg : public Message {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e1730
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nGameConnectFailureMsgType.
     * @ghidraAddress 0x003e17d0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `GameConnectFailureMsg`.
     * @ghidraAddress 0x003e17e0
     */
    virtual const char *Name();

private:
    HxStr mUnknown04; // +0x04
};

/**
 * Identity that GameConnectFailureMsg::Type() reports.
 *
 * This word belongs to GameConnectFailureMsg because GameConnectFailureMsg::Type() at
 * `0x003e17d0` returns it.
 *
 * @ghidraAddress 0x006d0384
 */
extern int g_nGameConnectFailureMsgType;
