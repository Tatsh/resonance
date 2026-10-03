#pragma once

#include <iostream>

#include "mid/tick.h"
#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008efd00`. It has Message as its one base. The object is 0x10 bytes
 * and its vtable is at `0x00812420`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). New() initialises `+0x04` to
 * kTickInfinity, the one store it makes, which marks that word as a position. PrintExtra() writes
 * only the word at `+0x0c`. Readers of the fields have not been traced, so they are private by
 * default.
 *
 * The destructor at `0x003dfb38` is compiler-generated and has no declaration here.
 */
class ContCtrlMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the position is
     * initialised.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7640
     * @ghidraAddress PAL: 0x0040f540
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003dfc28
     * @ghidraAddress PAL: 0x00418080
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nContCtrlMsgType.
     * @ghidraAddress NTSC-U/C: 0x003dfc80
     * @ghidraAddress PAL: 0x004180d8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `ContCtrlMsg`.
     * @ghidraAddress NTSC-U/C: 0x003dfc90
     * @ghidraAddress PAL: 0x004180e8
     */
    virtual const char *GetName() const;

    /**
     * Write the word at `+0x0c` to a diagnostic stream as a number.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e3e80
     * @ghidraAddress PAL: 0x0041c050
     */
    virtual void PrintExtra(std::ostream &stream) const;

private:
    // The controller and value titles are inferred from the class name, a MIDI continuous
    // controller change, whose value is the word PrintExtra() writes.
    Sch::Tick mPosition; // +0x04
    int mController;     // +0x08
    int mValue;          // +0x0c
};

/**
 * Identity that ContCtrlMsg::Type() reports.
 *
 * This word belongs to ContCtrlMsg because ContCtrlMsg::Type() at `0x003dfc80` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d02f4
 * @ghidraAddress PAL: 0x00713a8c
 */
extern int g_nContCtrlMsgType;
