#pragma once

#include <iostream>

#include "mid/mbt.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `9SeekerMsg` in the RTTI descriptor at `0x008ef800`, with Message as its one base. The object
 * is 0x1c bytes and its vtable is at `0x00812cd8`. The members below are the whole of the class:
 * everything recovered comes from them, and no other routine in the image refers to this type by
 * anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), and Print() labels most of
 * it. It writes the colour name of the player at `+0x04`, then either ` (off)` when the word at
 * `+0x14` is zero, or a range of bars from `+0x08` spanning `+0x0c` bars, the track at `+0x10`,
 * and the bar of the position at `+0x18`. New() sets the player to null and the position to
 * kMBTInfinity.
 *
 * The destructor at `0x003dcbf0` is compiler-generated and has no declaration here.
 */
class SeekerMsg : public Message {
public:
    /**
     * Construct a message with the player unset and the position at kMBTInfinity.
     *
     * Inline. New() expands it. A declaration is required because the class declares further
     * constructors.
     */
    SeekerMsg() {
    }

    /**
     * Report that a player's seeker is off.
     *
     * Inline, with no address of its own. Catcher::PostSeekerMsg() at `0x001ad4e0` expands it on
     * its stack, storing the player, a clear mEnabled, and kMBTInfinity. The bar range and the
     * track are left unset.
     *
     * @param pPlayer The player whose seeker is off.
     */
    explicit SeekerMsg(Player *pPlayer) : mPlayer(pPlayer), mEnabled(0) {
    }

    /**
     * Report a player's seeker over a range of bars on a track.
     *
     * Inline, with no address of its own. Catcher::PostSeekerRangeMsg() at `0x001ad560` expands it
     * on its stack with an mEnabled of 1 and a position of Mid::MBT(0).
     *
     * @param pPlayer The player the seeker belongs to.
     * @param nFirstBar The first bar of the range.
     * @param nBarCount The number of bars in the range.
     * @param nTrack The track.
     * @param nEnabled Non-zero for a seeker that is on.
     * @param when The position of the seeker.
     */
    SeekerMsg(
        Player *pPlayer, int nFirstBar, int nBarCount, int nTrack, int nEnabled, Mid::MBT when)
        : mPlayer(pPlayer), mFirstBar(nFirstBar), mBarCount(nBarCount), mTrack(nTrack),
          mEnabled(nEnabled), mWhen(when) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory.
     *
     * @return The message.
     * @ghidraAddress 0x003d6f10
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003dcce0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nSeekerMsgType.
     * @ghidraAddress 0x003dcd50
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `SeekerMsg`.
     * @ghidraAddress 0x003dcd60
     */
    virtual const char *Name();

    /**
     * Write the player's colour name and then either ` (off)` or the bar range, the track, and
     * the bar of the position to a diagnostic stream.
     *
     * The bar of the position is its tick divided by 1920, so an unset position prints the bar of
     * kMBTInfinity.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003d81f0
     */
    virtual void Print(std::ostream &stream);

private:
    Player *mPlayer = nullptr; // +0x04
    int mFirstBar;             // +0x08, labelled `bars[`
    int mBarCount;             // +0x0c, added to mFirstBar for the end of the range
    int mTrack;                // +0x10, labelled `tr#`
    int mEnabled;              // +0x14, zero prints ` (off)`
    Mid::MBT mWhen;            // +0x18, labelled `when:`
};

/**
 * Identity that SeekerMsg::Type() reports.
 *
 * This word belongs to SeekerMsg because SeekerMsg::Type() at `0x003dcd50` returns it. Several
 * handlers elsewhere read the same word to compare against it, which is the expected shape for a
 * registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d01fc
 */
extern int g_nSeekerMsgType;
