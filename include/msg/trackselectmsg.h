#pragma once

#include <iostream>

#include "mid/mbt.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `14TrackSelectMsg` in the RTTI descriptor at `0x00902a20`, with Message as its one base. The
 * object is 0x14 bytes and its vtable is at `0x00812d68`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * Print() hands `+0x0c` to Mid::MBT::Print(), and New() initialises it to kMBTInfinity.
 *
 * The destructor at `0x003dc840` is compiler-generated and has no declaration here.
 */
class TrackSelectMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the position is
     * initialised.
     *
     * @return The message.
     * @ghidraAddress 0x003d6e90
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003dc930
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_dwTrackSelectMsgType.
     * @ghidraAddress 0x003dc990
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `TrackSelectMsg`.
     * @ghidraAddress 0x003dc9a0
     */
    virtual const char *Name();

    /**
     * Write the player's colour name, ` tr#`, the two words at `+0x04` and `+0x08` joined by `/`,
     * a space, and the position to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e3ae8
     */
    virtual void Print(std::ostream &stream);

public:
    /** Copied into NetPlayer `+0x48` by its handler at `0x00125f70`. +0x04 */
    int mUnknown04;
    /** Copied into NetPlayer `+0x4c` by the same handler. +0x08 */
    int mUnknown08;

private:
    Mid::MBT mUnknown0c; // +0x0c

public:
    /**
     * Player the message is addressed to.
     *
     * NetPlayer::HandleMessage() compares this member against the receiving player and acts only on
     * a match, which is what types it as a player rather than as a payload word.
     *
     * +0x10
     */
    Player *mUnknown10;
};

/**
 * Identity that TrackSelectMsg::Type() reports.
 *
 * This word belongs to TrackSelectMsg because TrackSelectMsg::Type() at `0x003dc990` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d01ec
 */
extern unsigned int g_dwTrackSelectMsgType;
