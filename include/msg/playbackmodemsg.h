#pragma once

#include "mid/mbt.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `15PlaybackModeMsg` in the RTTI descriptor at `0x009022a0`, with Message as its one base. The
 * object is 0xc bytes and its vtable is at `0x007cf578`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). The types come from
 * InputMap::OnControllerReading(), the one builder, which stores the resolved player and the
 * controller reading's position. Readers of the fields have not been traced, so they are private
 * by default.
 *
 * The destructor at `0x0011d4c8` is compiler-generated and has no declaration here. The routine
 * at `0x0011d500` is a further emission of the type-information accessor.
 */
class PlaybackModeMsg : public Message {
public:
    /**
     * Construct a message with the position at kMBTInfinity and the player unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    PlaybackModeMsg() {
    }

    /**
     * Report a playback-mode press.
     *
     * Inline, with no address of its own. InputMap::OnControllerReading() at `0x00119878` expands
     * it on its stack.
     *
     * @param pPlayer The player the controller belongs to.
     * @param position The song position of the reading.
     */
    PlaybackModeMsg(Player *pPlayer, Mid::MBT position) : mPlayer(pPlayer), mPosition(position) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 105.
     *
     * @return The message.
     * @ghidraAddress 0x003d69a8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x0011d578
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPlaybackModeMsgType.
     * @ghidraAddress 0x0011d5c8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PlaybackModeMsg`.
     * @ghidraAddress 0x0011d5d8
     */
    virtual const char *Name();

private:
    Player *mPlayer;    // +0x04
    Mid::MBT mPosition; // +0x08
};

/**
 * Identity that PlaybackModeMsg::Type() reports.
 *
 * This word belongs to PlaybackModeMsg because PlaybackModeMsg::Type() at `0x0011d5c8` returns it,
 * and the registration at `0x003d9818` passes the same value, 105, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d0144
 */
extern int g_nPlaybackModeMsgType;
