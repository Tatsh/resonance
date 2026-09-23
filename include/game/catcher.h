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
#include "msg/trackselectmsg.h"
#include "sch/cmdid.h"
#include "sch/tick.h"
#include "sch/tickclock.h"

/**
 * Catcher that scores the gems one track presents.
 *
 * `7Catcher` in the RTTI descriptor at `0x00902010`, with GenericCatcher as its only base at
 * offset 0. Its primary table is at `0x007e0c38` with eleven entries and its MsgSource subobject
 * table at `0x007e0c10` with four. Two classes derive from it, MultiCatcher and SingleCatcher, and
 * both retain eleven entries, so neither introduces a virtual.
 *
 * The class supplies MsgSink::HandleMessage() and GenericCatcher's slots 4, 5, and 6, and it
 * introduces four virtuals of its own at slots 7 through 10. Slots 9 and 10 address the shared
 * pure-virtual stub at `0x005381a8`, so this class is abstract and the two subclasses exist to
 * supply them. The object is 0x80 bytes, which MultiCatcher's tagged allocation measures;
 * SingleCatcher's is 0x84, so that class adds one word.
 *
 * The constructor's parameter list is attested rather than inferred. The anonymous-namespace marker
 * for the file-local command class `GemCmd` records the enclosing constructor's mangled signature
 * as `__7CatcherP9PhraseMgrP9QuantizerPC9TrackDataPQ23Sch9TickClockiGQ23Sch4Tick`, which is
 * `Catcher(PhraseMgr *, Quantizer *, const TrackData *, Sch::TickClock *, int, Sch::Tick)`. A
 * second marker records `PostGemCmd` in the same translation unit. Both are the commands slot 4
 * schedules.
 *
 * The constructor stores only the low word of its Sch::Tick parameter, with `sw` rather than `sd`.
 * That is recorded here as measured rather than explained, and it belongs with the note in
 * `sch/tick.h` that two measurements of that type's member count disagree.
 *
 * A few bodies are not written yet. PostPhraseMuffedMsg() and PostCaughtBarMsg() build messages
 * whose classes have no constructor that takes a payload, and HandleMessage() reads a
 * CatchProgressPacket's private members. Each such routine is described where it is declared.
 * HandleMessage() dispatches a PitchRiffMsg to PostCatchMsg(), a TrackSelectMsg to
 * OnTrackSelect(), an AutoCatchMsg to OnAutoCatch(), and an InvalidateSeekerMsg to the inline
 * copy of OnInvalidateSeeker(). A CatchProgressPacket for this track stores its player, success
 * rate, and position in mRemotePlayer, mRemoteSuccess, and mRemotePosition.
 *
 * The two file-local commands, PostGemCmd and GemCmd, sit in the anonymous namespace the RTTI
 * records for this unit and call ProcessGemCommand() and SimulateRemoteGem().
 *
 * The table diff corrects an earlier attribution. `0x001adb78`, `0x001b15a8`, `0x001b1610`,
 * `0x001b19a0`, `0x001abe50`, and `0x001abfd8` were titled for MultiCatcher and sit in this
 * class's own table at slots 3, 4, 5, 6, 7, and 8. SingleCatcher's table holds the same six
 * addresses at the same indices, which confirms it independently of MultiCatcher's.
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
     * Withdraw both commands through slot 5 of this class.
     *
     * @ghidraAddress 0x001abc10
     */
    virtual ~Catcher();

    /**
     * Act on a message.
     *
     * Slot 3. The routine dispatches on the message's registered identity over the five
     * identities the class documentation lists, and every other message is discarded. The body
     * is not written, for the reason recorded in the class documentation.
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
    virtual void Slot4();

    /**
     * Withdraw the catcher's two commands from the clock.
     *
     * Slot 5. The two withdrawals mirror slot 4, the second being conditional on kGameModeNet in
     * the same way.
     *
     * @ghidraAddress 0x001b1610
     */
    virtual void Slot5();

    /**
     * Report whether the catcher has nothing outstanding.
     *
     * Slot 6.
     *
     * @return Non-zero when the counter at `+0x60` is zero.
     * @ghidraAddress 0x001b19a0
     */
    virtual int Slot6();

    /**
     * Record a missed gem.
     *
     * Slot 7. The routine plays the miss sound for the player's own slot, `SND_MISS_PLAYER1`
     * through `SND_MISS_PLAYER4` by Player::Slot2(), and plays nothing for any other slot. It then
     * sends a missed CatchMsg for the gem. When the tick's bar matches the bar at `+0x5c`, or the
     * counter at `+0x60` is positive, it records the tick in mUnknown48, increments the counter
     * at `+0x54`, clears the counter at `+0x60`, reports the muffed phrase, and refreshes the
     * seeker from the bar.
     *
     * @param nTick The scheduler time of the miss.
     * @param nGem The gem the CatchMsg reports.
     * @ghidraAddress 0x001abe50
     */
    virtual void Slot7(int nTick, int nGem);

    /**
     * Record a caught gem.
     *
     * Slot 8. The routine records the tick in mUnknown44, counts the gem in mUnknown50, and sends
     * the gem's riff as a MultiMuseMsg. While no gem of the phrase was missed or muffed, it totals
     * the gems and points of the bars from the phrase's first bar up to mSeekerEndBar, and the
     * first caught gem of a phrase sends a BeginPhraseCatchMsg with the points and the multiplier
     * Player::Slot16() reports. It then sends a caught CatchMsg with the progress through the
     * phrase and a GemMsg, calls Player::Slot14() and discards the result, and posts the
     * caught-bar message when the next gem lies in another bar.
     *
     * @param nTick The scheduler time of the gem.
     * @param nGem The gem.
     * @ghidraAddress 0x001abfd8
     */
    virtual void Slot8(int nTick, int nGem);

    /**
     * Slot 9, pure. MultiCatcher scores a range of bars and SingleCatcher scores one.
     *
     * @param nFirst The first word.
     * @param nSecond The second word.
     * @param nThird The third word. Both subclasses branch on whether it is zero.
     */
    virtual void Slot9(int nFirst, int nSecond, int nThird) = 0;

    /**
     * Slot 10, pure. MultiCatcher's override is empty and SingleCatcher's sends a
     * CaughtPowerbarMsg to its player when the phrase manager reports a value other than -1.
     */
    virtual void Slot10() = 0;

    /**
     * Run the post-gem command at a song position.
     *
     * The PostGemCmd the class schedules calls it. A position other than mUnknown44 counts a
     * muffed gem (mUnknown58 incremented, mUnknown60 cleared, the muffed phrase reported, the
     * seeker refreshed from mUnknown64, and slot 15 of mPlayer invoked). The routine then ends the
     * bar when the next gem falls in a later bar and schedules the next post-gem command.
     *
     * @param nTick The song position the command was scheduled for.
     * @ghidraAddress 0x001b1828
     */
    void ProcessGemCommand(int nTick);

    /**
     * Play the remote player's gem at a song position, then schedule the next gem command.
     *
     * The GemCmd the class schedules calls it. A gem is played only when mRemotePlayer is not the
     * stand-in, reports -1 from Player::Slot2(), is within one bar of mRemotePosition, and a draw
     * from the C library's rand() modulo 256 falls below mRemoteSuccess times 256. Playing it sends
     * the riff of the gem as a MultiMuseMsg when TrackData::GetRiff() reports one, then a caught
     * CatchMsg and a GemMsg for mRemotePlayer.
     *
     * @param nTick The song position the command was scheduled for.
     * @ghidraAddress 0x001ace78
     */
    void SimulateRemoteGem(int nTick);

