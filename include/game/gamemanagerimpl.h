#pragma once

#include <vector>

#include "app/msgqueue.h"
#include "app/msgsink.h"
#include "game/gameparams.h"
#include "game/gamestats.h"
#include "game/grooveworld.h"
#include "game/inputpoller.h"
#include "game/metagameworld.h"
#include "met/metpersonadata.h"
#include "msg/message.h"
#include "os/hxstr.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

class GamePlayback;
class GameRecorder;

/**
 * Connectivity of a session, recorded by GameManagerImpl::SetGameMode().
 *
 * The four names are the literals the setter publishes to the script layer, in value order, and
 * they are the whole evidence for the value set.
 */
enum GameMode { kGameModeNone = 0, kGameModeSolo = 1, kGameModeLocal = 2, kGameModeNet = 3 };

/**
 * Kind of a session, recorded by GameManagerImpl::SetPlayMode().
 *
 * The three names are the literals the setter publishes to the script layer, in value order.
 */
enum PlayMode { kPlayModeNone = 0, kPlayModeGame = 1, kPlayModeJam = 2 };

/**
 * Owner of the running game.
 *
 * `15GameManagerImpl` in the RTTI descriptor at `0x008f0090`, deriving from `11GameManager` at
 * `0x008ef6b0`, which derives in turn from the builtin MsgSink descriptor at `0x0086f780`. The
 * object is 0x10c bytes, its constructor is at `0x00105f50`, and its table at `0x007cd5f8` has 39
 * entries followed by the zero terminator. An earlier reading of this class recorded 36, and the
 * read of the terminator corrects it.
 *
 * The split between GameManager and GameManagerImpl cannot be recovered. No vtable in the image
 * addresses a GameManager subobject, the accessor at `0x0010b7c8` builds both descriptors and
 * returns the derived one, and nothing outside the accessor references `0x008ef6b0`. Every slot is
 * therefore declared here, on the derived class the vtable belongs to, and the base is declared
 * only so that the descriptor chain reads correctly.
 *
 * Every entry of the 39-entry table. Slot 0 is the compiler-generated type function at `0x0010b7c8`
 * and is not source. Slot 2 is MsgSink::Handle() at `0x00105158`, inherited unchanged.
 *
 *  - 1 `0x001062d0` the destructor.
 *  - 3 `0x00107540` HandleMessage().
 *  - 4 `0x001065a8` DrawFrame().
 *  - 5 `0x0010bfa0` DrawFrameSimple().
 *  - 6 `0x0010c128` OnUnknownSlot6().
 *  - 7 `0x0010c420` StartRecording().
 *  - 8 `0x0010c4b8` StartPlayback().
 *  - 9 `0x0010b870` GetUnknownfc().
 *  - 10 `0x00105e80` AddPersona().
 *  - 11 `0x0010b888` GetPersonas().
 *  - 12 `0x0010be20` ClearPersonas().
 *  - 13 `0x0010c168` Start().
 *  - 14 `0x00106e28` PollPlayback().
 *  - 15 `0x0010b890` GetWorld().
 *  - 16 `0x0010b898` GetMetaWorld().
 *  - 17 `0x0010b8a0` GetPoller().
 *  - 18 `0x0010b8a8` GetUnknown18().
 *  - 19 `0x0010b8b0` GetStats().
 *  - 20 `0x0010c588` Save().
 *  - 21 `0x001072b0` Load().
 *  - 22 `0x0010b8b8` IsPlaybackActive().
 *  - 23 `0x0010c290` SetGameMode().
 *  - 24 `0x0010b8c8` GetGameMode().
 *  - 25 `0x0010b8d0` GetParams().
 *  - 26 `0x0010b8d8` GetChangeCount().
 *  - 27 `0x0010c210` SetParams().
 *  - 28 `0x0010c3e0` SetUnknown88().
 *  - 29 `0x0010c348` SetPlayMode().
 *  - 30 `0x0010b8e0` GetUnknown88().
 *  - 31 `0x0010b8e8` GetPlayMode().
 *  - 32 `0x0010bee0` QueueMessage().
 *  - 33 `0x0010b878` SetDrawEnabled().
 *  - 34 `0x00106720` OnBeginGameLocal().
 *  - 35 `0x0010c148` OnEndGame().
 *  - 36 `0x001069a8` OnPauseGameSystem().
 *  - 37 `0x00106af8` OnUnpauseGameSystem().
 *  - 38 `0x0010bf10` OnDoPlayback().
 *
 * The five handler slots are each pinned by HandleMessage(), which compares the reported message
 * type against five globals and dispatches one slot for each. `0x006d03a4` belongs to
 * BeginGameLocalMsg, `0x006d03ac` to EndGameMsg, `0x006d03b4` to PauseGameSystemMsg, `0x006d03bc`
 * to UnpauseGameSystemMsg, and `0x006d03c4` to GameManagerDoPlaybackMsg. Each of the five globals
 * has exactly two readers, this dispatcher and the message class's own type reporter, which is what
 * makes the pairing certain. A type that matches none of the five trips
 * `FatalError("DISPATCH_CHECK: ", pMsg->Name())`.
 *
 * StartRecording() installs a GameRecorder and StartPlayback() a GamePlayback. Neither class emits
 * RTTI, so both titles are inferred, from EndRecordingCmd, whose name the RTTI attests, running the
 * recorder's end.
 *
 * Four slots read or write the embedded settings rather than a member of this class. The offsets
 * `+0x84`, `+0x88`, and `+0x90` all fall inside the 0x38-byte GameParams subobject at `+0x68`, so
 * SetPlayMode() and GetPlayMode() drive `GameParams::mUnknown1c`, SetUnknown88() and
 * GetUnknown88() drive `GameParams::mUnknown20`, and SetGameMode() writes
 * `GameParams::mUnknown28`. Those three settings members are public for that reason, and a friend
 * declaration on GameParams would fit the image equally well.
 *
 * Every member of this class is private. Nothing outside the class touches one directly, and each
 * of the members a caller needs has a virtual accessor among slots 9 to 31.
 */
