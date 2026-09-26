#pragma once

#include <vector>

#include "app/msgsink.h"
#include "game/rawcontroller.h"
#include "os/hxstr.h"

class Application;
class BGTrackGraph;
class Delayer;
class ForceFeedbackMgr;
class FreqAppearance;
class GameStats;
class Gamer;
class IBStream;
class InputCheatDetectorGS;
class InputMap;
class LevelBuilder;
class LevelData;
class Message;
class MetPersonaData;
class MsgJoiner;
class MsgSource;
class MuseSynth;
class OBStream;
class PlayMap;
class Player;
class Renderer;
class RendererBase;
class ScoreTrackGraph;
class TrackSelector;
struct MetControllerReading;

namespace Sch {
class TickClock;
} // namespace Sch

/**
 * Owner of the world a game session runs in.
 *
 * `11GrooveWorld` in the RTTI descriptor at `0x008eff70`, with MsgSink as a public base at offset 0
 * and RawController as a public base at `+0x04`. The object is 0xbc bytes, which the allocation in
 * GameManagerImpl::CreateWorld() fixes. The translation unit is `GrooveWorld.cpp`, which the
 * tagged release at `0x00194d5c` records with line 302, and ControllerCmd and the file-local
 * FuncCmd and ExitCmd share it.
 *
 * Two vtables belong to the class, and the secondary one precedes the primary one in memory. The
 * RawController subobject addresses `0x007dc628`, whose three entries each adjust `this` by `-4`,
 * and the MsgSink subobject addresses `0x007dc648` with no adjustment. The primary table has four
 * entries and a zero terminator at index 4, the type function at `0x001936f8`, the destructor at
 * `0x0018c368`, the inherited MsgSink::Handle() at `0x00105158`, and the HandleMessage() override
 * below. The secondary table has three entries and a zero terminator at index 3, the same type
 * function and destructor followed by the RawController override below.
 *
 * The member map comes from the constructor, the destructor, and BuildGraphs(). BuildGraphs()
 * creates the input map, the track selector, the message joiner, the delayer, the gamer, the
 * synthesiser, and the score track graphs, and each type is resolved through the table its
 * constructor installs. mState steps through 1 while the level file loads, 2 once it is
 * converted, 4 while the world accepts controller readings, and 6 on one shutdown path.
 *
 * GameManagerImpl reads mInputMap from three of its handlers, writes mUnknown8c from its load path
 * and its begin-game handler, and reads mUnknown90 from its unpause handler. `0x0018dc88` and
 * `0x0018de38` both run once the world is ready.
 */
class GrooveWorld : public MsgSink, public RawController {
    // InputPoller::ReadControllers() at 0x001dfab0 reads mState directly.
    friend class InputPoller;

public:
    /**
     * Construct an empty world.
     *
     * It clears every member except the three load-buffer words, sets mUnknownb8 to 1, and creates
     * the song clock on the application's Watchdog with no tempo map, the cheat detector over
     * g_gameCheatSequences, and the force-feedback manager.
     *
     * @param pApp The application.
     * @param pStats The game manager's statistics.
     * @ghidraAddress 0x0018bef0
     */
    GrooveWorld(Application *pApp, GameStats *pStats);

    /**
     * @ghidraAddress 0x0018c368
     */
    virtual ~GrooveWorld();

