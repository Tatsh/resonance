#pragma once

#include "mid/mbt.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008effd0`. It has Message as its one base. The object is 0x10 bytes
 * and its vtable is at `0x007cf4a0`. New(), Clone(), and the one builder install the table. An
 * identical, unreferenced emission sits at `0x00813088`. The allocation in New() and the allocation
 * in Clone() report the same size.
 *
 * The payload layout comes from the run of field copies in Clone(). The types come from
 * InputMap::OnControllerReading(), the one builder, which stores the resolved player, the
 * controller reading's position, and the track the player's slot 4 reports. Readers of the fields
 * have not been traced, so they are private by default.
 *
 * The destructor at `0x0011d830` is compiler-generated and has no declaration here. The routine
 * at `0x0011d868` is a further emission of the type-information accessor.
 */
class LoopToolMsg : public Message {
public:
    /**
     * Construct a message with the position at kMBTInfinity and the rest unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    LoopToolMsg() {
    }

    /**
     * Report a loop-tool press.
     *
     * Inline, with no address of its own. InputMap::OnControllerReading() at `0x00119b80` expands
     * it on its stack.
     *
     * @param pPlayer The player the controller belongs to.
     * @param position The song position of the reading.
     * @param nTrack The player's track.
     */
    LoopToolMsg(Player *pPlayer, Mid::MBT position, int nTrack)
        : mPlayer(pPlayer), mPosition(position), mTrack(nTrack) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 114.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d6be8
     * @ghidraAddress PAL: 0x0040ead8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x0011d8e0
     * @ghidraAddress PAL: 0x0011de68
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nLoopToolMsgType.
     * @ghidraAddress NTSC-U/C: 0x0011d938
     * @ghidraAddress PAL: 0x0011dec0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `LoopToolMsg`.
     * @ghidraAddress NTSC-U/C: 0x0011d948
     * @ghidraAddress PAL: 0x0011ded0
     */
    virtual const char *GetName() const;

    // Public because LocalPlayer::DispatchPriv() reads both directly at `0x0011ee48` and
    // `0x0011ee54`, and the image has no accessor.
    Player *mPlayer;    /*!< The player the controller belongs to. +0x04 */
    Mid::MBT mPosition; /*!< The song position of the reading. +0x08 */

private:
    int mTrack; // +0x0c
};

/**
 * Identity that LoopToolMsg::Type() reports.
 *
 * This word belongs to LoopToolMsg because LoopToolMsg::Type() at `0x0011d938` returns it, and the
 * registration at `0x003d9818` passes the same value, 114, as the identity of this class's
 * factory.
 *
 * @ghidraAddress NTSC-U/C: 0x006d018c
 * @ghidraAddress PAL: 0x00713924
 */
extern int g_nLoopToolMsgType;
