#pragma once

#include "gs/pitcher.h"
#include "mid/tick.h"
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
 * Its RTTI descriptor is at `0x008eef68`. It has Pitcher as its one base. Its three tables are at
 * `0x007e14a0`, `0x007e1478`, and `0x007e1448`, and it overrides exactly the two slots Pitcher
 * declares pure. PitchingSTG's tagged allocation measures the object at 0x70 bytes, and PitchingSTG
 * builds one of these when the track's kind word is 2 and a Scratcher when it is 3.
 *
 * It ignores an EraseOffMsg rather than forwarding it, which is the one message the three Pitcher
 * subclasses treat differently from each other.
 *
 * The names of the routines DispatchPriv() and Tick() dispatch to come from the message each
 * routine posts rather than from the message it receives.
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
     * @param bAllowOwnedBars Non-zero to let the player replay a bar it is already the owner of.
     *                        PitchingSTG
     *                        passes 1.
     * @param nUnreadOption Stored and never read. PitchingSTG passes zero.
     * @ghidraAddress NTSC-U/C: 0x001b1ce0
     * @ghidraAddress PAL: 0x001b7ab8
     */
    NotePitcher(PhraseMgr *pPhraseMgr,
                Quantizer *pQuantizer,
                Sch::TickClock *pClock,
                const TrackData *pTrackData,
                int bPlayModeOne,
                int bAllowOwnedBars,
                int nUnreadOption);

    /**
     * @ghidraAddress NTSC-U/C: 0x001b39c0
     * @ghidraAddress PAL: 0x001b9798
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
     * @ghidraAddress NTSC-U/C: 0x001b3b98
     * @ghidraAddress PAL: 0x001b9970
     */
    virtual int Tick(int nElapsedTicks);

protected:
    /**
     * React to a PitchRiffMsg for this track and player.
     *
     * The message's position is quantised. A bar CanPlayBar() rejects plays `SND_INACTIVE`.
     * Otherwise, at a new position, the riff TrackData::GetRiff() reports goes out as a
     * MultiMuseMsg, PostPhraseCapturedMsg() records the gem, a PitchMsg follows, and the position
     * is stored in mLastPitchPosition.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x001b1f10
     * @ghidraAddress PAL: 0x001b7ce8
     */
    void PostPitchMsg(PitchRiffMsg *pMsg);

    /**
     * Erase this player's phrases at an EraseMsg's position, outside play mode 1.
     *
     * The bar, or with the message's last word set every bar of its step, is cleared wherever
     * mPlayer is its owner, and clearing the message's bar also sends an AllNotesOffMsg. When
     * anything was cleared, `SND_ERASE_SECTION` or `SND_ERASE` plays, a ShowEraseEffectMsg goes
     * out, and the seeker is posted again. The position is stored in mLastErasePosition in both
     * cases.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x001b20b0
     * @ghidraAddress PAL: 0x001b7e88
     */
    void PostAllNotesOffMsg(EraseMsg *pMsg);

    /**
     * Install the player a TrackSelectMsg for this track selects.
     *
     * A null player first turns the previous player's seeker off. A real player has its seeker
     * posted at the message's bar with the force flag set.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x001b22f0
     * @ghidraAddress PAL: 0x001b80c8
     */
    void PostSeekerMsg(TrackSelectMsg *pMsg);

    /**
     * Record a caught gem.
     *
     * A new bar becomes mCapturedBar and is announced with a PhraseCapturedMsg worth the bar's
     * points. In play mode 1 a phrase another player owns there is cleared first. Unless
     * Player::IsLooping() reports non-zero, the gem goes to PhraseMgr::AddGem() for mCapturedBar
     * alone. Otherwise it goes to every bar of the step that CanPlayBar() accepts and whose phrase
     * PhraseMgr::PhrasesMatch() pairs with mCapturedBar, and then to mCapturedBar itself. The bar
     * is then replayed from one tick after the gem.
     *
     * @param nGem The gem, the PitchRiffMsg's first word.
     * @param nTick The quantised song position.
     * @ghidraAddress NTSC-U/C: 0x001b2400
     * @ghidraAddress PAL: 0x001b81d8
     */
    void PostPhraseCapturedMsg(int nGem, int nTick);

    /**
     * Post the seeker for mPlayer at the first playable bar of the eight from nBar.
     *
     * The routine does not send a message for the stand-in player. Without bForce, a player whose
     * Player::GetPlace() reports non-zero has its seeker turned off. Outside play mode 1, a player
     * whose Player::IsLooping() reports zero, or a search that does not find a bar CanPlayBar()
     * accepts, also turns the seeker off. A found bar posts a seeker over the mStepBars bars of its
     * step.
     *
     * @param nBar The bar to search from, clamped to zero.
     * @param bForce Non-zero to skip the Player::GetPlace() test.
     * @ghidraAddress NTSC-U/C: 0x001b2710
     * @ghidraAddress PAL: 0x001b84e8
     */
    void PostSeekerMsgSecond(int nBar, int bForce);

    /**
     * Act on a message.
     *
     * Primary table slot 3.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x001b3bd0
     * @ghidraAddress PAL: 0x001b99a8
     */
    virtual void DispatchPriv(Message *pMsg);

