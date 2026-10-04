#include "app/timeclock.h"

#include "app/attachment.h"
#include "app/scheduler.h"
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

Sch::TimeClock::TimeClock(Scheduler *pWatchdog)
    : mNegatedOrigin(0), mPausedNs(0), mHasOrigin(0), mWatchdog(pWatchdog) {
}

void Sch::TimeClock::SetOrigin(long long nNanoseconds) {
    if (mHasOrigin == 0) {
        mHasOrigin = 1;
        mNegatedOrigin = -nNanoseconds;
    }
}

long long Sch::TimeClock::Now() {
    if (mHasOrigin == 0) {
        return mPausedNs;
    }
    return mWatchdog->mNowNs + mNegatedOrigin;
}

void Sch::TimeClock::Pause() {
    if (mHasOrigin != 0) {
        mHasOrigin = 0;
        mPausedNs = mNegatedOrigin + mWatchdog->mNowNs;
    }
}

void Sch::TimeClock::Resume() {
    if (mHasOrigin == 0) {
        mHasOrigin = 1;
        mNegatedOrigin = mPausedNs - mWatchdog->mNowNs;
    }
}

void Sch::TimeClock::Post(
    Sch::Command *pCommand, Sch::Time tick, CmdID &id, int bRecordable, int bDelta) {
    Sch::TimedCommand *pTimed = new Sch::TimedCommand(pCommand, tick, bDelta);
    if (bDelta != 0) {
        mWatchdog->QueueDelta(pTimed, tick.mValue, id, bRecordable, kDefaultOrder);
    } else {
        mWatchdog->QueueAbsolute(
            pTimed, tick.mValue - mNegatedOrigin, id, kAbsoluteNotRecordable, kDefaultOrder);
    }
    Attachment::ReleaseIfSet(pTimed);
}

void Sch::TimeClock::PostIn(Sch::Command *pCommand, Sch::Time tick, CmdID &id, int bRecordable) {
    Sch::TimedCommand *pTimed = new Sch::TimedCommand(pCommand, tick, kDeltaPost);
    mWatchdog->QueueDelta(pTimed, tick.mValue, id, bRecordable, kDefaultOrder);
    Attachment::ReleaseIfSet(pTimed);
}

void Sch::TimeClock::PostIn(Sch::Command *pCommand, Sch::Time tick) {
    CmdID id;
    id.mValue = kUnallocatedCommand;
    Sch::TimedCommand *pTimed = new Sch::TimedCommand(pCommand, tick, kDeltaPost);
    mWatchdog->QueueDelta(pTimed, tick.mValue, id, kNotRecordable, kDefaultOrder);
    Attachment::ReleaseIfSet(pTimed);
}

void Sch::TimeClock::Withdraw(const CmdID &id) {
    const CmdID copy = id; // Yes, the binary copies the handle to the stack first.
    mWatchdog->WithdrawByCmdID(copy);
}
