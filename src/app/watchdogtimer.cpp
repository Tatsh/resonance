#include "app/watchdogtimer.h"

#include "app/attachment.h"
#include "app/watchdog.h"
#include "sch/cmdid.h"
#include "sch/timedcommand.h"

namespace {

// The order Post() gives every wrapper among those due at the same time.
constexpr int kDefaultOrder = -1;

// The recordable flag the absolute path passes in place of the caller's.
constexpr int kAbsoluteNotRecordable = 0;

// The delta flag both PostIn() overloads give the wrapper, and the recordable flag of the
// unkeyed one.
constexpr int kDeltaPost = 1;
constexpr int kNotRecordable = 0;

// The handle value that asks the queue to allocate one.
constexpr int kUnallocatedCommand = -2;

} // namespace

// 0x004a7780
WatchdogTimer::WatchdogTimer(Watchdog *pWatchdog)
    : mNegatedOrigin(0), mPausedNs(0), mHasOrigin(0), mWatchdog(pWatchdog) {
}

// 0x004a7828
void WatchdogTimer::SetOrigin(long long nNanoseconds) {
    if (mHasOrigin == 0) {
        mHasOrigin = 1;
        mNegatedOrigin = -nNanoseconds;
    }
}

// 0x004a77c0
long long WatchdogTimer::Now() {
    if (mHasOrigin == 0) {
        return mPausedNs;
    }
    return mWatchdog->mNowNs + mNegatedOrigin;
}

// 0x004a7878
void WatchdogTimer::Pause() {
    if (mHasOrigin != 0) {
        mHasOrigin = 0;
        mPausedNs = mNegatedOrigin + mWatchdog->mNowNs;
    }
}

// 0x004a7848
void WatchdogTimer::Resume() {
    if (mHasOrigin == 0) {
        mHasOrigin = 1;
        mNegatedOrigin = mPausedNs - mWatchdog->mNowNs;
    }
}

// 0x004a78b8
void WatchdogTimer::Post(
    Sch::Command *pCommand, Sch::Tick tick, CmdID &id, int bRecordable, int bDelta) {
    Sch::TimedCommand *pTimed = new Sch::TimedCommand(pCommand, tick, bDelta);
    if (bDelta != 0) {
        mWatchdog->QueueDelta(pTimed, tick.mValue, id, bRecordable, kDefaultOrder);
    } else {
        mWatchdog->QueueAbsolute(
            pTimed, tick.mValue - mNegatedOrigin, id, kAbsoluteNotRecordable, kDefaultOrder);
    }
    Attachment::ReleaseIfSet(pTimed);
}

// 0x004a60a0
void WatchdogTimer::PostIn(Sch::Command *pCommand, Sch::Tick tick, CmdID &id, int bRecordable) {
    Sch::TimedCommand *pTimed = new Sch::TimedCommand(pCommand, tick, kDeltaPost);
    mWatchdog->QueueDelta(pTimed, tick.mValue, id, bRecordable, kDefaultOrder);
    Attachment::ReleaseIfSet(pTimed);
}

// 0x004a6178
void WatchdogTimer::PostIn(Sch::Command *pCommand, Sch::Tick tick) {
    CmdID id;
    id.mValue = kUnallocatedCommand;
    Sch::TimedCommand *pTimed = new Sch::TimedCommand(pCommand, tick, kDeltaPost);
    mWatchdog->QueueDelta(pTimed, tick.mValue, id, kNotRecordable, kDefaultOrder);
    Attachment::ReleaseIfSet(pTimed);
}

// 0x004a79d0
void WatchdogTimer::Withdraw(const CmdID &id) {
    const CmdID copy = id; // Yes, the binary copies the handle to the stack first.
    mWatchdog->WithdrawByCmdID(copy);
}
