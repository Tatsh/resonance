#pragma once

#include <iostream>

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
 * The destructor at `0x003e1898` is compiler-generated and has no declaration here. So is the
 * string copy at `0x003e1ae8`, byte-identical to GameConnectFailureMsg's at `0x003e1868`.
 */
class GameConnectionLostMsg : public Message {
public:
    /**
     * Construct a message with an empty string.
     *
     * Inline. New() expands it, zeroing the string. A declaration is required because the class
     * declares a second constructor.
     */
    GameConnectionLostMsg() {
    }

    /**
     * Construct a message carrying a copy of a string.
     *
     * The image lists no caller for the out-of-line body.
     *
     * @param unknown04 The string copied into `+0x04`.
     * @ghidraAddress 0x003e1a90
     */
    GameConnectionLostMsg(const HxStr &unknown04);

    /**
     * Produce a message with an empty string on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory.
     *
     * @return The message.
     * @ghidraAddress 0x003d7a98
     */
    static Message *New();

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

    /**
     * Write the string to a diagnostic stream.
     *
     * The body was not claimed as a routine by the disassembler until this reconstruction, because
     * only the vtable reaches it.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e4070
     */
    virtual void Print(std::ostream &stream);

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
