#pragma once

#include <vector>

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "sch/cmdid.h"

class BGTrackGraph;
class EnableMgr;
class GameStats;
class Globals;
class Message;
class PhraseDatabase;
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
     * The body is not written.
     *
     * @param nTrackCount The level's track count.
     * @param nUnknown24 The level's slot-9 value, recorded at `+0x24`.
     * @param pStats The session statistics.
     * @ghidraAddress 0x00110138
     */
    Gamer(int nTrackCount, int nUnknown24, GameStats *pStats);

    /** @ghidraAddress 0x00110930 */
    virtual ~Gamer();

    /**
     * Receive one message.
     *
     * Not reconstructed. It dispatches on `Message::Type()` down a chain beginning with the
     * identity at `0x006d0184`, and gates on the members at `+0x2c`, `+0x30`, and `+0x18`.
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

private:
    int mUnknown18;                           // +0x18
    int mUnknown1c;                           // +0x1c
    int mUnknown20;                           // +0x20
    int mUnknown24;                           // +0x24
    int mUnknown28;                           // +0x28
    int mPlayMode;                            // +0x2c
    int mGameMode;                            // +0x30
    int mJukeboxMode;                         // +0x34
    int mUnknown38;                           // +0x38
    int mUnknown3c;                           // +0x3c
    int mTrackCount;                          // +0x40
    int mUnknown44;                           // +0x44
    int mUnknown48;                           // +0x48
    int mUnknown4c;                           // +0x4c
    int mUnknown50;                           // +0x50
    Globals *mGlobals;                        // +0x54
    GameStats *mStats;                        // +0x58
    int mTicksPerBar;                         // +0x5c
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
    int mUnknown84;        // +0x84
    int mUnknown88;        // +0x88
    PlayMap *mPlayMap;     // +0x8c
    EnableMgr *mEnableMgr; // +0x90
    EnableMgr *mUnknown94; // +0x94
    int mUnknown98;        // +0x98
};
