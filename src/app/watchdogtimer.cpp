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

} // namespace

long long WatchdogTimer::Now() {
    if (mHasOrigin == 0) {
        return mUnknown08;
    }
    return mWatchdog->mNowNs + mNegatedOrigin;
}

// 0x004a7878
void WatchdogTimer::Pause() {
    if (mHasOrigin != 0) {
        mHasOrigin = 0;
        mUnknown08 = mNegatedOrigin + mWatchdog->mNowNs;
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

// 0x004a79d0
void WatchdogTimer::Withdraw(const CmdID &id) {
    const CmdID copy = id; // Yes, the binary copies the handle to the stack first.
    mWatchdog->WithdrawByCmdID(copy);
}