protected:
    // Report a muffed phrase.
    // The body is not written, for the reason recorded in the class documentation.
    // 0x001ad3b0
    void PostPhraseMuffedMsg(int nBar, int nTick);

    // Report a caught gem.
    // HandleMessage() calls it for a PitchRiffMsg (the identity at 0x006d0134). The routine
    // ignores the call unless the two counters at `+0x54` and `+0x58` are both zero. The body is
    // not written, for the reason recorded in the class documentation.
    // 0x001ac370
    void PostCatchMsg(Message *pMsg);

    // Report a caught bar.
    // The body is not written, for the reason recorded in the class documentation.
    // 0x001ac7e8
    void PostCaughtBarMsg(int nBar, int nNextBar);

    // Returns the gem on either side of nTick nearer to it when that gem lies within
    // mCatchWindow, and nTick otherwise. PostCatchMsg() is the caller.
    // 0x001abcf8
    int SnapToNearestGem(int nTick);

    // Takes the track for the player a TrackSelectMsg names.
    // 0x001ac550
    void OnTrackSelect(TrackSelectMsg *pMsg);

    // Plays a free bar for the player an AutoCatchMsg names through slot 9, marks the message
    // handled through CmdMsg::mUnknown04, and replays the bar from the current offset through
    // PhraseMgr::ReplayBar() when the song is inside it.
    // 0x001ac688
    void OnAutoCatch(AutoCatchMsg *pMsg);

    // Closes the counts for a bar. A muffed phrase is reported for the previous bar when gems
    // were missed or muffed and some were caught.
    // 0x001ac958
    void EndBar(int nBar);

    // Returns the first gem position after nTick. When none lies within mUnknown30 bars of the
    // track, returns nTick plus that many bars less one.
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

    // Finds the next free bar within 32 bars of nBar (or of the bar after mUnknown64) and points
    // the seeker at the phrase starting there, or turns the seeker off. Nothing happens while
    // mPlayer is the stand-in or reports -1 from Player::Slot2().
    // 0x001ad0e8
    void UpdateSeeker(int nBar);

    // Sends a SeekerMsg that turns the seeker off and clears mSeekerEnabled.
    // 0x001ad4e0
    void PostSeekerMsg();

    // Sends a SeekerMsg for nBarCount bars from nFirstBar at position 0, and records the
    // range in mSeekerFirstBar, mSeekerEndBar, and mSeekerEnabled.
    // 0x001ad560
    void PostSeekerRangeMsg(int nFirstBar, int nBarCount);

    // Returns whether a bar is non-negative, playable according to TrackData::QueryBar(), and
    // owned by no player. UpdateSeeker() and OnAutoCatch() expand the same test inline.
    // 0x001b1488
    int IsBarFree(int nBar);

    // The out-of-line copy of the InvalidateSeekerMsg branch HandleMessage() expands inline.
    // 0x001b1578
    void OnInvalidateSeeker(InvalidateSeekerMsg *pMsg);

    // Gives every bar from nFirstBar up to nEndBar to one player. The image has no caller.
    // 0x001b1918
    void SetPhraseOwners(int nFirstBar, int nEndBar, Player *pPlayer);

    Quantizer *mQuantizer;       // +0x18
    PhraseMgr *mPhraseMgr;       // +0x1c
    const TrackData *mTrackData; // +0x20
    Player *mPlayer;             // +0x24, the file-scope NullPlayer until one is assigned
    Sch::TickClock *mClock;      // +0x28
    CmdID mPostGemCommand;       // +0x2c
    CmdID mGemCommand;           // +0x30
    Mid::MBT mTicksPerBar;       // +0x34, copied from the phrase manager and used as a divisor
    int mSeekerBarCount;         // +0x38, the constructor's int parameter
    int mCatchWindow;            // +0x3c, the low word of the constructor's Sch::Tick parameter
    int mUnknown40;              // +0x40, always 1
    Mid::MBT mUnknown44;         // +0x44, the position of the last caught gem
    Mid::MBT mUnknown48;         // +0x48, slot 7 stores the missed tick
    int mTrack;                  // +0x4c, copied from TrackData::mUnknown04
    int mUnknown50;              // +0x50, slot 8 increments it
    int mUnknown54;              // +0x54, slot 7 increments it
    int mUnknown58;              // +0x58, ProcessGemCommand() increments it
    int mUnknown5c;              // +0x5c, the bar EndBar() closed last
    int mUnknown60;              // +0x60, the bars UpdateSeeker() looks back
    int mUnknown64;              // +0x64, starts -1
    int mSeekerFirstBar;         // +0x68
    int mSeekerEndBar;           // +0x6c
    int mSeekerEnabled;          // +0x70
    Player *mRemotePlayer;       // +0x74, from a CatchProgressPacket
    float mRemoteSuccess;        // +0x78, the packet's success rate
    Mid::MBT mRemotePosition;    // +0x7c, the packet's position
};
