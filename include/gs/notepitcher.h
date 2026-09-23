#pragma once

#include "gs/pitcher.h"
#include "mid/mbt.h"
#include "msg/message.h"

class InvalidateSeekerMsg;
class PhraseMgr;
class Player;
class Quantizer;
class TrackData;

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
 * Only HandleMessage() and Tick() are written. The five routines they dispatch to and the
 * constructor are declared with their addresses and their bodies are not written. The five titles
 * come from the message each routine posts rather than from the message it receives, which is the
 * naming the program already had and is retained here.
 */
class NotePitcher : public Pitcher {
public:
    /**
     * Construct a note pitcher for one track.
     *
     * The seven arguments arrive in a1 through a3 and t0 through t3, which is the register
     * convention this target uses for arguments five through eight. The body is not written.
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
     * React to a PitchRiffMsg. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001b1f10
     */
    void PostPitchMsg(Message *pMsg);

    /**
     * React to an EraseMsg. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001b20b0
     */
    void PostAllNotesOffMsg(Message *pMsg);

    /**
     * React to a TrackSelectMsg. The body is not written.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001b22f0
     */
    void PostSeekerMsg(Message *pMsg);

    /**
     * Post the phrase-captured message. The body is not written.
     *
     * No caller is recovered. It is declared because the program already titles it and the address
     * belongs to this class.
     *
     * @ghidraAddress 0x001b2400
     */
    void PostPhraseCapturedMsg();

    /**
     * Post the seeker message for one bar. The body is not written.
     *
     * Both an InvalidateSeekerMsg and Tick() reach it, the first with the message's `+0x04` and
     * the second with the bar it computed. The second argument is zero at both call sites.
     *
     * @param nBar The bar.
     * @param bUnknown Zero at both recovered call sites.
     * @ghidraAddress 0x001b2710
     */
    void PostSeekerMsgSecond(int nBar, int bUnknown);

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

    PhraseMgr *mPhraseMgr; // +0x38
    Quantizer *mQuantizer; // +0x3c
    // Copied from the track description's `+0x04`. Matched against an InvalidateSeekerMsg's
    // `+0x08`, so it identifies the track this pitcher serves.
    int mUnknown40;      // +0x40
    Player *mUnknown44;  // +0x44, starts at g_nullPlayer
    int mUnknown48;      // +0x48, starts at -1
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
