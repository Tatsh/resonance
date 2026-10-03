#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008efd10`. It has Message as its one base. The object is 0x4 bytes
 * and its vtable is at `0x00811f58`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable.
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
     * Clone(), because the class has no payload.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7a20
     * @ghidraAddress PAL: 0x0040f920
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003e15a8
     * @ghidraAddress PAL: 0x00419a00
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nGameConnectSuccessMsgType.
     * @ghidraAddress NTSC-U/C: 0x003e15e0
     * @ghidraAddress PAL: 0x00419a38
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `GameConnectSuccessMsg`.
     * @ghidraAddress NTSC-U/C: 0x003e15f0
     * @ghidraAddress PAL: 0x00419a48
     */
    virtual const char *GetName() const;

    /**
     * Write nothing.
     *
     * Slot 5. The empty body lies among the other message PrintExtra() bodies at `0x003e2fb0`
     * through `0x003e4500` rather than after this class's destructor, where the re-emitted
     * Message::PrintExtra() stub of each translation unit sits, so the override is this class's
     * own.
     *
     * @param stream The stream, which is not written.
     * @ghidraAddress NTSC-U/C: 0x003e4040
     * @ghidraAddress PAL: 0x0041c270
     */
    virtual void PrintExtra(std::ostream &stream) const;
};

/**
 * Identity that GameConnectSuccessMsg::Type() reports.
 *
 * This word belongs to GameConnectSuccessMsg because GameConnectSuccessMsg::Type() at
 * `0x003e15e0` returns it. Several handlers elsewhere read the same word to compare against it,
 * which is the expected shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d037c
 * @ghidraAddress PAL: 0x00713b14
 */
extern int g_nGameConnectSuccessMsgType;
