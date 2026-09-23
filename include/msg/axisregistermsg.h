#pragma once

#include <iostream>

#include "mid/mbt.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `15AxisRegisterMsg` in the RTTI descriptor at `0x008eed48`, with Message as its one base. The
 * object is 0x14 bytes and its vtable is at `0x008132c8`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). Print() hands `+0x0c` to
 * Mid::MBT::Print() and writes the colour name of the player at `+0x04`, which types both. The
 * float at `+0x08` is printed without a label, and the word at `+0x10` is not printed.
 * InputMap::OnControllerReading(), the one builder, stores the reading's axis value at `+0x08` and
 * the track the player's slot 4 reports at `+0x10`.
 *
 * The destructor at `0x003daa00` is compiler-generated and has no declaration here.
 */
class AxisRegisterMsg : public Message {
public:
    /**
     * Construct a message with the position at kMBTInfinity and the rest unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    AxisRegisterMsg() {
    }

    /**
     * Report a register-axis movement.
     *
     * Inline, with no address of its own. InputMap::OnControllerReading() at `0x00119950` expands
     * it on its stack.
     *
     * @param pPlayer The player the controller belongs to.
     * @param flValue The axis value.
     * @param position The song position of the reading.
     * @param nTrack The player's track.
     */
    AxisRegisterMsg(Player *pPlayer, float flValue, Mid::MBT position, int nTrack)
        : mPlayer(pPlayer), mValue(flValue), mPosition(position), mTrack(nTrack) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the position is
     * initialised.
     *
     * @return The message.
     * @ghidraAddress 0x003d69e8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003daaf0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nAxisRegisterMsgType.
     * @ghidraAddress 0x003dab50
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `AxisRegisterMsg`.
     * @ghidraAddress 0x003dab60
     */
    virtual const char *Name();

    /**
     * Write the position, the player's colour name, and the float at `+0x08`, separated by
     * spaces, to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e3160
     */
    virtual void Print(std::ostream &stream);

    // PitchPicker::HandleMessage() at 0x001c3220 reads the two members below directly with no
    // accessor in the image, comparing mPlayer with its own player and scaling mValue by 1024.
    Player *mPlayer; /*!< The player the controller belongs to. +0x04 */
    float mValue;    /*!< The axis value. +0x08 */

private:
    Mid::MBT mPosition; // +0x0c
    int mTrack;         // +0x10
};

/**
 * Identity that AxisRegisterMsg::Type() reports.
 *
 * This word belongs to AxisRegisterMsg because AxisRegisterMsg::Type() at `0x003dab50` returns
 * it. Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d014c
 */
extern int g_nAxisRegisterMsgType;
