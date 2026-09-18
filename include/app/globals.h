#pragma once

#include "app/mainloop.h"
#include "app/scriptsink.h"
#include "app/watchdog.h"
#include "app/watchdogtimer.h"
#include "game/gamemanagerimpl.h"
#include "stream/obstream.h"
#include "synth/ps2hardsynth.h"

/** Size of the buffer that Globals opens its log stream over. */
constexpr int kLogBufferSize = 0x19000;

/**
 * Abstract owner of the process-wide services.
 *
 * `7Globals` in the RTTI descriptor at `0x0086f5d0`, with no base. The compiler therefore places
 * the vptr after the seven declared pointers, at `+0x1c`, which makes the class 0x20 bytes. The
 * destructor is declared first and takes vtable slot 1 of the table at `0x007cee78`; the two pure
 * virtuals follow it in slots 2 and 3, both pointing at the shared pure-virtual handler.
 *
 * Application is the only class in the image that derives from it, and the two overrides are
 * titled by the two exception templates the script layer registers,
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
 * Eight further accessors forward into the game manager and return an object whose owning class
 * belongs to the renderer rather than here, so they are not titled yet: `0x00118ce8`, `0x00118d18`,
 * `0x00118d70`, `0x00118da0`, `0x00118dd8`, `0x00118e08`, `0x00118e38`, and `0x00118ec8`.
 */
class Globals {
public:
    /**
     * Clear the service pointers.
     *
     * The synth and the script sink are not cleared, so Shutdown() tests uninitialised memory
     * when it runs before Init().
     *
     * @ghidraAddress 0x00118c40
     */
    Globals();

    /**
     * @ghidraAddress 0x00118c68
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
     * @ghidraAddress 0x001170d0
     */
    void Init();

    /**
     * Destroy every process-wide service.
     *
     * The watchdog is flushed first, then each service is destroyed in the reverse of the order
     * Init() created them. The routine is unreferenced in the image.
     *
     * @ghidraAddress 0x00117270
     */
    void Shutdown();

    /**
     * Create the hardware synth.
     *
     * @ghidraAddress 0x00118c98
     */
    void CreateSynth();

    /**
     * Drive the main loop until it stops.
     *
     * @ghidraAddress 0x00118cc8
     */
    void RunMainLoop();

    /**
     * @return The game manager.
     * @ghidraAddress 0x00118eb8
     */
    GameManagerImpl *GetGameManager();

    /**
     * @return The hardware synth.
     * @ghidraAddress 0x00118ec0
     */
    Ps2HardSynth *GetSynth();

    /**
     * @return The long-operation watchdog.
     * @ghidraAddress 0x00118eb0
     */
    Watchdog *GetWatchdog();

    /**
     * @return The watchdog's time base.
     * @ghidraAddress 0x00118e70
     */
    WatchdogTimer *GetWatchdogTimer();

private:
    GameManagerImpl *mGameManager; // +0x00
    MainLoop *mMainLoop;           // +0x04
    Ps2HardSynth *mSynth;          // +0x08
    Watchdog *mWatchdog;           // +0x0c
    WatchdogTimer *mWatchdogTimer; // +0x10
    ScriptSink *mScriptSink;       // +0x14
    OBStream *mLog;                // +0x18
};

/**
 * Buffer that Globals opens its log stream over.
 *
 * @ghidraAddress 0x0086f7d0
 */
extern char g_abLogBuffer[kLogBufferSize];