private:
    // The out-of-line copy of the InvalidateSeekerMsg branch DispatchPriv() expands inline.
    // NTSC-U/C: 0x001b3a38, PAL: 0x001b9810
    void OnInvalidateSeeker(InvalidateSeekerMsg *pMsg);

    // Returns non-zero when nTick differs from mLastPitchPosition. PostPitchMsg() calls it at
    // NTSC-U/C 0x001b1fa4 (PAL 0x001b7d7c).
    // NTSC-U/C: 0x001b3b88, PAL: 0x001b9960
    int IsOtherTick(int nTick);

    // Reports whether mPlayer may play nBar. In play mode 1 that is TrackData::QueryBar() and
    // Player::IsFreestyleBar(). Otherwise the bar needs TrackData::QueryBar() and, unless
    // mAllowOwnedBars is set, must lack an owner or equal nCurrentBar. An owned bar must also
    // belong to mPlayer.
    // PostSeekerMsgSecond() expands it inline, and PostPitchMsg() and PostPhraseCapturedMsg() call
    // the out-of-line copy. The title is inferred.
    // NTSC-U/C: 0x001b3a68, PAL: 0x001b9840
    int CanPlayBar(int nBar, int nCurrentBar);

    PhraseMgr *mPhraseMgr; // +0x38
    Quantizer *mQuantizer; // +0x3c
    // Copied from the track description's `+0x04`. Matched against an InvalidateSeekerMsg's
    // `+0x08`, so it identifies the track this pitcher serves.
    int mTrack;                   // +0x40
    Player *mPlayer;              // +0x44, starts at NullPlayer::sInstance
    Sch::Tick mLastErasePosition; // +0x48, Sch::Tick(-1), stored and then tested at 0x001b1e60
    Sch::Tick mLastPitchPosition; // +0x4c, starts at kTickInfinity
    // Copied from the phrase manager's `+0x34` after an initial kTickInfinity. Turns an elapsed
    // tick count into a bar index.
    int mBarDivisor;             // +0x50
    int mCapturedBar;            // +0x54, the bar last announced as captured, -1 at first
    int mPlayModeOne;            // +0x58, the constructor's fifth argument
    int mStepBars;               // +0x5c, the spacing of a step's bars, 2 at first
    int mAllowOwnedBars;         // +0x60, the constructor's sixth argument
    int mUnreadOption;           // +0x64, the constructor's seventh argument, never read
    const TrackData *mTrackData; // +0x68
    Sch::TickClock *mClock;      // +0x6c
};
