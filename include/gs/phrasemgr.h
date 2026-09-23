#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "mid/mbt.h"
#include "msg/invalidatetrackmsg.h"
#include "msg/message.h"
#include "msg/phrasepacket.h"
#include "sch/cmdid.h"

class Phrase;
class PhraseDatabase;
class PhrasePlayer;
class Player;
class PowerbarMgr;
class PlayMap;
class TrackData;

namespace Sch {
class TickClock;
} // namespace Sch

/**
 * Owner of the gem phrases on one track, and the seam between the network and the gems.
 *
 * `9PhraseMgr` in the RTTI descriptor at `0x00901fd0`, with MsgSink at offset 0 and MsgSource at
 * offset 4, and titled after `GsPhraseMgr.cpp`, the translation unit its two file-local classes
 * record. Those two are `Cmd` at `0x008f0960` and `ExportCmd` at `0x009021f0`, both deriving from
 * Sch::Command. Its primary table is at `0x007e28d0` and its MsgSource table at `0x007e28a8`, and
 * each runs four entries. ScoreTrackGraph's tagged allocation measures the object at 0x60 bytes.
 * The mangled signature of `Catcher::Catcher()`,
 * `__7CatcherP9PhraseMgrP9QuantizerPC9TrackDataPQ23Sch9TickClockiGQ23Sch4Tick`, also records the
 * title, and every stage passes the manager it retains to the catcher it builds.
 *
 * Every member below posts one message through its own MsgSource half, which is what the titles
 * describe. PostPhraseMsg() looks the phrase up through PhraseDatabase::GetPhraseAt() on
 * mDatabase, and when that lookup reports a phrase it builds a PhraseMsg with the argument at
 * `+0x04`, mUnknown30 at `+0x08`, and the phrase at `+0x0c`, and delivers it through
 * MsgSource::Send().
 *
 * HandleMessage() dispatches six identities, three of them packets rather than messages. A
 * PhrasePacket, a CaughtPhrasePacket, and a GemPacket arrive from the network, and an
 * InvalidateTrackMsg, a RefreshNetMsg, and a GameBeginMsg arrive locally. The PhrasePacket and the
 * InvalidateTrackMsg paths both loop, clearing and posting bars through RefreshBar(), and the
 * PhrasePacket path is guarded on the packet's `+0x14` matching mUnknown30.
 *
 * The constructor fixes the member map from `+0x18` to the end. The MsgSource subobject occupies
 * `+0x04` through `+0x17`, so the region at `+0x10` the destructor tears down is that subobject's
 * vector rather than a member of this class. The destructor deletes mDatabase and mPowerbarMgr
 * through slot 1 of each table, which is the destructor slot.
 */
class PhraseMgr : public MsgSink, public MsgSource {
    // ScoreTrackGraph::GetPhraseDatabase() at 0x001cf978 reads mDatabase directly, the Catcher
    // constructor at 0x001aba30 copies mBarTicks, and PhrasePlayer maps a bar through mMap at
    // 0x001c1a28.
    friend class Catcher;
    friend class PhrasePlayer;
    friend class ScoreTrackGraph;

public:
    /**
     * Construct the manager of one track and its phrase database.
     *
     * The fifth argument arrives in `t1`. ScoreTrackGraph passes the song clock, 1920, the play
     * map Globals reports, a configuration value, and its track description. The constructor ends
     * by creating the powerbar source through CreatePowerbarMgr().
     *
     * @param pClock The song clock.
     * @param nBarTicks The length of one bar in MIDI ticks.
     * @param pMap The play map the phrase database is sized from.
     * @param nConfig The configuration value ScoreTrackGraph reads from the query at
     *                `0x00509110` with the identifier 0x2be.
     * @param pTrackData The track description.
     * @ghidraAddress 0x001ba0d0
     */
    PhraseMgr(Sch::TickClock *pClock,
              int nBarTicks,
              PlayMap *pMap,
              int nConfig,
              const TrackData *pTrackData);

    /**
     * Withdraw both commands through WithdrawCommands(), then delete both owned objects.
     *
     * @ghidraAddress 0x001ba2b8
     */
    virtual ~PhraseMgr();

    /**
     * Replace the powerbar source with the one the play mode and the track kind call for.
     *
     * kPlayModeGame on a riff or catch track, with configuration flag 0x3a1 clear, gets a
     * SoloPowerbarMgr in kGameModeSolo and a MultiPowerbarMgr in any other game mode. Every other
     * combination gets a JamPowerbarMgr. The constructor and CatchingSTG's slot 12 call it.
     *
     * @ghidraAddress 0x001ba3d8
     */
    void CreatePowerbarMgr();