    /**
     * Route a CripplePacket to the delayer and a BumpPacket to the track selector.
     *
     * Slot 3 of the primary table. Each identity is confirmed by the Type() slot that returns it,
     * CripplePacket::Type() at `0x003f0cd8` and BumpPacket::Type() at `0x003f0ef8`. A message
     * matching neither is discarded. The two forwarders below are expanded inline here.
     *
     * @param pMsg The message to route.
     * @ghidraAddress 0x00195388
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Report a controller reading. Slot 2 of the secondary table.
     *
     * The body returns at once unless mState is 4, and passes the reading to the cheat detector
     * unless mUnknown90 is set. In jukebox mode a joystick press of button 10 clears mUnknownb8
     * and posts exit mode 1, and every other reading is dropped. While a playback runs, a
     * joystick press of any button below 100 posts exit mode 1. Otherwise a joystick press of
     * button 10 from a local pad posts exit mode 1 once a solo game song is complete, and queues
     * a PauseGameSystemMsg and stops the riffs in any other case. Every remaining reading is
     * queued as a ControllerCmd for replay, unless mUnknown84 is set outside a playback.
     *
     * @param nUnknown1 The first word of the reading.
     * @param nUnknown2 The second word of the reading.
     * @param nUnknown3 The third word of the reading.
     * @param flUnknown4 The float of the reading.
     * @ghidraAddress 0x0018ed98
     */
    virtual void OnUnknownSlot2(int nUnknown1, int nUnknown2, int nUnknown3, float flUnknown4);

    /**
     * Hand one recorded controller reading to the input map.
     *
     * It returns at once unless the input map exists and mState is 4, and otherwise builds a
     * RawControllerMsg from the reading and the song clock's position on the stack and passes it
     * to InputMap's MsgSink half. ControllerCmd::Execute() is the recovered caller.
     *
     * @param pReading The reading.
     * @ghidraAddress 0x0018f078
     */
    void ReplayControllerReading(const MetControllerReading *pReading);

    /**
     * Queue an ExitCmd built from three values.
     *
     * Does nothing unless mState is 4, and otherwise sets mState to 5. While a playback runs, or
     * when nUnknown88 is set, it runs Exit() at once. Otherwise it queues the ExitCmd on the
     * application's watchdog timer with no delay, as a recordable command. PostExitMode1(),
     * PostExitMode2(), and PostExitMode3() are the recovered callers.
     *
     * @param nMode The value Exit() stores in mUnknown94.
     * @param nUnknownb8 The value Exit() stores in mUnknownb8.
     * @param nUnknown88 The value Exit() stores in mUnknown88.
     * @ghidraAddress 0x0018e368
     */
    void PostExit(int nMode, int nUnknownb8, int nUnknown88);

    /**
     * Leave the game in one of three modes.
     *
     * It stores nMode in mUnknown94, nUnknownb8 in mUnknownb8, and nUnknown88 in mUnknown88,
     * stops the riffs and disables the input map, and sends a GameOverMsg to the delayer. The fade
     * is 5000 milliseconds in jukebox mode, 3000 for exit mode 1 or a running playback, and 1000
     * otherwise, and the screen fade sent to the delayer runs 500 longer. The synthesiser fades
     * out over the same length for exit mode 1 or a playback. Otherwise the notes stop at once,
     * the watchdog takes a snapshot, and the song clock pauses. It then stops the force feedback
     * and queues FinishSong() 600 milliseconds after the fade. ExitCmd::Execute() is the recovered
     * caller.
     *
     * @param nMode The exit mode.
     * @param nUnknownb8 Stored in mUnknownb8.
     * @param nUnknown88 Stored in mUnknown88.
     * @ghidraAddress 0x0018e478
     */
    void Exit(int nMode, int nUnknownb8, int nUnknown88);

    /**
     * Hand a CripplePacket to the delayer.
     *
     * HandleMessage() expands this body inline, and a second out-of-line copy sits at
     * `0x00194b20`.
     *
     * @param pMsg The packet.
     * @ghidraAddress 0x00195348
     */
    void OnCripplePacket(Message *pMsg);

    /**
     * Hand a BumpPacket to the track selector.
     *
     * HandleMessage() expands this body inline.
     *
     * @param pMsg The packet.
     * @ghidraAddress 0x00194b50
     */
    void OnBumpPacket(Message *pMsg);

