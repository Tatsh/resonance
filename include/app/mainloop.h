#pragma once

#include "app/task.h"
#include "app/watchdog.h"
#include "game/gamemanagerimpl.h"
#include "os/hxstr.h"

/**
 * The game's frame loop.
 *
 * `8MainLoop` in the RTTI descriptor at `0x00901f80`, deriving from Task. The Task subobject brings
 * its inherited virtual base, so the Attachment subobject moves to the end of this object at
 * `+0x38` and the declared members occupy `+0x0c` through `+0x37`, which makes the object 0x40
 * bytes. Its own table is at `0x007e7ce0` and its table for the Attachment subobject is at
 * `0x007e7cc0`, whose entries adjust `this` by `-0x38`.
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
 * game manager and the other services the long-operation watchdog. FireDueTimers() is what
 * a blocked long operation calls back into through the poll callback the constructor installs, so
 * both timers continue to run while the frame loop itself is stalled.
 */
class MainLoop : public Task {
public:
    /**
     * Create the frame loop and take over the long-operation callbacks.
     *
     * @param pWatchdog The long-operation watchdog to service.
     * @param pGameManager The game manager to draw through.
     * @ghidraAddress 0x001ec998
     */
    MainLoop(Watchdog *pWatchdog, GameManagerImpl *pGameManager);

    /**
     * Drop the instance pointer and remove the poll callback.
     *
     * The redraw callback the constructor installed is not removed.
     *
     * @ghidraAddress 0x001ef158
     */
    virtual ~MainLoop();

    /**
     * Pump the frame loop until it stops.
     *
     * Nothing in the image clears mRunning, so the routine does not return.
     *
     * @ghidraAddress 0x001ef290
     */
    void Run();

    /**
     * Fire every timer whose deadline has passed, then recompute the next deadline.
     *
     * @param nNowNs The current reading of the frame clock in nanoseconds.
     * @ghidraAddress 0x001ef308
     */
    void FireDueTimers(long long nNowNs);

    /**
     * Recompute the earliest of the two timer deadlines.
     *
     * @ghidraAddress 0x001ef2e0
     */
    void UpdateNextDeadline();

    /**
     * Write the watchdog's readings out now and disarm the scheduled write.
     *
     * The routine is unreferenced in the image.
     *
     * @ghidraAddress 0x001ef230
     */
    void FlushWatchdogNow();

    /**
     * Schedule the watchdog's readings to be written out after a further run of frames.
     *
     * The routine is unreferenced in the image.
     *
     * @param nFrames The number of frames to wait.
     * @ghidraAddress 0x001ef260
     */
    void FlushWatchdogAfter(int nFrames);

protected:
    /**
     * Report no progress, because the frame loop never finishes.
     *
     * @return Always zero.
     * @ghidraAddress 0x001ef118
     */
    virtual float Progress();

    /**
     * Report an empty title.
     *
     * @return An empty string.
     * @ghidraAddress 0x001ef128
     */
    virtual HxStr Name();

    /**
     * Run one frame.
     *
     * @return Always 1, so the frame loop is never retired by the run ring.
     * @ghidraAddress 0x001ecc90
     */
    virtual int Poll();

private:
    // Rearm the timer and poll the game manager. The routine it dispatches ticks the input
    // poller, accumulates a profile timer, and advances the game world when the world and the
    // playback object are both present; nothing in it concerns a sound bank, and no literal
    // attests the word, so the earlier spelling of both this member and its callee is dropped.
    // 0x001ef3d0
    void FirePollTimer(long long nNowNs);

    // Rearm the watchdog timer and service the watchdog. 0x001ef398
    void FireWatchdogPoll(long long nNowNs);

    // Runs after the frame is drawn, with an empty body. 0x001ef410
    void PostDraw();

    // Long-operation poll callback, which drives the two periodic timers while the frame loop is
    // blocked. 0x001ec7d0
    static void PumpTimers();

    // Long-operation redraw callback. It refreshes the display at most once every eighteen
    // milliseconds of the profiler clock. 0x001ec8c0
    static void KeepAliveDraw();

    int mRunning;                  // +0x0c
    long long mNextBankPollNs;     // +0x10
    long long mNextWatchdogPollNs; // +0x18
    long long mNextDeadlineNs;     // +0x20
    int mFrameCount;               // +0x28
    int mFlushFrame;               // +0x2c
    Watchdog *mWatchdog;           // +0x30
    GameManagerImpl *mGameManager; // +0x34
};

/**
 * The single frame loop, or null before Globals::Init() and after the loop is destroyed.
 *
 * @ghidraAddress 0x00694740
 */
extern MainLoop *g_pMainLoop;