    /**
     * @param nBar The bar, mapped through slot 5 of mMap.
     * @return PowerbarMgr::GetPowerbar() on mPowerbarMgr for the mapped bar.
     * @ghidraAddress 0x001c01e8
     */
    int GetPowerbar(int nBar);

    /**
     * Post a PhraseMsg for one phrase.
     *
     * Performs no work when the lookup at `0x001b8e50` reports nothing.
     *
     * @param nPhrase The value the message's `+0x04` receives.
     * @ghidraAddress 0x001bc468
     */
    void PostPhraseMsg(int nPhrase);

    /**
     * Post a GemMsg. The body is not written.
     *
     * The GemPacket path of HandleMessage() is the one recovered caller.
     *
     * @param pMsg The packet the gem is read out of.
     * @ghidraAddress 0x001ba6d0
     */
    void PostGemMsg(Message *pMsg);

    /**
     * Post the gems of one bar as GemMsg objects. The body is not written.
     *
     * RefreshBar() calls it for track modes 2 and 3.
     *
     * @param nBar The bar.
     * @ghidraAddress 0x001bc0f0
     */
    void PostGemMsgSecond(int nBar);

    /**
     * Post the gems of one bar as GemMsg objects, in the form RefreshBar() uses for track modes 1
     * and 5. The body is not written.
     *
     * @param nBar The bar.
     * @param nFlag A word RefreshBar() passes as 1.
     * @ghidraAddress 0x001bc290
     */
    void PostGemMsgThird(int nBar, int nFlag);

    /**
     * Post the gems of one bar as DurGemMsg objects. The body is not written.
     *
     * RefreshBar() calls it for track mode 4. At 0x2ac bytes it is the largest routine of the
     * class.
     *
     * @param nBar The bar.
     * @ghidraAddress 0x001bbcf0
     */
    void PostDurGemMsg(int nBar);

    /**
     * Post one bar again when it lies in the window from mWindowStart up to mWindowEnd.
     *
     * The body is not written, because it sends a ClearGemsMsg built on the stack when bClear is
     * non-zero. It then posts the bar's status through PostBarStatusMsg() and the bar's gems
     * through the routine mTrackKind selects from the jump table at `0x007e2670`.
     *
     * @param nBar The bar.
     * @param bClear Non-zero to clear the bar's gems first.
     * @ghidraAddress 0x001bbb90
     */
    void RefreshBar(int nBar, int bClear);

    /**
     * Post a BarStatusMsg for one bar. The body is not written.
     *
     * RefreshBar() is the recovered caller.
     *
     * @param nBar The bar.
     * @ghidraAddress 0x001bb9f8
     */
    void PostBarStatusMsg(int nBar);

    /**
     * Play one bar through mPhrasePlayer and schedule the file-local Cmd for the next bar.
     *
     * The file-local Cmd runs it.
     *
     * @param nBar The bar to play.
     * @ghidraAddress 0x001bb6b8
     */
    void OnCommand(int nBar);

    /**
     * Move the window to start before a bar, post the bar entering it, and schedule the file-local
     * ExportCmd for the next bar, mExportLead ticks after that bar starts.
     *
     * The file-local ExportCmd runs it.
     *
     * @param nBar The bar.
     * @ghidraAddress 0x001bb8a0
     */
    void OnExportCommand(int nBar);

    /**
     * @param nBar The bar. PhrasePlayer passes the bar it plays.
     * @return PhraseDatabase::GetPhraseAt() on mDatabase.
     * @ghidraAddress 0x001c01c8
     */
    Phrase *GetPhraseAt(int nBar);

    /**
     * @param nBar The bar. PhrasePlayer passes the bar it plays.
     * @return PhraseDatabase::GetStepValue() on mDatabase.
     * @ghidraAddress 0x001c0248
     */
    long *GetStepValue(int nBar);

    /**
     * @param nBar The bar, mapped through slot 5 of mMap.
     * @return The byte at `+0x28` of the phrase at the mapped bar.
     * @ghidraAddress 0x001c0338
     */
    unsigned char GetPhraseByte(int nBar);

    /**
     * Set the byte at `+0x28` of the phrase at a bar and at every bar slot 7 of mMap chains it to
     * for this track.
     *
     * @param nBar The bar, mapped through slot 5 of mMap.
     * @param cValue The byte.
     * @ghidraAddress 0x001c0298
     */
    void SetPhraseByte(int nBar, char cValue);

    /**
     * Return every phrase to one owner and clear and post every bar of the window again.
     *
     * @param pPlayer The owner.
     * @ghidraAddress 0x001c0380
     */
    void ResetOwners(Player *pPlayer);

    /**
     * Widen the window to the first mConfig + 1 bars and post each of them again.
     *
     * mRefreshing is set while the bars are posted.
     *
     * @ghidraAddress 0x001c03e0
     */
    void RefreshAllBars();

