#pragma once

class GameManagerImpl;
class GrooveWorld;
class IOBPreallocMemStream;
class LevelData;
class MainLoop;
class MetaGameWorld;
class PlayMap;
class Ps2HardSynth;
class ScriptSink;

namespace Sch {
class Scheduler;
class TickClock;
class TimeClock;
} // namespace Sch

/** Size of the buffer that Globals opens its log stream over. */
constexpr int kLogBufferSize = 0x19000;

/**
 * Abstract owner of the process-wide services.
 *
 * Its RTTI descriptor is at `0x0086f5d0`. It has no base. The compiler therefore places the vptr
 * after the seven declared pointers, at `+0x1c`, and the class is 0x20 bytes. The destructor
 * is declared first and takes vtable slot 1 of the table at `0x007cee78`; the two pure virtuals
 * follow it in slots 2 and 3, both pointing at the shared pure-virtual handler.
 *
 * Application is the only class in the image that derives from it, and the names of the two
 * overrides come from the two exception templates the script layer registers,
 * `An exception was thrown by the function Application::Run().` and
 * `An exception was thrown by the function Application::ExitInstance().`
 *
 * Init() creates every service and Shutdown() destroys them in the reverse order.
 *
 * Every service pointer is private. Four of them have an accessor of their own, and foreign code
 * reads those four only through the accessor. A scan of all 420 Application::shared() call sites
 * found exactly one direct access to the returned object, the vptr load in main() that dispatches
 * Run(), so no code outside the class reads any of the seven fields and no Application member
 * reads one either. The accessors are themselves the out-of-line copies of inline members, and
 * dozens of call sites inlined their own copy.
 *
 * Four further accessors forward into the game manager and are not declared yet: `0x00118ce8`,
 * `0x00118d18`, `0x00118d70`, and `0x00118ec8`.
 */
class Globals {
public:
    /**
     * Clear the service pointers.
     *
     * The synth and the script sink are not cleared, so Shutdown() tests uninitialised memory
     * when it runs before Init().
     *
     * @ghidraAddress NTSC-U/C: 0x00118c40
     * @ghidraAddress PAL: 0x00119190
     */
    Globals();

    /**
     * @ghidraAddress NTSC-U/C: 0x00118c68
     * @ghidraAddress PAL: 0x001191b8
     */
    virtual ~Globals();

    /**
     * Run the game until it exits.
     *
     * @return The process result.
     */
    virtual int Run() = 0;

    /**
     * Tear the game down as it exits.
     *
     * @return The process result.
     */
    virtual int ExitInstance() = 0;

    /**
     * Create every process-wide service.
     *
     * @ghidraAddress NTSC-U/C: 0x001170d0
     * @ghidraAddress PAL: 0x001175a0
     */
    void Init();

    /**
     * Destroy every process-wide service.
     *
     * The watchdog is flushed first, then each service is destroyed in the reverse of the order
     * Init() created them. The routine is unreferenced in the NTSC-U/C image. The PAL build calls
     * it from Application::ExitInstance() and does not destroy the synth.
     *
     * @ghidraAddress NTSC-U/C: 0x00117270
     * @ghidraAddress PAL: 0x00117740
     */
    void Shutdown();

    /**
     * Create the hardware synth.
     *
     * @ghidraAddress NTSC-U/C: 0x00118c98
     * @ghidraAddress PAL: 0x001191e8
     */
    void CreateSynth();

    /**
     * Drive the main loop until it stops.
     *
     * @ghidraAddress NTSC-U/C: 0x00118cc8
     * @ghidraAddress PAL: 0x00119218
     */
    void RunMainLoop();

#ifdef VIDEO_STANDARD_PAL
    /**
     * Stop the main loop after its current pass.
     *
     * No code calls it. The title is inferred.
     *
     * @ghidraAddress PAL: 0x00119238
     */
    void StopMainLoop();
#endif

    /**
     * @return The game manager.
     * @ghidraAddress NTSC-U/C: 0x00118eb8
     * @ghidraAddress PAL: 0x00119418
     */
    GameManagerImpl *GetGameManager();

    /**
     * @return The hardware synth.
     * @ghidraAddress NTSC-U/C: 0x00118ec0
     * @ghidraAddress PAL: 0x00119420
     */
    Ps2HardSynth *GetSynth();

    /**
     * @return The command scheduler.
     * @ghidraAddress NTSC-U/C: 0x00118eb0
     * @ghidraAddress PAL: 0x00119410
     */
    Sch::Scheduler *GetWatchdog();

    /**
     * @return The scheduler's time base.
     * @ghidraAddress NTSC-U/C: 0x00118e70
     * @ghidraAddress PAL: 0x001193d0
     */
    Sch::TimeClock *GetWatchdogTimer();

    /**
     * @return The sink that runs posted script text.
     * @ghidraAddress NTSC-U/C: 0x00118ef8
     * @ghidraAddress PAL: 0x00119458
     */
    ScriptSink *GetScriptSink();

