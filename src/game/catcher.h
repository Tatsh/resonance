#pragma once

#include "game/genericcatcher.h"
#include "game/player.h"
#include "game/quantizer.h"
#include "game/trackdata.h"
#include "gs/phrasemgr.h"
#include "mid/mbt.h"
#include "msg/autocatchmsg.h"
#include "msg/invalidateseekermsg.h"
#include "msg/message.h"
#include "msg/pitchriffmsg.h"
#include "msg/trackselectmsg.h"
#include "sch/cmdid.h"
#include "sch/tick.h"
#include "sch/tickclock.h"

/**
 * Catcher that scores the gems one track presents.
 *
 * Its RTTI descriptor is at `0x00902010`. It has GenericCatcher as its only base at offset 0. Its
 * primary table is at `0x007e0c38` with eleven entries and its MsgSource subobject table at
 * `0x007e0c10` with four. Two classes derive from it, MultiCatcher and SingleCatcher, and both
 * retain eleven entries. Neither introduces a virtual.
 *
 * The class supplies MsgSink::HandleMessage() and GenericCatcher's Start(), Stop(), and
 * IsPhraseRunEmpty(), and it introduces four virtuals at slots 7 through 10.
 * CapturePhrase() and ReportCaughtPowerbar(), slots 9 and 10, address the shared pure-virtual stub
 * at `0x005381a8`. This class is therefore abstract, and the two subclasses exist to supply them.
 * The object is 0x80 bytes, as MultiCatcher's tagged allocation measures. SingleCatcher's is 0x84
 * and adds one word.
 *
 * The constructor's parameter list is attested rather than inferred. The anonymous-namespace marker
 * for the file-local command class `GemCmd` records the enclosing constructor's signature,
 * `Catcher(PhraseMgr *, Quantizer *, const TrackData *, Sch::TickClock *, int, Sch::Tick)`. A
 * second marker records `PostGemCmd` in the same translation unit. Both are the commands Start()
 * schedules.
 *
 * The constructor stores only the low word of its Sch::Tick parameter, with `sw` rather than `sd`.
 * That is recorded here as measured rather than explained, and it belongs with the note in
 * `sch/tick.h` that two measurements of that type's member count disagree.
 *
 * HandleMessage() dispatches a PitchRiffMsg to PostCatchMsg(), a TrackSelectMsg to
 * OnTrackSelect(), an AutoCatchMsg to OnAutoCatch(), and an InvalidateSeekerMsg to the inline
 * copy of OnInvalidateSeeker(). A CatchProgressPacket for this track stores its player, success
 * rate, and position in mRemotePlayer, mRemoteSuccess, and mRemotePosition.
 *
 * The two file-local commands, PostGemCmd and GemCmd, sit in the anonymous namespace the RTTI
 * records for this unit and call ProcessGemCommand() and SimulateRemoteGem().
 *
 * `0x001adb78`, `0x001b15a8`, `0x001b1610`, `0x001b19a0`, `0x001abe50`, and `0x001abfd8` sit in
 * this class's table at slots 3, 4, 5, 6, 7, and 8. SingleCatcher's table has the same six
 * addresses at the same indices.
 */
class Catcher : public GenericCatcher {
public:
    /**
     * @param pPhraseMgr The phrase manager for the track.
     * @param pQuantizer The quantiser for the track.
     * @param pTrackData The track description.
     * @param pClock The clock the scheduled commands run on.
     * @param nSeekerBarCount The bars each seeker range spans. MultiCatcher passes 1.
     * @param catchWindow The catch window. Only the low word is retained, as MIDI ticks.
     * @ghidraAddress 0x001aba30
     */
    Catcher(PhraseMgr *pPhraseMgr,
            Quantizer *pQuantizer,
            const TrackData *pTrackData,
            Sch::TickClock *pClock,
            int nSeekerBarCount,
            Sch::Tick catchWindow);

    /**
     * Withdraw both commands through Stop().
     *
     * @ghidraAddress 0x001abc10
     */
    virtual ~Catcher();

    /**
     * Act on a message.
     *
     * Slot 3. The routine dispatches on the message's registered identity over the five
     * identities the class documentation lists, and every other message is discarded.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001adb78
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Schedule the catcher's two commands on the clock.
     *
     * Slot 4. The post-gem command is always scheduled and the gem command only in kGameModeNet.
     *
     * @ghidraAddress 0x001b15a8
     */
    virtual void Start();

    /**
     * Withdraw the catcher's two commands from the clock.
     *
     * Slot 5. The two withdrawals mirror Start(), the second being conditional on kGameModeNet in
     * the same way.
     *
     * @ghidraAddress 0x001b1610
     */
    virtual void Stop();

    /**
     * Report whether the catcher has nothing outstanding.
     *
     * Slot 6.
     *
     * @return Non-zero when mPhraseRunBars is zero.
     * @ghidraAddress 0x001b19a0
     */
    virtual int IsPhraseRunEmpty();

