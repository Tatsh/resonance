#pragma once

#include <vector>

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "mid/mbt.h"
#include "sch/cmdid.h"

class AdvanceSectionMsg;
class BGTrackGraph;
class CrippleMsg;
class EnableFreestyleMsg;
class EnableMgr;
class GameStats;
class Globals;
class Message;
class PhraseCapturedMsg;
class PhraseDatabase;
class PlaybackModeMsg;
class PlayMap;
class Player;
class ScoreTrackGraph;
class TrackData;

/**
 * One participant's view of a session, driven by messages.
 *
 * `Gamer` in the RTTI descriptor at `0x00901cb0`, over `MsgSink` and `MsgSource`. Two vtables
 * belong to it, one per base, both walked to their terminator: the primary at `0x007ce820` with
 * delta 0, which is the `MsgSink` table because `MsgSink` is the first base, and the `MsgSource`
 * table at `0x007ce7f8` with delta -4.
 *
 * Both are four entries, so the class **adds no virtual of its own**. It overrides only
 * `HandleMessage`, inheriting `MsgSink::Handle` and both `MsgSource` virtuals unchanged.
 *
 * The base subobjects account for `+0x00` through `+0x17`. The destructor at `0x00110930` restores
 * the primary table at `+0x00` and the `MsgSource` one at `+0x14`, which is where `MsgSource`
 * places its own vptr over its `mSinks` vector, so its subobject sits at `+0x04` exactly as the
 * -4 delta states. This class's own members start at `+0x18`.
 *
 * The object is 0x9c bytes, which GrooveWorld allocates at `0x0018cfc0`. The RTTI name of the
 * unit's file-local command embeds the constructor's mangled name, which fixes its three parameter
 * types as two integers and a GameStats pointer. The members are declared in recovered offset
 * order. Those whose purpose is undetermined keep their offset as their title.
 */
class Gamer : public MsgSink, public MsgSource {
public:
    /**
     * Build the participant's view for a level.
     *
     * Copies the world's players, gives every player a score of 0 with a ceiling of 100000 and,
     * in kGameModeSolo, the configured juice and juice ceiling. A jukebox session starts in
     * playback with every input binding off except the first slot's two rotations.
     *
     * @param nTrackCount The level's track count.
     * @param nEndBar The level's last bar, from its slot 9.
     * @param pStats The session statistics.
     * @ghidraAddress 0x00110138
     */
    Gamer(int nTrackCount, int nEndBar, GameStats *pStats);

    /**
     * Withdraw the queued command and delete both enable policies.
     *
     * @ghidraAddress 0x00110930
     */
    virtual ~Gamer();

    /**
     * Withdraw every command queued under mCommand from the song clock.
     *
     * GrooveWorld calls it at `0x0018e6f0`. The title is inferred.
     *
     * @ghidraAddress 0x00116c40
     */
    void Withdraw();

    /**
     * Receive one message.
     *
     * Dispatches AdvanceSectionMsg, PhraseCapturedMsg, EnableFreestyleMsg, PlaybackModeMsg, and
     * CrippleMsg to their handlers, in that order of test, and ignores every other message. The
     * first two handlers are expanded inline here.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x00112978
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Record the track graphs and build the enable policy for the session mode.
     *
     * In kPlayModeGame, kGameModeSolo builds GameEnableMgr::CreateReleasing() from configuration
     * code 0x387 and the other modes GameEnableMgr::CreateUnrestricted(), and every track outside
     * catch mode is then disabled. Outside kPlayModeGame, the solo and local modes build a
     * LocalJamEnableMgr and the network mode a NetJamEnableMgr over eight owner buckets. The title
     * is inferred.
     *
     * @param pGraphs The track graphs, one per track.
     * @ghidraAddress 0x00110ba0
     */
    void CreateEnableMgr(std::vector<ScoreTrackGraph *> *pGraphs);

    /**
     * Record a player against one bar of one track.
     *
     * Forwards to EnableMgr::SetBarOwner() on mEnableMgr. TrackData::SetOwner() is the recovered
     * caller.
     *
     * @param nTrack The track's index.
     * @param nBar The bar.
     * @param pPlayer The player.
     * @ghidraAddress 0x00116828
     */
    void SetBarOwner(int nTrack, int nBar, Player *pPlayer);