    /**
     * Install the sink and the source the gamer is wired to.
     *
     * The source is retained only in game mode 3, and a null pointer is stored otherwise.
     *
     * @param pSink The sink stored in mUnknown20.
     * @param pSource The source stored in mUnknown1c in game mode 3.
     * @ghidraAddress 0x00194b80
     */
    void SetUnknown20And1c(MsgSink *pSink, MsgSource *pSource);

    /**
     * Create the level and begin reading its MIDI file asynchronously.
     *
     * The read is submitted with no zone current, and mState becomes 1.
     *
     * @param path The file to read. An empty path reads g_szEmptyString.
     * @ghidraAddress 0x00194bc8
     */
    void StartLoad(const HxStr &path);

    /**
     * Report whether the MIDI file read has completed.
     *
     * A failed read is reported through Fatal().
     *
     * @return Non-zero once the read is complete.
     * @ghidraAddress 0x00194ca0
     */
    int IsLoadDone();

    /**
     * Convert the MIDI file into the level and adopt the level's tempo map.
     *
     * The buffer the read filled is released, and mState becomes 2.
     *
     * @ghidraAddress 0x00194d00
     */
    void FinishLoad();

    /**
     * Prepare the loaded level for play.
     *
     * Resets the synthesiser through its slots 5 and 6, sets its jam flag from the play mode,
     * runs BuildGraphs(), CreateRenderer(), and ConnectPlayers(), starts the streamed audio named
     * by configuration code 0x3a5 when code 0x3a4 is set, and reads the start offset from code
     * 0x38d, which becomes the negated song start clamped to the finite range. It then stores
     * configuration flag 0x3a1 in mUnknown90 and sets mState to 3. GameManagerImpl's
     * FinishWorldLoad() and Load() call it after FinishLoad(). The title is inferred.
     *
     * @ghidraAddress 0x0018dc88
     */
    void PrepareLevel();

    /**
     * Starts play on a prepared level.
     *
     * Disables the input map entries, flushes the watchdog, resumes mSongClock, starts the note
     * destroyer, and sets mState to 4. It runs synthesiser slot 10 and builds the mUnknown50
     * sequencers. Unless mUnknown90 is set, it posts EnableInput() half a quantum of the first
     * track ahead of the song start, and it always posts StartSequencers() at the start. It
     * sends a GameBeginMsg to the delayer and the joiner and a one-second FadeGameMsg that fades
     * in to the delayer, resets the statistics for the player count, runs Player slot 11 on
     * every player, and configures the force feedback manager (jukebox mode, mUnknown8c, the
     * settings flag, the local player count, and a metronome 3200 ticks ahead).
     * GameManagerImpl's OnUnknownSlot6() is the only caller. The title is inferred.
     *
     * @ghidraAddress 0x0018de38
     */
    void StartPlay();

    /**
     * Create a remote player and append it to mPlayers.
     *
     * @param nId The player's identifier, passed to NetPlayer twice.
     * @param nUnused Not read.
     * @param name The player's name.
     * @param pAppearance The appearance the player is drawn with.
     * @ghidraAddress 0x00194de8
     */
    void AddNetPlayer(int nId, int nUnused, const HxStr &name, const FreqAppearance *pAppearance);

    /**
     * Create a player this console drives and append it to mPlayers and mLocalPlayers.
     *
     * The track is nId, except in solo mode, where it is configuration code 0x3a6 minus 1. The
     * player receives the persona's appearance (MetPersonaData::mUnknown140). The title is
     * inferred.
     *
     * @param nId The player's identifier.
     * @param nInputSlot The controller slot.
     * @param nUnused Not read. GameManagerImpl::AddPersonaPlayers() passes the shuffled index.
     * @param colorName The player's colour name.
     * @param pPersona The persona the player plays as.
     * @ghidraAddress 0x0018c600
     */
    void AddLocalPlayer(
        int nId, int nInputSlot, int nUnused, const HxStr &colorName, MetPersonaData *pPersona);

