#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `12CaughtBarMsg` in the RTTI descriptor at `0x008f07a0`, with Message as its one base. The
 * object is 0xc bytes and its vtable is at `0x007e09e0`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). The two names come from
 * Catcher::PostCaughtBarMsg(), the one builder, which stores the catcher's player and the caught
 * bar. Readers of the fields have not been traced, so they are private by default.
 *
 * The destructor at `0x001b1100` is compiler-generated and has no declaration here.
 */
class CaughtBarMsg : public Message {
public:
    /**
     * Construct a message with the payload unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    CaughtBarMsg() {
    }

    /**
     * Report a bar a player caught.
     *
     * Inline, with no address of its own. Catcher::PostCaughtBarMsg() at `0x001ac83c` expands it
     * on its stack and delivers it to the player's own sink.
     *
     * @param pPlayer The player.
     * @param nBar The caught bar.
     */
    CaughtBarMsg(Player *pPlayer, int nBar) : mPlayer(pPlayer), mBar(nBar) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 119.
     *
     * @return The message.
     * @ghidraAddress 0x003d6d08
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x001b11b0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nCaughtBarMsgType.
     * @ghidraAddress 0x001b1200
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `CaughtBarMsg`.
     * @ghidraAddress 0x001b1210
     */
    virtual const char *Name();

private:
    Player *mPlayer; // +0x04
    int mBar;        // +0x08
};

/**
 * Identity that CaughtBarMsg::Type() reports.
 *
 * This word belongs to CaughtBarMsg because CaughtBarMsg::Type() at `0x001b1200` returns it, and
 * the registration at `0x003d9818` passes the same value, 119, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d01b4
 */
extern int g_nCaughtBarMsgType;
