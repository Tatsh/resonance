#pragma once

#include <iostream>

#include "mid/tick.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x00901cf0`. It has Message as its one base. The object is 0x14 bytes
 * and its vtable is at `0x00813160`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone().
 * InputMap::OnControllerReading(), the one builder, stores the resolved player, the controller
 * reading's position, the track the player's slot 4 reports, and a double-tap flag. The flag is set
 * when the press falls within 400,000,000 of the previous press time the player stores at `+0x40`,
 * and Scratcher widens the erased range to the whole section when it is set.
 *
 * PrintExtra() hands `+0x08` to Sch::Tick::Print() and writes the colour name of the player at
 * `+0x04`, which types both. New() initialises the position to kTickInfinity.
 *
 * The destructor at `0x003db2f0` is compiler-generated and has no declaration here.
 */
class EraseMsg : public Message {
public:
    /**
     * Identity that Type() reports.
     *
     * @ghidraAddress NTSC-U/C: 0x006d0174
     * @ghidraAddress PAL: 0x0071390c
     */
    static int sID;

    /**
     * Construct a message with the position at kTickInfinity and the rest unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    EraseMsg() {
    }

    /**
     * Report an erase press.
     *
     * Inline, with no address of its own. InputMap::OnControllerReading() at `0x00119aa0` expands
     * it on its stack. The four arguments are the four members in declaration order.
     *
     * @param pPlayer The player the controller belongs to.
     * @param position The song position of the reading.
     * @param nTrack The player's track.
     * @param nDoubleTap Non-zero when the press follows the previous one closely.
     */
    EraseMsg(Player *pPlayer, Sch::Tick position, int nTrack, int nDoubleTap)
        : mPlayer(pPlayer), mPosition(position), mTrack(nTrack), mDoubleTap(nDoubleTap) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the position is
     * initialised.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d6b28
     * @ghidraAddress PAL: 0x0040ea18
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003db3e0
     * @ghidraAddress PAL: 0x00413818
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return sID.
     * @ghidraAddress NTSC-U/C: 0x003db440
     * @ghidraAddress PAL: 0x00413878
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `EraseMsg`.
     * @ghidraAddress NTSC-U/C: 0x003db450
     * @ghidraAddress PAL: 0x00413888
     */
    virtual const char *GetName() const;

    /**
     * Write the position and the player's colour name, separated by a space, to a diagnostic
     * stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e3320
     * @ghidraAddress PAL: 0x0041b640
     */
    virtual void PrintExtra(std::ostream &stream) const;

public:
    // Public because Voxer::DispatchPriv(), Scratcher::DispatchPriv(), and
    // NotePitcher::DispatchPriv() read the members below directly, through an EraseMsg pointer
    // from outside the hierarchy, and the image exposes no accessor. A friend declaration fits
    // equally well.
    Player *mPlayer;     // +0x04
    Sch::Tick mPosition; // +0x08
    int mTrack;          // +0x0c
    int mDoubleTap;      // +0x10
};