    /**
     * Remove the first player with an identifier from mPlayers, without destroying it.
     *
     * @param nId The identifier to match against Player::mId20.
     * @ghidraAddress 0x00194ef0
     */
    void RemovePlayer(int nId);

    /**
     * Detach the renderer from every source that feeds it and delete it.
     *
     * @ghidraAddress 0x00194f88
     */
    void DestroyRenderer();

    /**
     * Write the phrase database of every score track graph.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00195058
     */
    void SavePhrases(OBStream &stream);

    /**
     * Read the phrase database of every score track graph back.
     *
     * @param stream The stream to read from.
     * @param bClearOwners Non-zero to return every loaded phrase to the stand-in player.
     * @ghidraAddress 0x001950c0
     */
    void LoadPhrases(IBStream &stream, int bClearOwners);

    /**
     * Enable every entry of the input map.
     *
     * @ghidraAddress 0x00195150
     */
    void EnableInput();

    /**
     * Queue exit mode 1 with the current mUnknownb8.
     *
     * @ghidraAddress 0x00195170
     */
    void PostExitMode1();

    /**
     * Queue exit mode 2.
     *
     * @ghidraAddress 0x00195198
     */
    void PostExitMode2();

    /**
     * Queue exit mode 3 with a final argument of 1.
     *
     * @ghidraAddress 0x001951c0
     */
    void PostExitMode3();

    /**
     * Tear the world down ahead of destruction.
     *
     * It runs StopLevel() when mState is 6 and DeletePlayers() always, deletes the level, the song
     * clock, the cheat detector, and mForceFeedback, and then stops and destroys the note destroyer
     * through StopNoteDestroyer() and DestroyNoteDestroyer(). The destructor is the recovered
     * caller.
     *
     * @ghidraAddress 0x001951e8
     */
    void Shutdown();

    /**
     * Report the world's song clock.
     *
     * Globals::GetSongClock() is the out-of-line caller, and the constructor creates the clock.
     *
     * @return The clock.
     * @ghidraAddress 0x001952a0
     */
    Sch::TickClock *GetSongClock();

    /**
     * Report the level's play map.
     *
     * Globals' accessor at `0x00118da0` is the recovered caller.
     *
     * @return The play map.
     * @ghidraAddress 0x001952a8
     */
    PlayMap *GetPlayMap();

    /**
     * @return The level.
     * @ghidraAddress 0x001952d8
     */
    LevelData *GetLevel();

    /**
     * Report the renderer's RendererBase half.
     *
     * GameManagerImpl::DrawFrame() runs RendererBase slots 6, 7, and 8 through it, and skips the
     * world when it is null.
     *
     * @return The renderer, or null when no renderer exists.
     * @ghidraAddress 0x001952e0
     */
    RendererBase *GetRendererSink();

    /**
     * Set the statistics word at `+0x14` of mStats to 1.
     *
     * @ghidraAddress 0x00195378
     */
    void MarkStatsFlag();

private:
    /**
     * Wire every player into the world's message graph.
     *
     * Each player becomes a sink of mInputMap, mTrackSelector, and mUnknown1c when set, and
     * mJoiner, mGamer, mTrackSelector, and mUnknown20 when set become sinks of the player.
     * PrepareLevel() is the caller. The title is inferred.
     *
     * @ghidraAddress 0x0018c828
     */
    void ConnectPlayers();

    /**
     * Undo ConnectPlayers().
     *
     * The player's own sinks are removed first, mGamer ahead of mJoiner, and the player is then
     * removed from mUnknown1c, mTrackSelector, and mInputMap in that order. The title is inferred.
     *
     * @ghidraAddress 0x0018c960
     */
    void DisconnectPlayers();

    /**
     * Delete every player and empty both player lists.
     *
     * Shutdown() is the caller. The title is inferred.
     *
     * @ghidraAddress 0x0018c778
     */
    void DeletePlayers();