class GameManagerImpl : public MsgSink {
public:
    /**
     * Build the manager, its input poller, and its message queue.
     *
     * The constructor creates the InputPoller, registers itself as a sink of its own embedded
     * queue, and clears the poller's field at `+0x34`. Every word it does not set otherwise starts
     * at zero, mUnknown18 at `0x00105f8c` included.
     *
     * mUnknownfc and mDrawSuppressed both start at 1. The second of the two is what makes the first
     * frame after construction draw nothing until SetDrawEnabled() runs.
     *
     * @ghidraAddress 0x00105f50
     */
    GameManagerImpl();

    /**
     * Destroy the recorder, the front-end world, the poller, the queue, and the tally.
     *
     * The game world is not deleted here. CheckState() runs first and its result is discarded.
     *
     * @ghidraAddress 0x001062d0
     */
    virtual ~GameManagerImpl();

    /**
     * Draw one frame.
     *
     * Slot 4. The queue is drained first. The routine then collects up to two drawable roots, the
     * game world's when it has one and the front-end world's when one exists, runs two virtuals on
     * each, and submits the frame between the display device's frame pair. A suppressed manager
     * collects the roots and runs the two virtuals but submits nothing.
     *
     * The body is not written. Both roots are resolved through members of GrooveWorld and
     * MetaGameWorld whose signatures are not settled.
     *
     * @ghidraAddress 0x001065a8
     */
    virtual void DrawFrame();

    /**
     * Redraw the front-end world alone.
     *
     * Slot 5. The routine returns at once without a front-end world, with a game world, or while
     * drawing is suppressed, so it runs only in the front end. MainLoop uses it to refresh the
     * screen during a long operation.
     *
     * The body is not written, for the reason recorded on DrawFrame().
     *
     * @ghidraAddress 0x0010bfa0
     */
    virtual void DrawFrameSimple();

