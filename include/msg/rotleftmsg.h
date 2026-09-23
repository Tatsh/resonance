#pragma once

#include "mid/mbt.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `10RotLeftMsg` in the RTTI descriptor at `0x008eec58`, with Message as its one base. The object
 * is 0xc bytes and its vtable is at `0x007cf608`. The allocation in New() and the allocation in
 * Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). The types come from
 * InputMap::OnControllerReading(), the one builder, which stores the resolved player and the
 * controller reading's position. Readers of the fields have not been traced, so they are private
 * by default.
 *
 * The destructor at `0x0011d288` is compiler-generated and has no declaration here. The routine
 * at `0x0011d2c0` is a further emission of the type-information accessor.
 */
class RotLeftMsg : public Message {
public:
    /**
     * Construct a message with the position at kMBTInfinity and the player unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    RotLeftMsg() {
    }

    /**
     * Report a rotate-left press.
     *
     * Inline, with no address of its own. InputMap::OnControllerReading() at `0x001197d8` expands
     * it on its stack.
     *
     * @param pPlayer The player the controller belongs to.
     * @param position The song position of the reading.
     */
    RotLeftMsg(Player *pPlayer, Mid::MBT position) : mPlayer(pPlayer), mPosition(position) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 101.
     *
     * @return The message.
     * @ghidraAddress 0x003d68a8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x0011d338
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nRotLeftMsgType.
     * @ghidraAddress 0x0011d388
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `RotLeftMsg`.
     * @ghidraAddress 0x0011d398
     */
    virtual const char *Name();

private:
    Player *mPlayer;    // +0x04
    Mid::MBT mPosition; // +0x08
};

/**
 * Identity that RotLeftMsg::Type() reports.
 *
 * This word belongs to RotLeftMsg because RotLeftMsg::Type() at `0x0011d388` returns it, and the
 * registration at `0x003d9818` passes the same value, 101, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d0124
 */
extern int g_nRotLeftMsgType;