    /**
     * Ask whether one bar of one track may be caught.
     *
     * Forwards to EnableMgr::QueryBar() on mEnableMgr. TrackData::QueryBar() is the recovered
     * caller, and Catcher treats a zero answer as a bar that cannot be caught.
     *
     * @param nTrack The track's index.
     * @param nBar The bar.
     * @return The answer of mEnableMgr.
     * @ghidraAddress 0x00116858
     */
    int QueryBar(int nTrack, int nBar);

    /**
     * Report whether one track is outside catch mode.
     *
     * @param nTrack The track's index.
     * @return Non-zero unless the track's mKind is kTrackModeCatch.
     * @ghidraAddress 0x00116888
     */
    bool IsNonCatchTrack(int nTrack);

    /**
     * Report the phrase database of one track's graph.
     *
     * @param nTrack The track's index into mGraphs.
     * @return The graph's phrase database.
     * @ghidraAddress 0x001168b0
     */
    PhraseDatabase *GetPhraseDatabase(int nTrack);

    /**
     * Report one track of the level.
     *
     * @param nTrack The track's index.
     * @return LevelData::TrackAt() of Globals::GetLevel().
     * @ghidraAddress 0x001168e0
     */
    TrackData *GetTrack(int nTrack);

    /**
     * Run the update of one bar and schedule the next.
     *
     * Switches each background track's MIDI by mBackEnableMgr, ends a solo or multiplayer game when
     * its conditions are met, steps jam playback, ends a jukebox song at mEndBar, moves the
     * streamed audio, and finally schedules bar nBar + 1. GamerCmd::Execute() is the caller. The
     * title is inferred.
     *
     * @param nBar The bar.
     * @ghidraAddress 0x00111fa8
     */
    void OnBar(int nBar);

    /**
     * Schedule the first bar's update.
     *
     * The title is inferred.
     *
     * @ghidraAddress 0x00116c20
     */
    void Start();

    /**
     * Record the background tracks and build the enable policy for them.
     *
     * The policy is GameEnableMgr::CreateReleasing() over configuration code 0x386, one track per
     * graph. RndWorld's draw pass is the caller. The title is inferred.
     *
     * @param pGraphs The background tracks' graphs.
     * @ghidraAddress 0x001167e0
     */
    void SetBackGraphs(std::vector<BGTrackGraph *> *pGraphs);

    /**
     * Give the first player a freestyle span.
     *
     * Calls Fatal() with `Only call Gamer::EnablePlayerFreestyle() from tutorial.` outside the
     * tutorial, which is what attests the title.
     *
     * @param nStartBar The first bar, passed to Player::Slot8().
     * @param nEndBar The end bar, passed to Player::Slot8().
     * @ghidraAddress 0x00116a98
     */
    void EnablePlayerFreestyle(int nStartBar, int nEndBar);

    /**
     * Add juice to the first player and announce it.
     *
     * The `add_juice` script command is the caller. The title is inferred.
     *
     * @param nAmount The juice to add.
     * @ghidraAddress 0x00116ae8
     */
    void AddJuice(int nAmount);

    /**
     * End the game at the next bar, optionally with a score for the first player.
     *
     * Sets mEndBar to 0. A non-zero score becomes the first player's score with a ceiling of
     * 10000. The listen-mode script command and the win-with-points cheat are the callers. The
     * title is inferred.
     *
     * @param nScore The score, or 0 to leave the score unchanged.
     * @ghidraAddress 0x00116b18
     */
    void EndWithScore(int nScore);

    /**
     * Advance to the bar of a position, with PlayMap::Slot18() of that bar as the advance.
     *
     * Public because the advance-section script command calls it with no accessor in the image.
     * The title is inferred.
     *
     * @param position The position to advance to.
     * @ghidraAddress 0x00116a30
     */
    void AdvanceAt(Mid::MBT position);

private:
    // 0x00116920
    // Outside jam or in a network game, and outside the tutorial, ignores the
    // message. Otherwise advances at the message's position unless the player's Slot2() is set.
    void OnAdvanceSection(AdvanceSectionMsg *pMsg);

    // 0x001169a8
    // Outside jam and before the game ends, passes the message to the capturing
    // player, then in a solo game outside the tutorial runs FreeTracksAfterCapture().
    void OnPhraseCaptured(PhraseCapturedMsg *pMsg);

    // 0x00111080
    // For a non-catch track, gives the player an eight-bar freestyle span, frees the
    // track for it, invalidates the track's seeker, marks the message handled, and sends a
    // FreestyleFXMsg.
    void OnEnableFreestyle(EnableFreestyleMsg *pMsg);