    /**
     * Create the in-game renderer and introduce every player to it.
     *
     * The renderer becomes a sink of mDelayer and of each player. Each player then sends it a
     * TrackSelectMsg, and a SeekerMsg as well while the player's Slot2() reports -1.
     * PrepareLevel() is the caller.
     *
     * @ghidraAddress 0x0018caa8
     */
    void CreateRenderer();

    /**
     * Build the message graph and the per-track graphs of the loaded level.
     *
     * Creates the delayer, the joiner, the input map, the track selector, the synthesiser, the
     * gamer, one BGTrackGraph per backing and intro track, and one ScoreTrackGraph per gameplay
     * track, and wires them together. While a saved game is loading, the song title and the
     * phrases are read back from the log FinishSong() wrote. PrepareLevel() is the caller. The
     * title is inferred.
     *
     * @ghidraAddress 0x0018cce8
     */
    void BuildGraphs();

    /**
     * Delete everything BuildGraphs() created.
     *
     * StopLevel() is the caller. The title is inferred.
     *
     * @ghidraAddress 0x0018da60
     */
    void DestroyGraphs();

    /**
     * Start the backing sequencers and every gameplay stage.
     *
     * The intro tracks' sequencers are deleted first. The world queues it as a FuncCmd, through
     * the pointer to member at `0x007dc280`. The title is inferred.
     *
     * @ghidraAddress 0x0018e238
     */
    void StartSequencers();

    /**
     * Stop every stage and sequencer at the end of the song and schedule EndLevel().
     *
     * In jam mode the song name and every stage's phrases are first written to the reset log,
     * behind a length word patched in last. Exit mode 2 posts EndLevel() half a second later,
     * and every other mode runs it at once. The world queues it as a FuncCmd, through the pointer
     * to member at `0x007dc2a0`. The title is inferred.
     *
     * @ghidraAddress 0x0018e6f0
     */
    void FinishSong();

    /**
     * Report the end of the game to the game manager.
     *
     * Clears the display to black in exit mode 3, sets mState to 6, and queues an EndGameMsg that
     * carries mUnknown88. Exit modes 1 and 2 then install the bank-load progress hook and run the
     * synthesiser's LoadBankSet4(), except in a jukebox session with mUnknownb8 set. The title is
     * inferred.
     *
     * @ghidraAddress 0x0018eb70
     */
    void EndLevel();

    /**
     * Take the level down after EndLevel().
     *
     * Disconnects the players, deletes the renderer and the graphs, pauses the song clock at
     * tick zero, and sets mState back to 2. Shutdown() runs it while mState is 6. The title is
     * inferred.
     *
     * @ghidraAddress 0x0018ec90
     */
    void StopLevel();

public:
    /**
     * Show a line of text on the track displays.
     *
     * Sends a TextMsg through mDelayer, and does nothing before mDelayer exists. The script
     * binding at `0x00153464` is the caller.
     *
     * @param text The text.
     * @ghidraAddress 0x0018f140
     */
    void DisplayText(const HxStr &text);

