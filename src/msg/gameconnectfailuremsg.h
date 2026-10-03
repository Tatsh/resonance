#pragma once

#include <iostream>

#include "msg/message.h"
#include "os/hxstr.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008efd20`. It has Message as its one base. The object is 0xc bytes
 * and its vtable is at `0x00811f10`. The whole payload is the single HxStr at `+0x04`. Clone()
 * copies it inline through HxStr::HxStr(const HxStr &) rather than delegating, and the cleanup
 * block that follows is the compiler unwinding the copy if it throws.
 *
 * The destructor at `0x003e1618` is compiler-generated and has no declaration here. So is the
 * routine at `0x003e1868`, which copy-constructs the string of one object into another without
 * storing a vtable pointer. GameConnectionLostMsg has a byte-identical copy at `0x003e1ae8`.
 */
class GameConnectFailureMsg : public Message {
public:
    /**
     * Construct a message with an empty string.
     *
     * Inline. New() expands it, zeroing the string. A declaration is required because the class
     * declares a second constructor.
     */
    GameConnectFailureMsg() {
    }

    /**
     * Construct a message carrying a copy of a string.
     *
     * The image lists no caller for the out-of-line body.
     *
     * @param reason The string copied into `+0x04`.
     * @ghidraAddress NTSC-U/C: 0x003e1810
     * @ghidraAddress PAL: 0x00419c80
     */
    GameConnectFailureMsg(const HxStr &reason);

    /**
     * Produce a message with an empty string on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7a58
     * @ghidraAddress PAL: 0x0040f958
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003e1730
     * @ghidraAddress PAL: 0x00419b98
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nGameConnectFailureMsgType.
     * @ghidraAddress NTSC-U/C: 0x003e17d0
     * @ghidraAddress PAL: 0x00419c38
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `GameConnectFailureMsg`.
     * @ghidraAddress NTSC-U/C: 0x003e17e0
     * @ghidraAddress PAL: 0x00419c48
     */
    virtual const char *GetName() const;

    /**
     * Write the string to a diagnostic stream.
     *
     * The body was not claimed as a routine by the disassembler until this reconstruction, because
     * only the vtable reaches it.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e4048
     * @ghidraAddress PAL: 0x0041c278
     */
    virtual void PrintExtra(std::ostream &stream) const;

private:
    HxStr mReason; // +0x04, the text PrintExtra() writes, with a title after the failure it reports
};

/**
 * Identity that GameConnectFailureMsg::Type() reports.
 *
 * This word belongs to GameConnectFailureMsg because GameConnectFailureMsg::Type() at
 * `0x003e17d0` returns it.
 *
 * @ghidraAddress NTSC-U/C: 0x006d0384
 * @ghidraAddress PAL: 0x00713b1c
 */
extern int g_nGameConnectFailureMsgType;
