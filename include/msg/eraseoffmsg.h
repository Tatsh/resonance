#pragma once

#include <iostream>

#include "mid/mbt.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `11EraseOffMsg` in the RTTI descriptor at `0x00901e10`, with Message as its one base. The
 * object is 0x10 bytes and its vtable is at `0x00813118`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). Print() hands `+0x08` to
 * Mid::MBT::Print() and writes the colour name of the player at `+0x04`, which types both. The
 * word at `+0x0c` is not printed.
 *
 * The destructor at `0x003db4c8` is compiler-generated and has no declaration here.
 */
class EraseOffMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the position is
     * initialised.
     *
     * @return The message.
     * @ghidraAddress 0x003d6b68
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003db5b8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nEraseOffMsgType.
     * @ghidraAddress 0x003db610
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `EraseOffMsg`.
     * @ghidraAddress 0x003db620
     */
    virtual const char *Name();

    /**
     * Write the position and the player's colour name, separated by a space, to a diagnostic
     * stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e33d0
     */
    virtual void Print(std::ostream &stream);

private:
    Player *mPlayer;    // +0x04
    Mid::MBT mPosition; // +0x08
    int mUnknown0c;     // +0x0c
};

/**
 * Identity that EraseOffMsg::Type() reports.
 *
 * This word belongs to EraseOffMsg because EraseOffMsg::Type() at `0x003db610` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d017c
 */
extern int g_nEraseOffMsgType;
