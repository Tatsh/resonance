#pragma once

#include "msg/message.h"
#include "os/hxstr.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `22LobbyConnectionLostMsg` in the RTTI descriptor at `0x008efd40`, with Message as its one
 * base. The object is 0xc bytes and its vtable is at `0x00811e80`, so the whole payload is the
 * single HxStr at `+0x04`. Clone() copies it inline through HxStr::HxStr(const HxStr &) rather
 * than delegating, and the cleanup block that follows is the compiler unwinding that copy if it
 * throws.
 *
 * The class overrides Message::Print() at `0x003e4098`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class LobbyConnectionLostMsg : public Message {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e1c30
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nLobbyConnectionLostMsgType.
     * @ghidraAddress 0x003e1cd0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `LobbyConnectionLostMsg`.
     * @ghidraAddress 0x003e1ce0
     */
    virtual const char *Name();

private:
    HxStr mUnknown04; // +0x04
};

/**
 * Identity that LobbyConnectionLostMsg::Type() reports.
 *
 * This word belongs to LobbyConnectionLostMsg because LobbyConnectionLostMsg::Type() at
 * `0x003e1cd0` returns it.
 *
 * @ghidraAddress 0x006d0394
 */
extern int g_nLobbyConnectionLostMsgType;