    /**
     * Report the jukebox song title.
     *
     * The image has no caller. Overlay copies mSongName directly instead. The name is inferred.
     *
     * @return A copy of mSongName.
     * @ghidraAddress 0x001937a8
     */
    HxStr GetSongName() const;

private:
    Application *mApp; // +0x08

public:
    /**
     * The input map of the world's players. +0x0c
     *
     * Public because GameManagerImpl::OnUnpauseGameSystem() rebuilds it directly at `0x00106b74`,
     * and the image has no accessor for it.
     */
    InputMap *mInputMap;

private:
    TrackSelector *mTrackSelector; // +0x10
    MsgJoiner *mJoiner;            // +0x14
    // The MIDI level. BuildGraphs() halts with `MIDI level file has not been loaded.` while it is
    // null.
    LevelBuilder *mLevel; // +0x18
    // Registered with in game mode 3 only.
    MsgSource *mUnknown1c; // +0x1c
    MsgSink *mUnknown20;   // +0x20
    Delayer *mDelayer;     // +0x24
    // The in-game renderer. CreateRenderer() creates it.
    Renderer *mRenderer; // +0x28

public:
    /**
     * The scoring state. +0x2c
     *
     * Public because the juice, section, and track cheats reach it through Globals::GetWorld()
     * with no accessor in the image.
     */
    Gamer *mGamer;

private:
    GameStats *mStats; // +0x30

public:
    /**
     * Vibration driver of the controllers. +0x34
     *
     * Built by the constructor at `0x0016dae0` and deleted by Shutdown(). Public because
     * TnlCrippleFX's frame routine at `0x0043e500` reads it through Globals::GetWorld() with no
     * accessor in the image.
     */
    ForceFeedbackMgr *mForceFeedback;

public:
    // The score graphs, indexed by track. Public because the volume script command reads one
    // with no accessor in the image.
    std::vector<ScoreTrackGraph *> mTrackGraphs; // +0x38

private:
    std::vector<BGTrackGraph *> mUnknown44; // +0x44
    // The element type is fixed by DestroyGraphs() and StartSequencers(). Both run one
    // std::for_each instantiation over this vector and mUnknown44.
    std::vector<BGTrackGraph *> mUnknown50; // +0x50
    BGTrackGraph *mUnknown5c;               // +0x5c
    MuseSynth *mMuseSynth;                  // +0x60
    Sch::TickClock *mSongClock;             // +0x64

public:
    /**
     * Every player in the world, in slot order. +0x68
     *
     * Public because Renderer and Overlay walk it directly, and the image has no accessor for it.
     */
    std::vector<Player *> mPlayers;

    /**
     * The players this console drives. +0x74
     *
     * The element type is proven by the routine at `0x00102790`, which reads the identifier at
     * `+0x20` of the first element. The role is inferred from Renderer, which resolves one
     * `tnl local%d.view` per element. Public because Renderer and AppTunnel read it directly, and
     * the image has no accessor for it.
     */
    std::vector<Player *> mLocalPlayers;

private:
    InputCheatDetectorGS *mCheatDetector; // +0x80
    int mUnknown84;                       // +0x84
    int mUnknown88;                       // +0x88

public:
    /**
     * A word GameManagerImpl::OnBeginGameLocal() clears at `0x001067c4` and
     * GameManagerImpl::Load() sets at `0x001074b8`. Public because both write it directly, and
     * the image has no accessor for it. No reader is recovered. +0x8c
     */
    int mUnknown8c;

    /**
     * A word GameManagerImpl::OnUnpauseGameSystem() reads directly at `0x00106b54`. It rebuilds
     * mInputMap only while the word is zero. The image has no accessor for it. +0x90
     */
    int mUnknown90;

private:
    int mUnknown94;    // +0x94, the exit mode
    int mState;        // +0x98
    HxStr mLevelPath;  // +0x9c
    void *mLoadBuffer; // +0xa4, the raw MIDI file
    int mLoadSize;     // +0xa8
    int mLoadHandle;   // +0xac

public:
    /**
     * Song title shown in jukebox mode. +0xb0
     *
     * The constructor zeroes both words and the destructor frees the text through the inline
     * ~HxStr(). Overlay's constructor copy-constructs it at `0x0041d358` and displays it. Public
     * because Overlay reads it directly, and the image has no accessor for it.
     */
    HxStr mSongName;

    /**
     * A word that starts at 1 and that PostExitMode1() posts with the exit.
     *
     * Public because GameManagerImpl::EndGame() reads it directly at `0x00106c30` and announces
     * MetFreqEndedMsg(1) only while it is zero. The image has no accessor for it. +0xb8
     */
    int mUnknownb8;
};
