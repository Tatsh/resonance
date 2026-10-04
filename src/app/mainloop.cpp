#include "app/mainloop.h"

#include <stdio.h>

#include "app/application.h"
#include "app/longop.h"
#include "app/scheduler.h"
#include "game/gamemanagerimpl.h"
#include "os/cycles.h"
#include "os/mem.h"
#include "profile/profiler.h"
#include "rnd/asyncloader.h"
#include "synth/ps2hardsynth.h"

namespace {

constexpr long long kNanosecondsPerMillisecond = 1000000;
constexpr long long kNanosecondsPerSecond = 1000000000;

// Half a millisecond, added before the division that turns the frames-per-second window into
// milliseconds so that the quotient rounds rather than truncates.
constexpr long long kHalfMillisecondNs = 500000;

constexpr double kMillisecondsPerSecondDouble = 1000.0;

// Period of both periodic timers.
constexpr long long kTimerPeriodNs = 4000000;

// Frame number that disarms the scheduled watchdog flush, because the loop never runs that long.
constexpr int kFlushFrameNever = 100000000;

// Shortest interval between two long-operation redraws, a little over one field.
#ifdef VIDEO_STANDARD_PAL
constexpr long long kKeepAliveIntervalMs = 21;
#else
constexpr long long kKeepAliveIntervalMs = 18;
#endif

// The length is not recovered. The label it receives is eleven characters.
constexpr int kFrameLabelSize = 32;

// The profiler's timer slots that the frame loop labels.
enum AppTimer { kAppTimerPreDraw = 0, kAppTimerDraw = 1, kAppTimerAsync = 2, kAppTimerBank = 3 };

long long s_qwElapsedMs;

// The initialisers below are run from this translation unit's static-initialisation function at
// `0x001ef068`, rather than folded into the image, because a 64-bit initialiser is not a constant
// expression to this compiler. That is why the zero is stored explicitly too.
long long s_qwFpsWindowStartNs = 0;
long long s_qwFpsWindowEndNs = kNanosecondsPerSecond;
long long s_qwLastKeepAliveMs;
long long s_qwKeepAliveNowMs;
int s_nFramesPerSecond;
int s_nFramesThisWindow;
int s_nPollTicks;
int s_bInKeepAliveDraw;
MainLoop *s_pPumpedLoop;
Sch::Scheduler *s_pPumpedWatchdog;

// Reading of the frame clock in nanoseconds, measured from the origin the scheduler's clock
// recorded when the run started.
inline long long FrameClockNs(Sch::Scheduler *pWatchdog) {
    return (GetElapsedMilliseconds() - pWatchdog->mClock.mOriginMs) * kNanosecondsPerMillisecond;
}

} // namespace

// NTSC-U/C: 0x00694740, PAL: 0x006d5a48
MainLoop *g_pMainLoop;

MainLoop::MainLoop(int nInCharge, Sch::Scheduler *pWatchdog, GameManagerImpl *pGameManager) {
    // The call passes 1. The value selects virtual-base setup the compiler emits. The body
    // does not use the flag.
    (void)nInCharge;
    mRunning = 0;
    mNextBankPollNs = 0;
    mNextWatchdogPollNs = 0;
    mNextDeadlineNs = 0;
    mFrameCount = 0;
    mFlushFrame = kFlushFrameNever;
    mWatchdog = pWatchdog;
    mGameManager = pGameManager;
    g_pMainLoop = this;
    SetLongOperationPollProc(PumpTimers);
    SetLongOperationDrawProc(KeepAliveDraw);
    g_profileTimers[kAppTimerPreDraw].mName = HxStr("    app predraw");
    g_profileTimers[kAppTimerDraw].mName = HxStr("    app draw");
    g_profileTimers[kAppTimerAsync].mName = HxStr("    app async");
    g_profileTimers[kAppTimerBank].mName = HxStr("    app bank");
    UpdateCallbackTime();
}

MainLoop::~MainLoop() {
    g_pMainLoop = nullptr;
    SetLongOperationPollProc(nullptr);
    // Yes, the binary retains the redraw callback.
}