    /**
     * Record a missed gem.
     *
     * Slot 7. The routine plays the miss sound for the player's own slot, `SND_MISS_PLAYER1`
     * through `SND_MISS_PLAYER4` by Player::GetInputSlot(), and plays nothing for any other slot.
     * It then sends a missed CatchMsg for the gem. When the tick's bar matches mLastEndedBar, or
     * mPhraseRunBars is positive, it records the tick in mLastMissedPosition, increments
     * mMissedGems, clears mPhraseRunBars, reports the muffed phrase, and refreshes the seeker from
     * the bar.
     *
     * @param nTick The scheduler time of the miss.
     * @param nGem The gem the CatchMsg reports.
     * @ghidraAddress 0x001abe50
     */
    virtual void MissGem(int nTick, int nGem);

    /**
     * Record a caught gem.
     *
     * Slot 8. The routine records the tick in mLastCaughtPosition, counts the gem in mCaughtGems,
     * and sends the gem's riff as a MultiMuseMsg. While no gem of the phrase was missed or muffed,
     * it totals the gems and points of the bars from the phrase's first bar up to mSeekerEndBar,
     * and the first caught gem of a phrase sends a BeginPhraseCatchMsg with the points and the
     * multiplier Player::GetMultiplier() reports. It then sends a caught CatchMsg with the progress
     * through the phrase and a GemMsg, calls Player::CountCaughtGem() and discards the result, and
     * posts the caught-bar message when the next gem lies in another bar.
     *
     * @param nTick The scheduler time of the gem.
     * @param nGem The gem.
     * @ghidraAddress 0x001abfd8
     */
    virtual void CatchGem(int nTick, int nGem);

    /**
     * Capture the phrase a caught run of bars completes. Slot 9, pure. SingleCatcher captures the
     * whole step around the bar and MultiCatcher the bar alone.
     *
     * @param nBar The last bar of the run.
     * @param nRun The bars the run spans.
     * @param nAutoCatch Non-zero for an automatic catch. Both subclasses branch on whether it is
     *                   zero.
     */
    virtual void CapturePhrase(int nBar, int nRun, int nAutoCatch) = 0;

    /**
     * Report a caught bar's power bar. Slot 10, pure. MultiCatcher's override is empty and
     * SingleCatcher's sends a CaughtPowerbarMsg to its player when the phrase manager reports a
     * value other than -1.
     *
     * @param nBar The caught bar. SingleCatcher's override passes it to PhraseMgr::GetPowerbar().
     */
    virtual void ReportCaughtPowerbar(int nBar) = 0;

    /**
     * Run the post-gem command at a song position.
     *
     * The PostGemCmd the class schedules calls it. A position other than mLastCaughtPosition
     * counts a muffed gem (mMuffedGems incremented, mPhraseRunBars cleared, the muffed phrase
     * reported, the seeker refreshed from mLastMuffedBar, and slot 15 of mPlayer invoked). The
     * routine then ends the bar when the next gem falls in a later bar and schedules the next
     * post-gem command.
     *
     * @param nTick The song position the command was scheduled for.
     * @ghidraAddress 0x001b1828
     */
    void ProcessGemCommand(int nTick);

    /**
     * Play the remote player's gem at a song position, then schedule the next gem command.
     *
     * The GemCmd the class schedules calls it. A gem is played only when mRemotePlayer is not the
     * stand-in, reports -1 from Player::GetInputSlot(), is within one bar of mRemotePosition, and a
     * draw from the C library's rand() modulo 256 falls below mRemoteSuccess times 256. Playing it
     * sends the riff of the gem as a MultiMuseMsg when TrackData::GetRiff() reports one, then a
     * caught CatchMsg and a GemMsg for mRemotePlayer.
     *
     * @param nTick The song position the command was scheduled for.
     * @ghidraAddress 0x001ace78
     */
    void SimulateRemoteGem(int nTick);

protected:
    // Sends a PhraseMuffedMsg for a bar once, recording the bar in mLastMuffedBar. The message
    // reports the phrase as tried when the bar is free and a gem of it was caught or missed.
    // 0x001ad3b0
    void PostPhraseMuffedMsg(int nBar, Mid::MBT position);

    // Scores a PitchRiffMsg for this track. A riff from another player queries that player's
    // slot 5 and plays SND_INACTIVE. A riff snapped into a bar that is not free plays SND_INACTIVE
    // and sends a missed CatchMsg. Otherwise a riff on the gem at the snapped position, other
    // than the last one caught, goes to CatchGem(), and every other riff to MissGem().
    // 0x001ac370
    void PostCatchMsg(PitchRiffMsg *pMsg);

    // Does nothing once a gem of the phrase was missed or muffed. Otherwise it delivers a
    // CaughtBarMsg to the player, extends the phrase run in mPhraseRunBars towards nNextBar
    // (bounded by the next step and mSeekerBarCount), and scores the run through CapturePhrase()
    // when it ends the seeker range.
    // 0x001ac7e8
    void PostCaughtBarMsg(int nBar, int nNextBar);

