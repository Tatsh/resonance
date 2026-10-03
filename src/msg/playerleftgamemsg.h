#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x00901d40`. It has Message as its one base. The object is 0x8 bytes
 * and its vtable is at `0x00811e38`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The destructor at `0x003e1d98` is compiler-generated and has no declaration here.
 */
class PlayerLeftGameMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. The payload is left unset.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7b18
     * @ghidraAddress PAL: 0x0040fa30
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003e1e88
     * @ghidraAddress PAL: 0x0041a328
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPlayerLeftGameMsgType.
     * @ghidraAddress NTSC-U/C: 0x003e1ed0
     * @ghidraAddress PAL: 0x0041a370
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PlayerLeftGameMsg`.
     * @ghidraAddress NTSC-U/C: 0x003e1ee0
     * @ghidraAddress PAL: 0x0041a380
     */
    virtual const char *GetName() const;

    /**
     * Write the word at `+0x04` to a diagnostic stream as a number.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e40a0
     * @ghidraAddress PAL: 0x0041c2d0
     */
    virtual void PrintExtra(std::ostream &stream) const;

private:
    int mPlayerId; // +0x04, with a title after the player the class name reports as leaving
};

/**
 * Identity that PlayerLeftGameMsg::Type() reports.
 *
 * This word belongs to PlayerLeftGameMsg because PlayerLeftGameMsg::Type() at `0x003e1ed0`
 * returns it. Several handlers elsewhere read the same word to compare against it, which is the
 * expected shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d039c
 * @ghidraAddress PAL: 0x00713b34
 */
extern int g_nPlayerLeftGameMsgType;