    /**
     * Unrecovered. Slot 6.
     *
     * The body forwards the game world to the GrooveWorld member at `0x0018de38` and nothing else.
     * Neither that member nor this slot has a recovered purpose.
     *
     * @ghidraAddress 0x0010c128
     */
    virtual void OnUnknownSlot6();

    /**
     * Start recording the session.
     *
     * Slot 7. Trips `Recording already in progress` when a recorder already exists and
     * `Cannot start recording from this state` when mState is non-zero. The two diagnostics are
     * what establish mState as a state word. Otherwise it installs a GameRecorder.
     *
     * @ghidraAddress 0x0010c420
     */
    virtual void StartRecording();

    /**
     * Replay a recorded session from a file.
     *
     * Slot 8. Trips `Cannot recreate game from this state` when mState is non-zero and
     * `Playback already in progress` when a playback already exists. Any recorder is destroyed
     * first, and the front-end world is then asked to tear its game down through `0x003d4890`.
     *
     * The installed GamePlayback reopens the file, reads three words from it, and runs Load() on
     * this manager, so a playback restores a saved session rather than feeding input back.
     *
     * @param file The recording to replay.
     * @param nFlag Passed to the installed object unchanged.
     * @ghidraAddress 0x0010c4b8
     */
    virtual void StartPlayback(const HxStr &file, int nFlag);

    /**
     * Unrecovered. Slot 9.
     *
     * Returns mUnknownfc.
     *
     * @return mUnknownfc.
     * @ghidraAddress 0x0010b870
     */
    virtual int GetUnknownfc();

    /**
     * Copy one persona onto the roster.
     *
     * Slot 10. The argument is copied into a fresh heap MetPersonaData rather than adopted, and the
     * copy is appended. The allocation tag is the literal `MetPersonaData`, which is what fixes the
     * element type of mPersonas.
     *
     * @param persona The persona to copy.
     * @ghidraAddress 0x00105e80
     */
    virtual void AddPersona(const MetPersonaData &persona);

    /**
     * Resolve the roster.
     *
     * Slot 11.
     *
     * @return The roster.
     * @ghidraAddress 0x0010b888
     */
    virtual std::vector<MetPersonaData *> *GetPersonas();

    /**
     * Delete every persona on the roster and empty it.
     *
     * Slot 12. Each element is destroyed through its own vtable slot 1, which sits at `+0x168`
     * inside a MetPersonaData.
     *
     * @ghidraAddress 0x0010be20
     */
    virtual void ClearPersonas();

    /**
     * Create the front-end world and hand it to the poller.
     *
     * Slot 13.
     *
     * @ghidraAddress 0x0010c168
     */
    virtual void Start();

    /**
     * Advance the game world while a playback runs.
     *
     * Slot 14. The routine ticks the poller, reads the EE cycle counter into the profile timer at
     * `0x007082c8`, and advances the game world only when the poller's field at `+0x38`, the
     * playback, and the world are all present. MainLoop drives it from one of its two periodic
     * timers.
     *
     * An earlier reading titled the slot for advancing the sound banks. Nothing in the body
     * supports that, and the three-way guard is what the title records instead.
     *
     * @ghidraAddress 0x00106e28
     */
    virtual void PollPlayback();

    /**
     * Resolve the world a game session runs in.
     *
     * Slot 15. Null until CreateWorld() runs.
     *
     * @return The game world, or null in the front end.
     * @ghidraAddress 0x0010b890
     */
    virtual GrooveWorld *GetWorld();

    /**
     * Resolve the front-end world.
     *
     * Slot 16. Null until Start() runs.
     *
     * @return The front-end world.
     * @ghidraAddress 0x0010b898
     */
    virtual MetaGameWorld *GetMetaWorld();

