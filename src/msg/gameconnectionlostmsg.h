#pragma once

#include <iostream>

#include "msg/message.h"
#include "os/hxstr.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008efd30`. It has Message as its one base. The object is 0xc bytes
 * and its vtable is at `0x00811ec8`. The whole payload is the single HxStr at `+0x04`. Clone()
 * copies it inline through HxStr::HxStr(const HxStr &) rather than delegating, and the cleanup
 * block that follows is the compiler unwinding the copy if it throws.
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
     * @param reason The string copied into `+0x04`.
     * @ghidraAddress NTSC-U/C: 0x003e1a90
     * @ghidraAddress PAL: 0x00419f18
     */
    GameConnectionLostMsg(const HxStr &reason);

    /**
     * Produce a message with an empty string on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7a98
     * @ghidraAddress PAL: 0x0040f9a0
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003e19b0
     * @ghidraAddress PAL: 0x00419e30
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nGameConnectionLostMsgType.
     * @ghidraAddress NTSC-U/C: 0x003e1a50
     * @ghidraAddress PAL: 0x00419ed0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `GameConnectionLostMsg`.
     * @ghidraAddress NTSC-U/C: 0x003e1a60
     * @ghidraAddress PAL: 0x00419ee0
     */
    virtual const char *GetName() const;

    /**
     * Write the string to a diagnostic stream.
     *
     * The body was not claimed as a routine by the disassembler until this reconstruction, because
     * only the vtable reaches it.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e4070
     * @ghidraAddress PAL: 0x0041c2a0
     */
    virtual void PrintExtra(std::ostream &stream) const;

private:
    HxStr mReason; // +0x04, the text PrintExtra() writes, with a title after the loss it reports
};

/**
 * Identity that GameConnectionLostMsg::Type() reports.
 *
 * This word belongs to GameConnectionLostMsg because GameConnectionLostMsg::Type() at
 * `0x003e1a50` returns it.
 *
 * @ghidraAddress NTSC-U/C: 0x006d038c
 * @ghidraAddress PAL: 0x00713b24
 */
extern int g_nGameConnectionLostMsgType;
