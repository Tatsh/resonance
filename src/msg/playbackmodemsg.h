#pragma once

#include "mid/tick.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x009022a0`. It has Message as its one base. The object is 0xc bytes
 * and its vtable is at `0x007cf578`. The allocation in New() and the allocation in Clone() report
 * the same size.
 *
 * The payload layout comes from the run of field copies in Clone(). The types come from
 * InputMap::OnControllerReading(), the one builder, which stores the resolved player and the
 * controller reading's position. Both members are public because Gamer's playback-mode handler at
 * `0x00110e60` reads them directly with no accessor in the image.
 *
 * The destructor at `0x0011d4c8` is compiler-generated and has no declaration here. The routine
 * at `0x0011d500` is a further emission of the type-information accessor.
 */
class PlaybackModeMsg : public Message {
public:
    /**
     * Identity that Type() reports, 105.
     *
     * @ghidraAddress NTSC-U/C: 0x006d0144
     * @ghidraAddress PAL: 0x007138dc
     */
    static int sID;

    /**
     * Construct a message with the position at kTickInfinity and the player unset.
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
    PlaybackModeMsg(Player *pPlayer, Sch::Tick position) : mPlayer(pPlayer), mPosition(position) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 105.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d69a8
     * @ghidraAddress PAL: 0x0040e898
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x0011d578
     * @ghidraAddress PAL: 0x0011db00
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return sID.
     * @ghidraAddress NTSC-U/C: 0x0011d5c8
     * @ghidraAddress PAL: 0x0011db50
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PlaybackModeMsg`.
     * @ghidraAddress NTSC-U/C: 0x0011d5d8
     * @ghidraAddress PAL: 0x0011db60
     */
    virtual const char *GetName() const;

    Player *mPlayer;     /*!< The player the controller belongs to. +0x04 */
    Sch::Tick mPosition; /*!< The song position of the reading. +0x08 */
};
