#pragma once

#include <iostream>

#include "mid/mbt.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `11AxisYPowMsg` in the RTTI descriptor at `0x00901b10`, with Message as its one base. The
 * object is 0x10 bytes and its vtable is at `0x00813238`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). Print() hands `+0x0c` to
 * Mid::MBT::Print() and writes the colour name of the player at `+0x04`, which types both.
 * InputMap::OnControllerReading(), the one builder, stores the reading's axis value truncated to
 * an integer at `+0x08`.
 *
 * The destructor at `0x003dadb0` is compiler-generated and has no declaration here.
 */
class AxisYPowMsg : public Message {
public:
    /**
     * Construct a message with the position at kMBTInfinity and the rest unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    AxisYPowMsg() {
    }

    /**
     * Report a vertical powerup-axis movement.
     *
     * Inline, with no address of its own. InputMap::OnControllerReading() at `0x00119bf8` expands
     * it on its stack.
     *
     * @param pPlayer The player the controller belongs to.
     * @param nValue The axis value, truncated to an integer.
     * @param position The song position of the reading.
     */
    AxisYPowMsg(Player *pPlayer, int nValue, Mid::MBT position)
        : mPlayer(pPlayer), mValue(nValue), mPosition(position) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the position is
     * initialised.
     *
     * @return The message.
     * @ghidraAddress 0x003d6a68
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003daea0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nAxisYPowMsgType.
     * @ghidraAddress 0x003daef8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `AxisYPowMsg`.
     * @ghidraAddress 0x003daf08
     */
    virtual const char *Name();

    /**
     * Write the position, the player's colour name, and the word at `+0x08`, separated by
     * spaces, to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e3480
     */
    virtual void Print(std::ostream &stream);

    // Public because LocalPlayer::HandleMessage() reads both directly at `0x0011ee04` and
    // `0x0011ee20`, and the image has no accessor.
    Player *mPlayer; /*!< The player the controller belongs to. +0x04 */
    int mValue;      /*!< The axis step. +0x08 */

private:
    Mid::MBT mPosition; // +0x0c
};

/**
 * Identity that AxisYPowMsg::Type() reports.
 *
 * This word belongs to AxisYPowMsg because AxisYPowMsg::Type() at `0x003daef8` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d015c
 */
extern int g_nAxisYPowMsgType;
