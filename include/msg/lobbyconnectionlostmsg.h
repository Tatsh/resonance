#pragma once

#include <iostream>

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
 * Unlike its two siblings, Print() writes nothing, although the class carries the same string.
 *
 * The destructor at `0x003e1b18` is compiler-generated and has no declaration here. So is the
 * string copy at `0x003e1d68`, the same shape as GameConnectFailureMsg's at `0x003e1868`.
 */
class LobbyConnectionLostMsg : public Message {
public:
    /**
     * Construct a message with an empty string.
     *
     * Inline. New() expands it, zeroing the string. A declaration is required because the class
     * declares a second constructor.
     */
    LobbyConnectionLostMsg() {
    }

    /**
     * Construct a message carrying a copy of a string.
     *
     * The image lists no caller for the out-of-line body.
     *
     * @param unknown04 The string copied into `+0x04`.
     * @ghidraAddress 0x003e1d10
     */
    LobbyConnectionLostMsg(const HxStr &unknown04);

    /**
     * Produce a message with an empty string on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory.
     *
     * @return The message.
     * @ghidraAddress 0x003d7ad8
     */
    static Message *New();

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

    /**
     * Write nothing.
     *
     * Slot 5. The empty body lies among the other message Print() bodies, directly after
     * GameConnectionLostMsg's, rather than after this class's destructor, so the override is this
     * class's own.
     *
     * @param stream The stream, which is not written.
     * @ghidraAddress 0x003e4098
     */
    virtual void Print(std::ostream &stream);

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