    // Returns the gem on either side of nTick nearer to it when that gem lies within
    // mCatchWindow, and nTick otherwise. PostCatchMsg() is the caller.
    // 0x001abcf8
    int SnapToNearestGem(int nTick);

    // Takes the track for the player a TrackSelectMsg names.
    // 0x001ac550
    void OnTrackSelect(TrackSelectMsg *pMsg);

    // Plays a free bar for the player an AutoCatchMsg identifies through CapturePhrase(), marks the
    // message handled through CmdMsg::mResult, and replays the bar from the current offset
    // through PhraseMgr::ReplayBar() when the song is inside it.
    // 0x001ac688
    void OnAutoCatch(AutoCatchMsg *pMsg);

    // Closes the counts for a bar. A muffed phrase is reported for the previous bar when gems
    // were missed or muffed and some were caught.
    // 0x001ac958
    void EndBar(int nBar);

    // Returns the first gem position after nTick. When none lies within TrackData::mGemSearchBars
    // bars of the track, returns nTick plus that many bars less one.
    // 0x001aca48
    int FindNextGemTick(int nTick);

    // Build a PostGemCmd for the gem after nTick and queue it one tick after PostGemDelay() under
    // the handle mPostGemCommand.
    // 0x001acba0
    void SchedulePostGemCommand(int nTick);

    // Build a GemCmd for the gem after nTick and queue it at that gem under the handle
    // mGemCommand.
    // 0x001acca0
    void ScheduleGemCommand(int nTick);

    // Returns the earlier of the midpoint between nTick and the next gem, and nTick plus
    // mCatchWindow.
    // 0x001acd30
    int PostGemDelay(int nTick);

    // Finds the next free bar within 32 bars of nBar (or of the bar after mLastMuffedBar) and
    // points the seeker at the phrase starting there, or turns the seeker off. It does nothing
    // while mPlayer is the stand-in or reports -1 from Player::GetInputSlot(). 0x001ad0e8
    void UpdateSeeker(int nBar);

    // Sends a SeekerMsg that turns the seeker off and clears mSeekerEnabled.
    // 0x001ad4e0
    void PostSeekerMsg();

    // Sends a SeekerMsg for nBarCount bars from nFirstBar at position 0, and records the
    // range in mSeekerFirstBar, mSeekerEndBar, and mSeekerEnabled.
    // 0x001ad560
    void PostSeekerRangeMsg(int nFirstBar, int nBarCount);

    // Returns whether a bar is non-negative, playable according to TrackData::QueryBar(), and
    // owned by no player. UpdateSeeker(), OnAutoCatch(), PostCatchMsg(), and
    // PostPhraseMuffedMsg() expand the same test inline.
    // 0x001b1488
    int IsBarFree(int nBar);

    // The out-of-line copy of the InvalidateSeekerMsg branch HandleMessage() expands inline.
    // 0x001b1578
    void OnInvalidateSeeker(InvalidateSeekerMsg *pMsg);

    // Gives every bar from nFirstBar up to nEndBar to one player. The image has no caller.
    // 0x001b1918
    void SetPhraseOwners(int nFirstBar, int nEndBar, Player *pPlayer);

    Quantizer *mQuantizer;        // +0x18
    PhraseMgr *mPhraseMgr;        // +0x1c
    const TrackData *mTrackData;  // +0x20
    Player *mPlayer;              // +0x24, the file-scope NullPlayer until one is assigned
    Sch::TickClock *mClock;       // +0x28
    CmdID mPostGemCommand;        // +0x2c
    CmdID mGemCommand;            // +0x30
    Mid::MBT mTicksPerBar;        // +0x34, copied from the phrase manager and used as a divisor
    int mSeekerBarCount;          // +0x38, the constructor's int parameter
    int mCatchWindow;             // +0x3c, the low word of the constructor's Sch::Tick parameter
    int mEnabled;                 // +0x40, always 1 and never read
    Mid::MBT mLastCaughtPosition; // +0x44
    Mid::MBT mLastMissedPosition; // +0x48
    int mTrack;                   // +0x4c, copied from TrackData::mIndex
    int mCaughtGems;              // +0x50, gems caught in the current bar
    int mMissedGems;              // +0x54, gems missed in the current bar
    int mMuffedGems;              // +0x58, gems passed without a riff in the current bar
    int mLastEndedBar;            // +0x5c, the bar EndBar() closed last
    int mPhraseRunBars;           // +0x60, the caught bars of the current run
    int mLastMuffedBar;           // +0x64, the bar PostPhraseMuffedMsg() reported last, starts -1
    int mSeekerFirstBar;          // +0x68
    int mSeekerEndBar;            // +0x6c
    int mSeekerEnabled;           // +0x70
    Player *mRemotePlayer;        // +0x74, from a CatchProgressPacket
    float mRemoteSuccess;         // +0x78, the packet's success rate
    Mid::MBT mRemotePosition;     // +0x7c, the packet's position
};