    /**
     * Resolve the controller reader.
     *
     * Slot 17.
     *
     * @return The poller the constructor created.
     * @ghidraAddress 0x0010b8a0
     */
    virtual InputPoller *GetPoller();

    /**
     * Unrecovered. Slot 18.
     *
     * Returns mUnknown18, which nothing recovered writes.
     *
     * @return mUnknown18.
     * @ghidraAddress 0x0010b8a8
     */
    virtual int GetUnknown18();

    /**
     * Resolve the tally.
     *
     * Slot 19. The subobject is embedded, so the accessor is an address computation rather than a
     * load.
     *
     * @return The tally.
     * @ghidraAddress 0x0010b8b0
     */
    virtual GameStats *GetStats();

    /**
     * Write the manager to a stream.
     *
     * Slot 20. Three words go out, mState, mUnknown08, and mGameMode, and the settings write
     * themselves afterwards through their own slot 2.
     *
     * @param pStream The stream to write to.
     * @ghidraAddress 0x0010c588
     */
    virtual void Save(OBStream *pStream);

    /**
     * Read the manager back from a stream.
     *
     * Slot 21. The three words come back in the order Save() wrote them and the settings read
     * themselves through their own slot 3. The three setters then run on the restored values, the
     * roster is emptied and given one persona titled `freq player 1`, and the world is created and
     * waited on.
     *
     * The wait is a spin. The routine calls `0x00194ca0` on the game world in a loop with no yield
     * until it reports the load finished.
     *
     * The body is not written. The persona copy, the world load, and the front-end notification all
     * run through members whose signatures are not settled.
     *
     * @param pStream The stream to read from.
     * @ghidraAddress 0x001072b0
     */
    virtual void Load(IBStream *pStream);

    /**
     * Report whether a playback is running.
     *
     * Slot 22.
     *
     * @return Non-zero while a playback exists.
     * @ghidraAddress 0x0010b8b8
     */
    virtual int IsPlaybackActive();

    /**
     * Record the connectivity mode and publish it to the script layer.
     *
     * Slot 23. The mode is published under script symbol 0x262 as one of `none`, `solo`, `local`,
     * and `net`, in that value order, and any other value publishes the empty string. Those four
     * literals are the whole evidence for the value set. The settings field mUnknown28 becomes the
     * test for `net`, and the change counter advances.
     *
     * @param nMode The mode, 0 through 3.
     * @ghidraAddress 0x0010c290
     */
    virtual void SetGameMode(int nMode);

    /**
     * Resolve the connectivity mode.
     *
     * Slot 24.
     *
     * @return The mode SetGameMode() recorded.
     * @ghidraAddress 0x0010b8c8
     */
    virtual int GetGameMode();

    /**
     * Resolve the settings a session starts with.
     *
     * Slot 25. The subobject is embedded, so the accessor is an address computation.
     *
     * @return The settings.
     * @ghidraAddress 0x0010b8d0
     */
    virtual GameParams *GetParams();

    /**
     * Report how many times the settings have changed.
     *
     * Slot 26. SetGameMode(), SetParams(), SetUnknown88(), and SetPlayMode() each advance the count
     * by one, and nothing resets it.
     *
     * @return The count.
     * @ghidraAddress 0x0010b8d8
     */
    virtual int GetChangeCount();

    /**
     * Replace the settings and republish the two modes.
     *
     * Slot 27. The two modes are republished from the members rather than from the new settings, so
     * the script layer sees the values already recorded. The change counter advances.
     *
     * @param params The settings to copy.
     * @ghidraAddress 0x0010c210
     */
    virtual void SetParams(const GameParams &params);

    /**
     * Unrecovered. Slot 28.
     *
     * Records the settings field mUnknown20 and publishes the raw value under script symbol 0x264.
     * No literal maps the value, so its meaning is unrecovered. The change counter advances.
     *
     * @param nValue The value to record.
     * @ghidraAddress 0x0010c3e0
     */
    virtual void SetUnknown88(int nValue);

