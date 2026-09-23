#include "app/watchdog.h"

#include <exception>

#include "app/watchdogplayback.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "sch/command.h"

namespace {

// The value of mStreamMode while a recording plays back.
constexpr int kStreamModePlayback = 2;

// How far g_llWatchdogSecondNs may trail the clock before Service() catches it up.
constexpr long long kWatchdogSecondNs = 1000000000LL;

// A clock this far past the current time is marked back to it rather than run through.
constexpr long long kWatchdogCatchUpLimitNs = 6000000000LL;

// How long Service() shows the text of an exception a command raised.
constexpr int kCommandErrorDuration = 50;

} // namespace

// 0x006f8a80
long long g_llWatchdogSecondNs;

// 0x004a9858
Watchdog::Watchdog() : mStreamMode(0), mUnknown10(0), mPlayback(nullptr), mNowNs(0), mBlocked(0) {
}

// 0x004ac7b0
void Watchdog::QueueReplayed(Sch::TimedCommand *pCommand) {
    pCommand->mCommand->mQueued = 1;
    if (mBlocked != 0) {
        return;
    }
    if (pCommand != nullptr) {
        ++pCommand->mRefs;
    }
    mQueue.insert(pCommand);
}

// 0x004aa260
void Watchdog::WithdrawByCmdID(const CmdID &id) {
    if (id.mValue <= 0) {
        return;
    }
    for (auto it = mQueue.begin(); it != mQueue.end(); ++it) {
        Sch::TimedCommand *pCommand = *it;
        if (pCommand->mCmdID.mValue == id.mValue) {
            mQueue.erase(it);
            if (pCommand != nullptr) {
                pCommand->Release();
            }
            return;
        }
    }
}

// 0x004a9d00
void Watchdog::Withdraw(Sch::TimedCommand *pCommand) {
    const auto it = mQueue.lower_bound(pCommand);
    if (it == mQueue.end()) {
        return;
    }
    mQueue.erase(it);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

// 0x004aa848
void Watchdog::Service() {
    long long nNow = mClock.Now();
    if (g_llWatchdogSecondNs + kWatchdogSecondNs < nNow) {
        g_llWatchdogSecondNs = nNow;
    }
    if (nNow - mNowNs > kWatchdogCatchUpLimitNs) {
        mClock.Mark(mNowNs);
        g_llWatchdogSecondNs = mClock.Now();
        nNow = mClock.Now();
    }
    for (;;) {
        const auto it = mQueue.begin();
        if (it == mQueue.end() || nNow < (*it)->mDueTick.mValue) {
            if (mNowNs < nNow) {
                mNowNs = nNow;
            }
            return;
        }
        Sch::TimedCommand *pCommand = *it;
        mQueue.erase(it);
        mNowNs = pCommand->mDueTick.mValue;
        try {
            pCommand->Run();
        } catch (std::exception &error) {
            ShowReportedMessage(HxStr(error.what()), kCommandErrorDuration);
        }
        // Yes, the binary clears the flag before it tests the wrapper for null.
        pCommand->mCommand->mQueued = 0;
        if (pCommand != nullptr) {
            pCommand->Release();
        }
    }
}

// 0x004aca30
void Watchdog::RestartClock() {
    mClock.Mark(0);
    mNowNs = 0;
}

// 0x004aca60
void Watchdog::Flush() {
    mClock.Mark(mNowNs);
    g_llWatchdogSecondNs = mClock.Now();
}

// 0x004ac950
void Watchdog::StartPlayback(IBStream &stream) {
    mPlayback = new WatchdogPlayback(this);
    mPlayback->Load(stream);
    mStreamMode = kStreamModePlayback;
    mPlayback->Start();
}
