#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x00901e50`. It has Message as its one base. The object is 0x4 bytes
 * and its vtable is at `0x00811bb0`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The destructor at `0x003e29d0` is compiler-generated and has no declaration here.
 */
class MetStartPauseMsg : public Message {
public:
    /**
     * Produce a message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7d10
     * @ghidraAddress PAL: 0x0040fc28
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003e2ac0
     * @ghidraAddress PAL: 0x0041af60
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nMetStartPauseMsgType.
     * @ghidraAddress NTSC-U/C: 0x003e2af8
     * @ghidraAddress PAL: 0x0041af98
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `MetStartPauseMsg`.
     * @ghidraAddress NTSC-U/C: 0x003e2b08
     * @ghidraAddress PAL: 0x0041afa8
     */
    virtual const char *GetName() const;

    /**
     * Write the literal `MetStartPauseMsg` to a diagnostic stream.
     *
     * The body was not claimed as a routine by the disassembler until this reconstruction, because
     * only the vtable reaches it.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e4488
     * @ghidraAddress PAL: 0x0041c6b8
     */
    virtual void PrintExtra(std::ostream &stream) const;
};

/**
 * Identity that MetStartPauseMsg::Type() reports.
 *
 * This word belongs to MetStartPauseMsg because MetStartPauseMsg::Type() at `0x003e2af8` returns
 * it. Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d03e4
 * @ghidraAddress PAL: 0x00713b7c
 */
extern int g_nMetStartPauseMsgType;
