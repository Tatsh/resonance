#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008eecd8`. It has Message as its one base. The object is 0x4 bytes
 * and its vtable is at `0x00811bf8`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The destructor at `0x003e2870` is compiler-generated and has no declaration here.
 */
class MetStartNetLaunchMsg : public Message {
public:
    /**
     * Produce a message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7cd8
     * @ghidraAddress PAL: 0x0040fbf0
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003e2960
     * @ghidraAddress PAL: 0x0041ae00
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nMetStartNetLaunchMsgType.
     * @ghidraAddress NTSC-U/C: 0x003e2998
     * @ghidraAddress PAL: 0x0041ae38
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `MetStartNetLaunchMsg`.
     * @ghidraAddress NTSC-U/C: 0x003e29a8
     * @ghidraAddress PAL: 0x0041ae48
     */
    virtual const char *Name();

    /**
     * Write the literal `MetStartNetLaunchMsg` to a diagnostic stream.
     *
     * The body was not claimed as a routine by the disassembler until this reconstruction, because
     * only the vtable reaches it.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e4460
     * @ghidraAddress PAL: 0x0041c690
     */
    virtual void Print(std::ostream &stream);
};

/**
 * Identity that MetStartNetLaunchMsg::Type() reports.
 *
 * This word belongs to MetStartNetLaunchMsg because MetStartNetLaunchMsg::Type() at `0x003e2998`
 * returns it. Several handlers elsewhere read the same word to compare against it, which is the
 * expected shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d03dc
 * @ghidraAddress PAL: 0x00713b74
 */
extern int g_nMetStartNetLaunchMsgType;