    /**
     * @param nTick The song position, in MIDI ticks.
     * @return The bar the position falls in. The start of that bar is computed and discarded.
     * @ghidraAddress 0x001bf6c0
     */
    int TickToBar(int nTick);

    /**
     * @param nBar The bar.
     * @return The song position the bar starts at.
     * @ghidraAddress 0x001bf738
     */
    int BarToTick(int nBar);

    /**
     * Report whether the phrases at two bars carry the same gems.
     *
     * Two equal bars match at once. Otherwise both bars are mapped through slot 5 of mMap, two
     * missing phrases match, one missing phrase does not, and two phrases match when their gem
     * lists are equal element by element. NotePitcher::PostPhraseCapturedMsg() is the recovered
     * caller.
     *
     * @param nFirstBar The first bar.
     * @param nSecondBar The second bar.
     * @return Non-zero when the phrases match.
     * @ghidraAddress 0x001bb558
     */
    int PhrasesMatch(int nFirstBar, int nSecondBar);

    /**
     * Have mPhrasePlayer play a bar again from an offset, at the song position it has reached.
     *
     * Catcher::OnAutoCatch() and NotePitcher::PostPhraseCapturedMsg() are the recovered callers.
     *
     * @param nBar The bar.
     * @param nOffset The offset within the bar, in MIDI ticks.
     * @ghidraAddress 0x001bb798
     */
    void ReplayBar(int nBar, int nOffset);

    /**
     * Install a phrase at a bar, report the owner change, and optionally post the bar again.
     *
     * The body is not written, because it sends a message built on the stack through the sink at
     * mNetSink. It first calls the Globals routine at `0x00118d40` and the GrooveWorld routine
     * at `0x00195378` and discards both results. AxePhraseMaker and Voxer are the recovered
     * callers.
     *
     * @param pPhrase The phrase.
     * @param nBar The bar, mapped through slot 5 of mMap.
     * @param bRefresh Non-zero to post the bar through RefreshBar() afterwards.
     * @ghidraAddress 0x001bb1a0
     */
    void InstallPhrase(Phrase *pPhrase, int nBar, int bRefresh);

    /**
     * Clear the phrase at a bar and at every bar slot 7 of mMap chains it to, and post the
     * affected window bars again.
     *
     * The body is not written, because it sends a message built on the stack through the sink at
     * mNetSink for each cleared bar. The chain is followed only in kPlayModeGame with bAll set.
     * AxePhraseMaker, NotePitcher, and PhraseNeutralizer are the recovered callers.
     *
     * @param nBar The bar, mapped through slot 5 of mMap.
     * @param bAll Non-zero to clear every chained bar as well.
     * @ghidraAddress 0x001bb328
     */
    void ClearPhrase(int nBar, int bAll);

    /**
     * Give the phrase of a CaughtPhrasePacket for this track to the player the packet identifies,
     * or clear it, and post the chained window bars again.
     *
     * The body is not written, because CaughtPhrasePacket declares its payload private.
     * HandleMessage() is the recovered caller.
     *
     * @param pMsg The packet.
     * @ghidraAddress 0x001ba540
     */
    void OnCaughtPhrasePacket(Message *pMsg);

    /**
     * Schedule the file-local Cmd for bar 0 at position 0 and the file-local ExportCmd for bar 1,
     * mExportLead ticks after that bar starts.
     *
     * ScoreTrackGraph's slot 2 is the recovered caller.
     *
     * @ghidraAddress 0x001bc588
     */
    void StartCommands();

    /**
     * Add a gem to the phrase at a bar, creating the phrase for an owner when the bar has none.
     *
     * The body is not written, because it sends a message built on the stack through the sink at
     * mNetSink. It first calls the Globals routine at `0x00118d40` and the GrooveWorld routine
     * at `0x00195378` and discards both results, gives a bar without a phrase to pOwner through
     * SetPhraseOwner(), and adds the gem through Phrase::AddGem(). NotePitcher and Scratcher are
     * the recovered callers.
     *
     * @param nGem The gem.
     * @param nTrans The transposition.
     * @param nBar The bar, mapped through slot 5 of mMap.
     * @param nTick The song position within the phrase, in MIDI ticks.
     * @param pOwner The player a new phrase is given to.
     * @param nUnknown A word the rest of the body uses. Both NotePitcher calls pass a member.
     * @ghidraAddress 0x001baa98
     */
    void AddGem(int nGem, int nTrans, int nBar, int nTick, Player *pOwner, int nUnknown);

    /**
     * Withdraw both scheduled commands. The destructor calls it first.
     *
     * @ghidraAddress 0x001c0450
     */
    void WithdrawCommands();

