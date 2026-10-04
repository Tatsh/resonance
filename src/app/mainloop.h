#pragma once

#include "app/task.h"
#include "os/hxstr.h"

class GameManagerImpl;

namespace Sch {
class Scheduler;
} // namespace Sch

/**
 * The game's frame loop.
 *
 * Its RTTI descriptor is at `0x00901f80`. It derives from Task. The declared members occupy `+0x0c`
 * through `+0x37`, and the virtual Attachment base closes the object at `+0x38`. The allocation is
 * 0x40 bytes. The MainLoop table is at `0x007e7ce0`.
 *
 * Globals::Init() creates the single instance and Globals::RunMainLoop() drives it. Every member
 * below is private, because the only code that reads one is a member of this class.
 *
 * Run() pumps Poll() until mRunning is cleared. Nothing in the image clears it, so the loop is
 * where the whole game runs and Run() never returns. Poll() samples the profiler clock, advances
 * the asynchronous loader and the synth stream, recomputes the frames-per-second reading once a
 * second, and then requests one frame from the game manager.
 *
 * Two periodic timers run alongside the frame, each with a four-millisecond period. One polls the
 * game manager and the other services the long-operation watchdog. Callback() is what
 * a blocked long operation calls back into through the poll callback the constructor installs, so
 * both timers continue to run while the frame loop itself is stalled.
 */
class MainLoop : public Task {
public:
    /**
     * Create the frame loop and take over the long-operation callbacks.
     *
     * @param nInCharge The call site always passes 1. The flag selects virtual-base setup the
     * compiler emits. The body does not use the flag.
     * @param pWatchdog The long-operation watchdog to service.
     * @param pGameManager The game manager to draw through.
     * @ghidraAddress NTSC-U/C: 0x001ec998
     * @ghidraAddress PAL: 0x001f2c20
     */
    MainLoop(int nInCharge, Sch::Scheduler *pWatchdog, GameManagerImpl *pGameManager);

    /**
     * Drop the instance pointer and remove the poll callback.
     *
     * The redraw callback the constructor installed is not removed.
     *
     * @ghidraAddress NTSC-U/C: 0x001ef158
     * @ghidraAddress PAL: 0x001f54f8
     */
    virtual ~MainLoop();

    /**
     * Pump the frame loop until it stops.
     *
     * Nothing in the image clears mRunning, so the routine does not return.
     *
     * @ghidraAddress NTSC-U/C: 0x001ef290
     * @ghidraAddress PAL: 0x001f5630
     */
    void Run();

#ifdef VIDEO_STANDARD_PAL
    /**
     * Clear mRunning so that Run() returns after the current pass.
     *
     * Only Globals::StopMainLoop() uses it, and no code calls that routine.
     */
    void Stop() {
        mRunning = 0;
    }
#endif

    /**
     * Fire every timer whose deadline has passed, then recompute the next deadline.
     *
     * @param nNowNs The current reading of the frame clock in nanoseconds.
     * @ghidraAddress NTSC-U/C: 0x001ef308
     * @ghidraAddress PAL: 0x001f56a8
     */
    void Callback(long long nNowNs);

    /**
     * Recompute the earliest of the two timer deadlines.
     *
     * @ghidraAddress NTSC-U/C: 0x001ef2e0
     * @ghidraAddress PAL: 0x001f5680
     */
    void UpdateCallbackTime();

    /**
     * Write the watchdog's readings out now and disarm the scheduled write.
     *
     * The routine is unreferenced in the image.
     *
     * @ghidraAddress NTSC-U/C: 0x001ef230
     * @ghidraAddress PAL: 0x001f55d0
     */
    void Resume();

    /**
     * Schedule the watchdog's readings to be written out after a further run of frames.
     *
     * The routine is unreferenced in the image.
     *
     * @param nFrames The number of frames to wait.
     * @ghidraAddress NTSC-U/C: 0x001ef260
     * @ghidraAddress PAL: 0x001f5600
     */
    void Step(int nFrames);

protected:
    /**
     * Report no progress, because the frame loop never finishes.
     *
     * @return Always zero.
     * @ghidraAddress NTSC-U/C: 0x001ef118
     * @ghidraAddress PAL: 0x001f54b8
     */
    virtual float Progress();

    /**
     * Report an empty title.
     *
     * @return An empty string.
     * @ghidraAddress NTSC-U/C: 0x001ef128
     * @ghidraAddress PAL: 0x001f54c8
     */
    virtual HxStr Name() const;

    /**
     * Run one frame.
     *
     * @return Always 1, so the frame loop is never retired by the run ring.
     * @ghidraAddress NTSC-U/C: 0x001ecc90
     * @ghidraAddress PAL: 0x001f2f98
     */
    virtual int Poll();

public:
    /**
     * Drive the two periodic timers while the frame loop is blocked.
     *
     * The long-operation poll callback. Renderer's constructor also calls it directly, at
     * `0x0042c3bc`, before it waits for the common loads.
     *
     * @ghidraAddress NTSC-U/C: 0x001ec7d0
     * @ghidraAddress PAL: 0x001f2a58
     */
    static void PumpTimers();

    /**
     * Redraw the display during a long operation.
     *
     * Refreshes the display at most once every 18 milliseconds of the profiler clock, or
     * every 21 milliseconds in the PAL build. Public because GrooveWorld::EndLevel() at
     * `0x0018eb70` installs it as the bank-load progress hook.
     *
     * @ghidraAddress NTSC-U/C: 0x001ec8c0
     * @ghidraAddress PAL: 0x001f2b48
     */
    static void KeepAliveDraw();

private:
    /**
     * Rearm the timer and poll the game manager.
     *
     * The routine it dispatches ticks the input poller, accumulates a profile timer, and advances
     * the game world when the world and the playback object are both present; nothing in it
     * concerns a sound bank, and no literal attests the word, so the earlier spelling of both this
     * member and its callee is dropped.
     *
     * @ghidraAddress NTSC-U/C: 0x001ef3d0
     * @ghidraAddress PAL: 0x001f5770
     */
    void FirePollTimer(long long nNowNs);

    /**
     * Rearm the watchdog timer and service the watchdog.
     *
     * @ghidraAddress NTSC-U/C: 0x001ef398
     * @ghidraAddress PAL: 0x001f5738
     */
    void SchCallback(long long nNowNs);

    /**
     * Runs after the frame is drawn, with an empty body.
     *
     * @ghidraAddress NTSC-U/C: 0x001ef410
     * @ghidraAddress PAL: 0x001f57b0
     */
    void PostDraw();

    int mRunning;                  // +0x0c
    long long mNextBankPollNs;     // +0x10
    long long mNextWatchdogPollNs; // +0x18
    long long mNextDeadlineNs;     // +0x20
    int mFrameCount;               // +0x28
    int mFlushFrame;               // +0x2c
    Sch::Scheduler *mWatchdog;     // +0x30
    GameManagerImpl *mGameManager; // +0x34
    // The virtual Attachment base closes the object at `+0x38`; the compiler constructs the base
    // with one reference and the base table. The constructor's flag-gated stores perform the
    // construction in the image. No member is declared for the base here.
};

/**
 * The single frame loop, or null before Globals::Init() and after the loop is destroyed.
 *
 * @ghidraAddress NTSC-U/C: 0x00694740
 * @ghidraAddress PAL: 0x006d5a48
 */
extern MainLoop *g_pMainLoop;
