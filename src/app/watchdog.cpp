#include "app/watchdog.h"

#include <algorithm>
#include <exception>

#include "app/attachment.h"
#include "app/watchdogplayback.h"
#include "app/watchdogrecorder.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "sch/command.h"

namespace {

// The value of mStreamMode while a recording plays back.
constexpr int kStreamModePlayback = 2;

// The value of mStreamMode while commands are recorded.
constexpr int kStreamModeRecording = 1;

// The handle value a caller supplies to have one allocated.
constexpr int kUnallocatedCmdId = -2;

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
Watchdog::Watchdog()
    : mStreamMode(0), mRecorder(nullptr), mPlayback(nullptr), mNowNs(0), mBlocked(0) {
}

// 0x004a9980
Watchdog::~Watchdog() {
    mBlocked = 1;
    std::for_each(mQueue.begin(), mQueue.end(), Attachment::ReleaseIfSet);
    mQueue.clear();
    delete mRecorder;
    mRecorder = nullptr;
    if (mPlayback != nullptr) {
        delete mPlayback;
    }
    mPlayback = nullptr;
}

// 0x004ac8c0
void Watchdog::BeginRecording(OBStream &stream) {
    mRecorder = new WatchdogRecorder(&stream);
    mStreamMode = kStreamModeRecording;
    mClock.Mark(0);
    mNowNs = 0;
}

// 0x004ac9e8
void Watchdog::Close() {
    delete mRecorder;
    mRecorder = nullptr;
    if (mPlayback != nullptr) {
        delete mPlayback;
    }
    mStreamMode = 0;
    mPlayback = nullptr;
}

inline void Watchdog::Enqueue(Sch::TimedCommand *pCommand, CmdID &id) {
    if (id.mValue == kUnallocatedCmdId) {
        id.mValue = CmdID::AllocateValue();
    }
    // Yes, the binary writes the handle before it tests the wrapper for null.
    pCommand->mCmdID = id;
    if (pCommand != nullptr) {
        ++pCommand->mRefs;
    }
    mQueue.insert(pCommand);
}

// 0x004ac608
void Watchdog::QueueAbsolute(Sch::TimedCommand *pCommand,
                             long long nTick,
                             CmdID &id,
                             [[maybe_unused]] int bRecordable,
                             int nOrder) {
    if (mBlocked != 0) {
        return;
    }
    pCommand->mOrder = nOrder;
    pCommand->mDueTick.mValue = nTick;
    Enqueue(pCommand, id);
}

// 0x004ac698
void Watchdog::QueueDelta(
    Sch::TimedCommand *pCommand, long long nDelta, CmdID &id, int bRecordable, int nOrder) {
    if (mStreamMode == kStreamModePlayback && bRecordable != 0) {
        return;
    }
    const long long nNow = mClock.Now();
    const long long nBase = (mNowNs < nNow) ? nNow : mNowNs;
    if (mBlocked != 0) {
        return;
    }
    pCommand->mOrder = nOrder;
    pCommand->mDueTick.mValue = nBase + nDelta;
    Enqueue(pCommand, id);
    pCommand->mCommand->mQueued = 1;
    if (mStreamMode == kStreamModeRecording && bRecordable != 0) {
        mRecorder->Record(pCommand);
    }
}

// 0x004ac808
void Watchdog::PostUnreferenced(Sch::Command *pCommand) {
    if (mStreamMode != kStreamModeRecording) {
        return;
    }
    const long long nNow = mClock.Now();
    const long long nDue = (mNowNs < nNow) ? nNow : mNowNs;
    Sch::TimedCommand wrapper(pCommand, Sch::Tick{0}, 0);
    wrapper.mDueTick.mValue = nDue;
    CmdID id;
    id.mValue = CmdID::AllocateValue();
    wrapper.mCmdID = id;
    // Yes, the binary destroys the wrapper without queueing or recording it.
}

// 0x004a9a78
void Watchdog::Snapshot() {
    const std::multiset<Sch::TimedCommand *, QueueOrder> queue(mQueue);
    mQueue.clear();
    std::for_each(queue.begin(), queue.end(), Attachment::ReleaseIfSet);
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