void MainLoop::Run() {
    mRunning = 1;
    do {
        Poll(); // Yes, the binary discards this call's result and tests mRunning instead.
    } while (mRunning != 0);
}

void MainLoop::UpdateCallbackTime() {
    mNextDeadlineNs = mNextWatchdogPollNs < mNextBankPollNs ? mNextWatchdogPollNs : mNextBankPollNs;
}

void MainLoop::FirePollTimer(long long nNowNs) {
    mNextBankPollNs = nNowNs + kTimerPeriodNs;
    mGameManager->PollPlayback();
}

void MainLoop::SchCallback(long long nNowNs) {
    mNextWatchdogPollNs = nNowNs + kTimerPeriodNs;
    mWatchdog->Service();
    PollSynthEvents();
}

void MainLoop::Callback(long long nNowNs) {
    if (nNowNs >= mNextBankPollNs) {
        FirePollTimer(nNowNs);
    }
    if (nNowNs >= mNextWatchdogPollNs) {
        SchCallback(nNowNs);
    }
    UpdateCallbackTime();
}

void MainLoop::Resume() {
    mFlushFrame = kFlushFrameNever;
    mWatchdog->Flush();
}

void MainLoop::Step(int nFrames) {
    mFlushFrame = mFrameCount + nFrames;
    mWatchdog->Flush();
}

float MainLoop::Progress() {
    return 0.0f;
}

HxStr MainLoop::Name() const {
    return HxStr("");
}

int MainLoop::Poll() {
    s_qwElapsedMs = GetElapsedMilliseconds();

    char szFrameLabel[kFrameLabelSize];
    sprintf(szFrameLabel, "Frame: %d\n", s_nFramesThisWindow);
    MemLogWriteMarker(szFrameLabel);

    RndAsyncLoader::PollAsyncLoads();
    PollSynthStream();

    long long nNowNs = FrameClockNs(mWatchdog);
    if (s_qwFpsWindowEndNs < nNowNs) {
        int nWindowMs = static_cast<int>((nNowNs - s_qwFpsWindowStartNs + kHalfMillisecondNs) /
                                         kNanosecondsPerMillisecond);
        float flFps = static_cast<float>(static_cast<double>(s_nFramesThisWindow) *
                                         kMillisecondsPerSecondDouble / nWindowMs);
        int nFps = static_cast<int>(flFps + 0.5);
        if (nFps != s_nFramesPerSecond) {
            s_nFramesPerSecond = nFps;
        }
        s_qwFpsWindowEndNs += kNanosecondsPerSecond;
        s_qwFpsWindowStartNs = nNowNs;
        s_nPollTicks = 0;
        s_nFramesThisWindow = 0;
    }

    UpdateCallbackTime();
    mGameManager->DrawFrame();
    PostDraw();
    ++s_nFramesThisWindow;
    ++mFrameCount;
    return 1;
}

void MainLoop::PostDraw() {
}

void MainLoop::PumpTimers() {
    ++s_nPollTicks;
    if (s_pPumpedLoop == nullptr) {
        s_pPumpedLoop = g_pMainLoop;
        s_pPumpedWatchdog = g_pMainLoop->mWatchdog;
    }
    long long nNowNs = FrameClockNs(s_pPumpedWatchdog);
    if (nNowNs >= s_pPumpedLoop->mNextDeadlineNs) {
        s_pPumpedLoop->Callback(nNowNs);
    }
}

void MainLoop::KeepAliveDraw() {
    long long nNowMs = GetElapsedMilliseconds();
    if (nNowMs - s_qwLastKeepAliveMs < kKeepAliveIntervalMs) {
        return;
    }
    s_qwKeepAliveNowMs = nNowMs;
    // The binary takes a maximum into s_qwLastKeepAliveMs and then overwrites it, so the
    // comparison has no effect.
    if (s_qwLastKeepAliveMs < s_qwElapsedMs) {
        s_qwLastKeepAliveMs = s_qwElapsedMs;
    }
    s_qwLastKeepAliveMs = nNowMs;
    if (s_bInKeepAliveDraw == 0) {
        s_bInKeepAliveDraw = 1;
        Application::shared()->GetGameManager()->DrawFrameSimple();
        s_bInKeepAliveDraw = 0;
    }
}
