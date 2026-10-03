#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008ef730`. It has Message as its one base. The object is 0x4 bytes
 * and its vtable is at `0x00812078`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The destructor at `0x003e0ed0` is compiler-generated and has no declaration here.
 */
class LeaveGameMsg : public Message {
public:
    /**
     * Produce a message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. The body is byte-identical to
     * Clone(), because the class has no payload.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7940
     * @ghidraAddress PAL: 0x0040f840
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003e0f80
     * @ghidraAddress PAL: 0x004193d8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nLeaveGameMsgType.
     * @ghidraAddress NTSC-U/C: 0x003e0fb8
     * @ghidraAddress PAL: 0x00419410
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `LeaveGameMsg`.
     * @ghidraAddress NTSC-U/C: 0x003e0fc8
     * @ghidraAddress PAL: 0x00419420
     */
    virtual const char *GetName() const;
};

/**
 * Identity that LeaveGameMsg::Type() reports.
 *
 * This word belongs to LeaveGameMsg because LeaveGameMsg::Type() at `0x003e0fb8` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d035c
 * @ghidraAddress PAL: 0x00713af4
 */
extern int g_nLeaveGameMsgType;