    /**
     * Install the phrase a PhrasePacket for this track carries at its step, or clear the step when
     * the packet has none, and post the window bar mapped to that step again.
     *
     * The body is not written, because PhrasePacket declares `+0x14` and `+0x1c` private. When the
     * owner changes, TrackData::SetOwner() receives the owner PhraseDatabase::GetOwner() reported
     * before the change, which is what the binary passes. No caller is recovered.
     *
     * @param pPacket The packet.
     * @ghidraAddress 0x001c0010
     */
    void OnPhrasePacket(PhrasePacket *pPacket);

    /**
     * Clear and post again every window bar whose mapped bar lies in the range an
     * InvalidateTrackMsg for this track names.
     *
     * The body is not written, because InvalidateTrackMsg declares its payload private. No caller
     * is recovered.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001c0110
     */
    void OnInvalidateTrack(InvalidateTrackMsg *pMsg);

    /**
     * Post a CaughtPhrasePacket. The body is not written.
     *
     * The CaughtPhrasePacket path of HandleMessage() is the one recovered caller.
     *
     * @param pMsg The packet.
     * @ghidraAddress 0x001ba928
     */
    void PostCaughtPhrasePacket(Message *pMsg);

    /**
     * Report the player who owns the phrase at a bar.
     *
     * @param nBar The bar, passed to PhraseDatabase::GetPhraseAt().
     * @return The owner of the phrase the database reports, or g_nullPlayer when there is none.
     * @ghidraAddress 0x001c0268
     */
    Player *GetPhraseOwner(int nBar);

    /**
     * Give the phrase at a bar a new owner.
     *
     * The bar is mapped through slot 5 of mMap. For that step, and in kPlayModeGame for every step
     * slot 7 chains it to, the owner is exchanged in mDatabase, TrackData::SetOwner() receives the
     * previous owner when it changed, a CaughtPhrasePacket goes to mNetSink when one is installed,
     * and the window bars slot 6 reports for the step are posted again. The Catcher subclasses'
     * slot 9, Catcher::SetPhraseOwners(), and NotePitcher::PostPhraseCapturedMsg() are the
     * recovered callers.
     *
     * @param pPlayer The new owner.
     * @param nBar The bar.
     * @ghidraAddress 0x001bafa8
     */
    void SetPhraseOwner(Player *pPlayer, int nBar);

protected:
    /**
     * Act on a message.
     *
     * Primary table slot 3. The body is not written.
     *
     * @param pMsg The message or packet.
     * @ghidraAddress 0x001bc718
     */
    virtual void HandleMessage(Message *pMsg);

public:
    /**
     * Player of this track's phrases.
     *
     * The constructor clears it and ScoreTrackGraph's constructor stores its own PhrasePlayer here
     * at `0x001cefec` from outside the class. OnCommand() plays each bar through it.
     *
     * +0x18
     */
    PhrasePlayer *mPhrasePlayer;

    /**
     * Sink the phrase changes go to as packets, installed by the stage classes' slot 8.
     *
     * The constructor clears it and every one of the four stages writes it from outside the class,
     * which is what records the member public. InstallPhrase(), ClearPhrase(), AddGem(), and
     * SetPhraseOwner() send it GemPacket and CaughtPhrasePacket objects through MsgSink::Handle()
     * when it is set.
     *
     * +0x1c
     */
    MsgSink *mNetSink;

private:
    const TrackData *mTrackData; // +0x20
    // Created by the constructor over mMap, deleted by the destructor, and the object
    // PostPhraseMsg() and three HandleMessage() paths look a phrase up in.
    PhraseDatabase *mDatabase; // +0x24
    PlayMap *mMap;             // +0x28
    // Created by CreatePowerbarMgr() and deleted by the destructor.
    PowerbarMgr *mPowerbarMgr; // +0x2c
    // Copied from the track description's `+0x04`. The track this manager serves. A PhrasePacket's
    // `+0x14` is matched against it, and PostPhraseMsg() copies it into the message's `+0x08`.
    int mUnknown30;         // +0x30
    int mBarTicks;          // +0x34
    int mConfig;            // +0x38
    int mWindowStart;       // +0x3c, the first bar RefreshBar() posts
    int mWindowEnd;         // +0x40, the bar RefreshBar() stops before
    int mRefreshing;        // +0x44, set while RefreshAllBars() and OnExportCommand() post bars
    Mid::MBT mExportLead;   // +0x48, from the start of a bar to its ExportCmd
    Sch::TickClock *mClock; // +0x4c
    CmdID mCommand;         // +0x50, the handle of the file-local Cmd
    CmdID mExportCommand;   // +0x54, the handle of the file-local ExportCmd
    int mTrackKind;         // +0x58, a TrackMode copied from TrackData::mKind
    int mPlayMode;          // +0x5c, the play mode Globals reported at construction
};