    // 0x00110e60
    // In jam, toggles playback at the message's bar, switching the input bindings
    // and the play map, and announces the change in a PlaybackToggleMsg.
    void OnPlaybackMode(PlaybackModeMsg *pMsg);

    // 0x00111230
    // Sends a CripplePacket naming every other player on the message's track, and
    // marks the message handled when there is one.
    void OnCripple(CrippleMsg *pMsg);

    // 0x00112838
    // Posts a GamerCmd for the bar on the song clock under mCommand, one tick before
    // the bar starts unless the bar starts at zero. The title is inferred.
    void ScheduleBar(int nBar);

    // 0x001116c8
    // Sends an AdvanceSectionToggleMsg for the bar, then an InvalidateTrackMsg from
    // the following step's mapped position through mUnknown3c further and an InvalidateSeekerMsg
    // through each track's source, and runs script template 1013. The title is inferred.
    void AdvanceTo(int nBar, int nAdvance);

    // 0x00111c90
    // Sends a TracksOnMsg with the count of catch tracks whose phrase at the bar has
    // an owner. Returns true when no enabled catch track has an unowned phrase with gems there.
    // The title is inferred.
    bool SendTracksOn(int nBar);

    // 0x00111e30
    // When SendTracksOn() returns true, frees every non-catch track from mFreeEndBar
    // to the following step bar and sends a FreestyleFXMsg for each. The title is inferred.
    bool FreeTracksAfterCapture(int nBar);

    // 0x001118c0
    // Records every player's score in mStats, sends a WinMsg with the players on the
    // best score, plays `SND_WIN`, and ends the game. The title is inferred.
    void DeclareWinners();

    // 0x00111b78
    // Records player 0's score, Slot17() count, and Slot18() fraction in mStats, with
    // the fraction of the song reached. The title is inferred.
    void RecordSoloStats(int bCompleted, int nBar);

    // How the game has ended. The constructor starts at kEndStateNone.
    enum EndState {
        kEndStateNone = 0,
        kEndStateWon = 1,
        kEndStateOver = 2,
    };

    int mTutorial; // +0x18 set in the tutorial

public:
    /**
     * Practice-mode flag the practice cheat sets. +0x1c
     *
     * Public because the practice cheat writes it through GrooveWorld::mGamer with no accessor
     * in the image.
     */
    int mUnknown1c;

private:
    int mUnknown20;                           // +0x20
    int mEndBar;                              // +0x24 the bar the game ends at
    int mEndState;                            // +0x28 an EndState
    int mPlayMode;                            // +0x2c
    int mGameMode;                            // +0x30
    int mJukeboxMode;                         // +0x34
    int mUnknown38;                           // +0x38
    int mUnknown3c;                           // +0x3c
    int mTrackCount;                          // +0x40
    int mUnknown44;                           // +0x44
    int mUnknown48;                           // +0x48
    int mPlaybackOn;                          // +0x4c
    int mUnknown50;                           // +0x50
    Globals *mGlobals;                        // +0x54
    GameStats *mStats;                        // +0x58
    Mid::MBT mBarLength;                      // +0x5c
    std::vector<Player *> mPlayers;           // +0x60
    CmdID mCommand;                           // +0x6c
    std::vector<BGTrackGraph *> *mBackGraphs; // +0x70
    std::vector<ScoreTrackGraph *> *mGraphs;  // +0x74

public:
    /**
     * One message source per track.
     *
     * Public because GameEnableMgr sends InvalidateTrackMsg through an element directly at
     * `0x00101ea0`, and the image has no accessor. +0x78
     */
    std::vector<MsgSource> mTrackSources;

private:
    int mUnknown84;    // +0x84
    int mFreeEndBar;   // +0x88 the end bar non-catch tracks are free until
    PlayMap *mPlayMap; // +0x8c

public:
    /**
     * Per-track enable policy. +0x90
     *
     * Public because the enable-all-tracks cheat drives it through GrooveWorld::mGamer with no
     * accessor in the image.
     */
    EnableMgr *mEnableMgr;

private:
    EnableMgr *mBackEnableMgr; // +0x94

public:
    /**
     * Cheat-armed flag the cheats set. +0x98
     *
     * Public because the practice, listen-mode, and enable-all-tracks cheats write it through
     * GrooveWorld::mGamer with no accessor in the image.
     */
    int mUnknown98;
};