    /**
     * Record the play mode and publish it to the script layer.
     *
     * Slot 29. The mode lands in the settings field mUnknown1c and is published under script symbol
     * 0x263 as one of `none`, `game`, and `jam`,
     * in that value order, and any other value publishes the empty string. Those three literals are
     * the whole evidence for the value set. The change counter advances.
     *
     * @param nMode The mode, 0 through 2.
     * @ghidraAddress 0x0010c348
     */
    virtual void SetPlayMode(int nMode);

    /**
     * Unrecovered. Slot 30.
     *
     * Returns the settings field mUnknown20.
     *
     * @return The value SetUnknown88() recorded.
     * @ghidraAddress 0x0010b8e0
     */
    virtual int GetUnknown88();

    /**
     * Resolve the play mode.
     *
     * Slot 31. Returns the settings field mUnknown1c.
     *
     * @return The mode SetPlayMode() recorded.
     * @ghidraAddress 0x0010b8e8
     */
    virtual int GetPlayMode();

    /**
     * Store a message on the embedded queue.
     *
     * Slot 32. The call runs through the queue's MsgSink subobject at `+0x14` rather than through
     * the queue itself, so it reaches MsgQueue::HandleMessage() virtually.
     *
     * @param pMsg The message to store.
     * @ghidraAddress 0x0010bee0
     */
    virtual void QueueMessage(Message *pMsg);

    /**
     * Enable or suppress drawing.
     *
     * Slot 33. The member records the inverse of the argument, and the two draw slots run only
     * while it is clear.
     *
     * @param nEnabled Non-zero to draw.
     * @ghidraAddress 0x0010b878
     */
    virtual void SetDrawEnabled(int nEnabled);

protected:
    /**
     * Act on a message.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x00107540
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Enter a local game.
     *
     * Slot 34. Creates the world, releases the front-end world's game, clears mUnknowna8, hands the
     * poller a cleared flag, and posts a command built on a 12-byte record. The message itself is
     * ignored, and HandleMessage() passes it all the same.
     *
     * The body is not written. The command post runs through five members whose signatures are not
     * settled.
     *
     * @param pMsg The message, ignored.
     * @ghidraAddress 0x00106720
     */
    virtual void OnBeginGameLocal(Message *pMsg);

    /**
     * Leave the game.
     *
     * Slot 35. Forwards EndGameMsg::mRestart to EndGame().
     *
     * @param pMsg The EndGameMsg.
     * @ghidraAddress 0x0010c148
     */
    virtual void OnEndGame(Message *pMsg);

    /**
     * Pause the session.
     *
     * Slot 36. Returns at once when already paused. Otherwise records the pause, stops the watchdog
     * clock unless the connectivity mode is `net`, reconnects the poller to the front-end world and
     * pauses it, silences and pauses the synthesiser, pauses the vibration, and hands the
     * front-end renderer a MetStartPauseMsg. The message itself is ignored.
     *
     * @param pMsg The message, ignored.
     * @ghidraAddress 0x001069a8
     */
    virtual void OnPauseGameSystem(Message *pMsg);

    /**
     * Resume the session.
     *
     * Slot 37. Returns at once when not paused. Otherwise clears the pause and undoes each step
     * OnPauseGameSystem() took, handing the poller to the game world. It also rebuilds the world's
     * input map while the world's mUnknown90 is zero. The message itself is ignored.
     *
     * @param pMsg The message, ignored.
     * @ghidraAddress 0x00106af8
     */
    virtual void OnUnpauseGameSystem(Message *pMsg);