    /**
     * @return The shared log stream, which the remix tasks also use as the remix payload buffer.
     * @ghidraAddress NTSC-U/C: 0x00118f00
     * @ghidraAddress PAL: 0x00119460
     */
    IOBPreallocMemStream *GetLog();

    /**
     * Rewind the shared log stream and report it.
     *
     * LoadRemixMCT's constructor and MetRemixManager::AsyncCallbackDone() use the rewound stream
     * as the destination of a remix payload. The title is inferred.
     *
     * @return The log stream, rewound through Reset().
     * @ghidraAddress NTSC-U/C: 0x00118f08
     * @ghidraAddress PAL: 0x00119468
     */
    IOBPreallocMemStream *GetResetLog();

    /**
     * Report the game manager's world.
     *
     * Reads mGameManager directly rather than through GetGameManager().
     *
     * @return GameManagerImpl::GetWorld().
     * @ghidraAddress NTSC-U/C: 0x00118d40
     * @ghidraAddress PAL: 0x001192a0
     */
    GrooveWorld *GetWorld();

    /**
     * Report the game manager's front-end world.
     *
     * Reads mGameManager directly rather than through GetGameManager(). Two script commands call
     * it.
     *
     * @return GameManagerImpl::GetMetaWorld().
     * @ghidraAddress NTSC-U/C: 0x00118d70
     * @ghidraAddress PAL: 0x001192d0
     */
    MetaGameWorld *GetMetaWorld();

    /**
     * Forward GameManagerImpl::GetUnwrittenValue().
     *
     * Reads mGameManager directly. The image records no caller.
     *
     * @return GameManagerImpl::GetUnwrittenValue().
     * @ghidraAddress NTSC-U/C: 0x00118ec8
     * @ghidraAddress PAL: 0x00119428
     */
    int GetUnwrittenValue();

    /**
     * Report the game manager's play mode.
     *
     * @return GameManagerImpl::GetPlayMode().
     * @ghidraAddress NTSC-U/C: 0x00118dd8
     * @ghidraAddress PAL: 0x00119338
     */
    int GetPlayMode();

    /**
     * Report the game manager's game mode.
     *
     * @return GameManagerImpl::GetGameMode().
     * @ghidraAddress NTSC-U/C: 0x00118e08
     * @ghidraAddress PAL: 0x00119368
     */
    int GetGameMode();

    /**
     * Report the song clock of the game manager's world.
     *
     * The body composes GetWorld() with GrooveWorld::GetSongClock().
     *
     * @return The world's song clock.
     * @ghidraAddress NTSC-U/C: 0x00118e78
     * @ghidraAddress PAL: 0x001193d8
     */
    Sch::TickClock *GetSongClock();

    /**
     * Report the play map of the world's level.
     *
     * The body composes GetWorld() with GrooveWorld::GetPlayMap().
     *
     * @return The play map.
     * @ghidraAddress NTSC-U/C: 0x00118da0
     * @ghidraAddress PAL: 0x00119300
     */
    PlayMap *GetPlayMap();

    /**
     * Report whether the session is a jukebox session.
     *
     * @return GameParams::mJukeboxMode of GameManagerImpl::GetParams().
     * @ghidraAddress NTSC-U/C: 0x00118e38
     * @ghidraAddress PAL: 0x00119398
     */
    int IsJukeboxMode();

    /**
     * Report the world's level.
     *
     * The body composes GetWorld() with GrooveWorld::GetLevel().
     *
     * @return The level.
     * @ghidraAddress NTSC-U/C: 0x00118d18
     * @ghidraAddress PAL: 0x00119278
     */
    LevelData *GetLevel();

    /**
     * Report the tempo of the song clock's tempo map.
     *
     * Reads Sch::TempoMap::mMicrosecondsPerQuarter through Sch::TickClock::mTempoMap inline, after
     * a finiteness test of song position 0 whose result is discarded. The title is inferred.
     *
     * @return The tempo, in microseconds per quarter note.
     * @ghidraAddress NTSC-U/C: 0x00118ce8
     * @ghidraAddress PAL: 0x00119248
     */
    int GetTempo();

private:
    GameManagerImpl *mGameManager;  // +0x00
    MainLoop *mMainLoop;            // +0x04
    Ps2HardSynth *mSynth;           // +0x08
    Sch::Scheduler *mWatchdog;      // +0x0c
    Sch::TimeClock *mWatchdogTimer; // +0x10
    ScriptSink *mScriptSink;        // +0x14
    IOBPreallocMemStream *mLog;     // +0x18
};

/**
 * Buffer that Globals opens its log stream over.
 *
 * @ghidraAddress NTSC-U/C: 0x0086f7d0
 * @ghidraAddress PAL: 0x008b3ed0
 */
extern char g_abLogBuffer[kLogBufferSize];
