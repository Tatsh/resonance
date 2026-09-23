#pragma once

#include <iostream>

#include "mid/mbt.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `11StopRiffMsg` in the RTTI descriptor at `0x008efff0`, with Message as its one base. The
 * object is 0x14 bytes and its vtable is at `0x00813358`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). Print() hands `+0x0c` to
 * Mid::MBT::Print(), writes the colour name of the player at `+0x08`, and labels `+0x04` as `b#`.
 * The word at `+0x10` is not printed.
 *
 * Every member is public because the handlers AutoRiffer dispatches to at `0x001992e0` and Voxer's
 * slot 3 at `0x001d90a4` read the payload with no accessor in the image, and the two stack builds
 * at `0x00119e74` and `0x0011da7c` write all four words.
 *
 * The destructor at `0x003da708` is compiler-generated and has no declaration here.
 */
class StopRiffMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the position is
     * initialised.
     *
     * @return The message.
     * @ghidraAddress 0x003d6968
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003da7f8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nStopRiffMsgType.
     * @ghidraAddress 0x003da858
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `StopRiffMsg`.
     * @ghidraAddress 0x003da868
     */
    virtual const char *Name();

    /**
     * Write the position, a space, the player's colour name, ` b#`, and the word at `+0x04` to a
     * diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e3098
     */
    virtual void Print(std::ostream &stream);

    int mUnknown04;     /*!< Labelled `b#` by Print(). +0x04 */
    Player *mPlayer;    /*!< The player whose riff stops. +0x08 */
    Mid::MBT mPosition; /*!< The song position of the stop. +0x0c */
    int mUnknown10;     /*!< Purpose unrecovered. +0x10 */
};

/**
 * Identity that StopRiffMsg::Type() reports.
 *
 * This word belongs to StopRiffMsg because StopRiffMsg::Type() at `0x003da858` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d013c
 */
extern int g_nStopRiffMsgType;
