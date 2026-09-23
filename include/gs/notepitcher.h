#pragma once

#include "gs/pitcher.h"
#include "mid/mbt.h"
#include "msg/message.h"

class EraseMsg;
class InvalidateSeekerMsg;
class PhraseMgr;
class PitchRiffMsg;
class Player;
class Quantizer;
class TrackData;
class TrackSelectMsg;

namespace Sch {
class TickClock;
} // namespace Sch

/**
 * Pitcher that drives a note track.
 *
 * `11NotePitcher` in the RTTI descriptor at `0x008eef68`, with Pitcher as its one base. Its three
 * tables are at `0x007e14a0`, `0x007e1478`, and `0x007e1448`, and it overrides exactly the two
 * slots Pitcher leaves pure. PitchingSTG's tagged allocation measures the object at 0x70 bytes,
 * and PitchingSTG builds one of these when the track's kind word is 2 and a Scratcher when it is 3.
 *
 * It ignores an EraseOffMsg rather than forwarding it, which is the one message the three Pitcher
 * subclasses treat differently from each other.
 *
 * The titles of the routines HandleMessage() and Tick() dispatch to come from the message each
 * routine posts rather than from the message it receives. That is the naming the program already
 * had, and it is retained here.
 */
class NotePitcher : public Pitcher {
public:
    /**
     * Construct a note pitcher for one track.
     *
     * The seven arguments arrive in a1 through a3 and t0 through t3, which is the register
     * convention this target uses for arguments five through eight.
     *
     * @param pPhraseMgr The phrase manager for the track.
     * @param pQuantizer The quantiser for the track.
     * @param pClock The clock the producer schedules against.
     * @param pTrackData The track description.
     * @param bPlayModeOne Non-zero when the game manager reports play mode 1.
     * @param nUnknown1 PitchingSTG passes 1.
     * @param nUnknown2 PitchingSTG passes zero.
     * @ghidraAddress 0x001b1ce0
     */
    NotePitcher(PhraseMgr *pPhraseMgr,
                Quantizer *pQuantizer,
                Sch::TickClock *pClock,
                const TrackData *pTrackData,
                int bPlayModeOne,
                int nUnknown1,
                int nUnknown2);

    /**
     * @ghidraAddress 0x001b39c0
     */
    virtual ~NotePitcher();

    /**
     * Advance to the bar the elapsed tick count falls in.
     *
     * Divides the elapsed count by mBarDivisor and hands the result to PostSeekerMsgSecond() with
     * a second argument of zero.
     *
     * @param nElapsedTicks Ticks since the epoch.
     * @return 1 always.
     * @ghidraAddress 0x001b3b98
     */
    virtual int Tick(int nElapsedTicks);

protected:
    /**
     * React to a PitchRiffMsg for this track and player.
     *
     * The message's position is quantised. A bar CanPlayBar() rejects plays `SND_INACTIVE`.
     * Otherwise, at a new position, the riff TrackData::GetRiff() reports goes out as a
     * MultiMuseMsg, PostPhraseCapturedMsg() records the gem, a PitchMsg follows, and the position
     * is stored in mUnknown4c.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001b1f10
     */
    void PostPitchMsg(PitchRiffMsg *pMsg);

    /**
     * Erase this player's phrases at an EraseMsg's position, outside play mode 1.
     *
     * The bar, or with the message's last word set every bar of its step, is cleared wherever
     * mUnknown44 owns it, and clearing the message's own bar also sends an AllNotesOffMsg. When
     * anything was cleared, `SND_ERASE_SECTION` or `SND_ERASE` plays, a ShowEraseEffectMsg goes
     * out, and the seeker is posted again. The position is stored in mUnknown48 either way.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001b20b0
     */
    void PostAllNotesOffMsg(EraseMsg *pMsg);

    /**
     * Install the player a TrackSelectMsg for this track selects.
     *
     * A null player first turns the previous player's seeker off. A real player has its seeker
     * posted at the message's bar with the force flag set.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001b22f0
     */
    void PostSeekerMsg(TrackSelectMsg *pMsg);

