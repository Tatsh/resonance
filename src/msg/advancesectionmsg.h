#pragma once

#include "mid/tick.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008eed08`. It has Message as its one base. The object is 0x10 bytes
 * and its vtable is at `0x007cf530`. The allocation in New() and the allocation in Clone() report
 * the same size.
 *
 * The payload layout comes from the run of field copies in Clone(). The types come from
 * InputMap::OnControllerReading(), the one builder, which stores the resolved player, the
 * controller reading's position, and the track the player's slot 4 reports. mPlayer and mPosition
 * are public because Gamer's DispatchPriv() at `0x001129e8` reads them directly with no accessor
 * in the image.
 *
 * The destructor at `0x0011d5e8` is compiler-generated and has no declaration here. The routine
 * at `0x0011d620` is a further emission of the type-information accessor.
 */
class AdvanceSectionMsg : public Message {
public:
    /**
     * Identity that Type() reports, 113.
     *
     * @ghidraAddress NTSC-U/C: 0x006d0184
     * @ghidraAddress PAL: 0x0071391c
     */
    static int sID;

    /**
     * Construct a message with the position at kTickInfinity and the rest unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    AdvanceSectionMsg() {
    }

    /**
     * Report an advance-section press.
     *
     * Inline, with no address of its own. InputMap::OnControllerReading() at `0x00119b18` expands
     * it on its stack.
     *
     * @param pPlayer The player the controller belongs to.
     * @param position The song position of the reading.
     * @param nTrack The player's track.
     */
    AdvanceSectionMsg(Player *pPlayer, Sch::Tick position, int nTrack)
        : mPlayer(pPlayer), mPosition(position), mTrack(nTrack) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 113.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d6ba8
     * @ghidraAddress PAL: 0x0040ea98
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x0011d698
     * @ghidraAddress PAL: 0x0011dc20
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return sID.
     * @ghidraAddress NTSC-U/C: 0x0011d6f0
     * @ghidraAddress PAL: 0x0011dc78
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `AdvanceSectionMsg`.
     * @ghidraAddress NTSC-U/C: 0x0011d700
     * @ghidraAddress PAL: 0x0011dc88
     */
    virtual const char *GetName() const;

    Player *mPlayer;     /*!< The player the controller belongs to. +0x04 */
    Sch::Tick mPosition; /*!< The song position of the reading. +0x08 */

private:
    int mTrack; // +0x0c
};