    /**
     * Replay the recording the script layer nominates.
     *
     * Slot 38. The file comes from script symbol 0x26a and the flag is zero. The message is
     * ignored.
     *
     * @param pMsg The message, ignored.
     * @ghidraAddress 0x0010bf10
     */
    virtual void OnDoPlayback(Message *pMsg);

private:
    // 0x0010bec8. Reads mState and returns 1 on both paths, so the branch on the state has no
    // effect. The constructor, the destructor, SetParams(), and OnBeginGameLocal() all run it and
    // all discard the result.
    int CheckState();

    // 0x001068a0. Creates the game world with the application and this manager's tally, publishes
    // two of the settings under script symbols 0x277 and 0x27b, and hands the world the container
    // name from script symbol 0x38e. Load() and OnBeginGameLocal() are its two callers.
    void CreateWorld();

    // 0x0010c0c0. Sets mUnknownfc, spins on the world's load report at 0x00194ca0 until it
    // finishes, completes the load, adds the players, prepares the level, and reconnects the
    // poller. Load() inlines the same sequence rather than calling this.
    void FinishWorldLoad();

    // 0x0010c1f0. Forwards to AddPersonaPlayers(). FinishWorldLoad() and Load() call it.
    void AddPlayers();

    // 0x00106ec0. Deals the personas out in a shuffled order, each with one of the colour names
    // at 0x007cd3b0, and adds a player for each through the GrooveWorld routine at 0x0018c600.
    // Not written, because that routine and the random draw at 0x0052d098 are not declared.
    void AddPersonaPlayers();

    // 0x00106c08. The out-of-line body of OnEndGame(). Deletes the game world, ends a recording
    // and a playback, and then either queues a BeginGameLocalMsg or returns to the front end.
    void EndGame(int bRestart);

    // A state word. The two diagnostics StartRecording() and StartPlayback() trip both describe it
    // as the state, and both fire when it is non-zero. Load() is the only writer recovered, so its
    // value set is unrecovered.
    int mState;                 // +0x04
    int mUnknown08;             // +0x08
    GrooveWorld *mpWorld;       // +0x0c
    InputPoller *mpPoller;      // +0x10
    MetaGameWorld *mpMetaWorld; // +0x14
    // Not written by the constructor and not written anywhere recovered.
    int mUnknown18;                          // +0x18
    std::vector<MetPersonaData *> mPersonas; // +0x1c
    GameStats mStats;                        // +0x28
    GameParams mParams;                      // +0x68
    // The connectivity mode, one of the four values SetGameMode() publishes.
    int mGameMode;    // +0xa0
    int mChangeCount; // +0xa4
    // Set by Start() and cleared by OnBeginGameLocal(). DrawFrame() runs one extra pass while it is
    // set, so it distinguishes the front end from a game session.
    int mUnknowna8;           // +0xa8
    GameRecorder *mpRecorder; // +0xac the recorder StartRecording() installs
    GamePlayback *mpPlayback; // +0xb0 the playback StartPlayback() installs
    // The constructor clears both of its words and the destructor frees the buffer at +0xb8 only
    // when it is set, which is HxStr's own destruction. Nothing recovered writes it otherwise.
    HxStr mUnknownb4; // +0xb4
    // Neither written by the constructor nor touched anywhere recovered. The field exists
    // because the queue starts at +0xc0 while the run of cleared fields ends at +0xb8, so four
    // bytes sit between them.
    int mUnknownbc;  // +0xbc
    MsgQueue mQueue; // +0xc0
    // Set by FinishWorldLoad() and by Load(), and never cleared. GetUnknownfc() is its one reader.
    int mUnknownfc; // +0xfc
    // Cleared by OnBeginGameLocal(). Nothing recovered sets it.
    int mUnknown100; // +0x100
    // Set by OnPauseGameSystem() and cleared by OnUnpauseGameSystem(), each of which returns early
    // on the value it would write.
    int mPaused; // +0x104
    // The inverse of SetDrawEnabled()'s argument. The constructor sets it, so a fresh manager draws
    // nothing.
    int mDrawSuppressed; // +0x108
};