    /**
     * Record a caught gem.
     *
     * A new bar becomes mUnknown54 and is announced with a PhraseCapturedMsg worth the bar's
     * points. In play mode 1 a phrase another player owns there is cleared first. Unless
     * Player::Slot10() reports non-zero, the gem goes to PhraseMgr::AddGem() for mUnknown54 alone.
     * Otherwise it goes to every bar of the step that CanPlayBar() accepts and whose phrase
     * PhraseMgr::PhrasesMatch() pairs with mUnknown54, and then to mUnknown54 itself. The bar is
     * then replayed from one tick after the gem.
     *
     * @param nGem The gem, the PitchRiffMsg's first word.
     * @param nTick The quantised song position.
     * @ghidraAddress 0x001b2400
     */
    void PostPhraseCapturedMsg(int nGem, int nTick);

    /**
     * Post the seeker for mUnknown44 at the first playable bar of the eight from nBar.
     *
     * Nothing is sent for the stand-in player. Without bForce, a player whose Player::Slot5()
     * reports non-zero has its seeker turned off. Outside play mode 1, a player whose
     * Player::Slot10() reports zero, or a search that finds no bar CanPlayBar() accepts, also
     * turns the seeker off. A found bar posts a seeker over the mUnknown5c bars of its step.
     *
     * @param nBar The bar to search from, clamped to zero.
     * @param bForce Non-zero to skip the Player::Slot5() test.
     * @ghidraAddress 0x001b2710
     */
    void PostSeekerMsgSecond(int nBar, int bForce);

    /**
     * Act on a message.
     *
     * Primary table slot 3.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001b3bd0
     */
    virtual void HandleMessage(Message *pMsg);

private:
    // The out-of-line copy of the InvalidateSeekerMsg branch HandleMessage() expands inline.
    // 0x001b3a38
    void OnInvalidateSeeker(InvalidateSeekerMsg *pMsg);

    // Returns non-zero when nTick differs from mUnknown4c. PostPitchMsg() calls it at 0x001b1fa4.
    // 0x001b3b88
    int IsOtherTick(int nTick);

    // Reports whether mUnknown44 may play nBar. In play mode 1 that is TrackData::QueryBar() and
    // Player::Slot9(). Otherwise the bar needs TrackData::QueryBar() and, unless mUnknown60 is set,
    // no owner or nCurrentBar equal to nBar. An owned bar must also belong to mUnknown44.
    // PostSeekerMsgSecond() expands it inline, and PostPitchMsg() and PostPhraseCapturedMsg() call
    // the out-of-line copy. The title is inferred.
    // 0x001b3a68
    int CanPlayBar(int nBar, int nCurrentBar);

    PhraseMgr *mPhraseMgr; // +0x38
    Quantizer *mQuantizer; // +0x3c
    // Copied from the track description's `+0x04`. Matched against an InvalidateSeekerMsg's
    // `+0x08`, so it identifies the track this pitcher serves.
    int mUnknown40;      // +0x40
    Player *mUnknown44;  // +0x44, starts at g_nullPlayer
    Mid::MBT mUnknown48; // +0x48, MBT(-1), stored and then tested at 0x001b1e60
    Mid::MBT mUnknown4c; // +0x4c, starts at kMBTInfinity
    // Copied from the phrase manager's `+0x34` after an initial kMBTInfinity. Turns an elapsed
    // tick count into a bar index.
    int mBarDivisor;             // +0x50
    int mUnknown54;              // +0x54, starts at -1
    int mPlayModeOne;            // +0x58, the constructor's fifth argument
    int mUnknown5c;              // +0x5c, starts at 2
    int mUnknown60;              // +0x60, the constructor's sixth argument
    int mUnknown64;              // +0x64, the constructor's seventh argument
    const TrackData *mTrackData; // +0x68
    Sch::TickClock *mClock;      // +0x6c
};
