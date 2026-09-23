#include "sch/tickclock.h"

#include "app/attachment.h"
#include "app/watchdog.h"
#include "sch/cmdid.h"
#include "sch/tempomap.h"
#include "sch/timedcommand.h"

namespace Sch {

namespace {

// The tempo a Standard MIDI File assumes when it declares none, which is 120 quarter notes per
// minute.
constexpr int kDefaultMicrosecondsPerQuarter = 500000;

// The flags and the order every post below gives the scheduler's absolute queueing path.
constexpr int kAbsolutePost = 0;
constexpr int kNotRecordable = 0;
constexpr int kDefaultOrder = -1;

// The handle value that asks the queue to allocate one.
constexpr int kUnallocatedCommand = -2;

// Converts a song position to scheduler time through the clock's tempo map.
inline long long SongTickToTime(const TempoMap *pTempoMap, long long nTick) {
    return nTick * pTempoMap->mNanosecondsPerTick + pTempoMap->mOriginNanoseconds;
}

} // namespace

// 0x004a79f8
TickClock::TickClock(Watchdog *pWatchdog, TempoMap *pTempoMap) : WatchdogTimer(pWatchdog) {
    // The store of pTempoMap sits in the branch delay slot of the null test and therefore runs
    // whether the test passes or not. The private-map branch then overwrites it.
    if (pTempoMap != nullptr) {
        mTempoMap = pTempoMap;
        ++mTempoMap->mRefs;
    } else {
        mTempoMap = new TempoMap(kDefaultMicrosecondsPerQuarter);
    }
}

// 0x004a7aa8
TickClock::~TickClock() {
    if (mTempoMap != nullptr) {
        mTempoMap->Release();
    }
}

// 0x004a7af8
int TickClock::SongTick() {
    const long long nTime = Now() + mTempoMap->mCeilingBias;
    return Mid::MBT(static_cast<int>(nTime / mTempoMap->mNanosecondsPerTick)).mTick;
}

// 0x004a7b60
void TickClock::SetSongTick(Mid::MBT tick) {
    const long long nTime = SongTickToTime(mTempoMap, tick.mTick);
    if (nTime != Now()) {
        mPausedNs = nTime; // Yes, the binary stores this even while the clock runs.
    }
}

// 0x004a7be0
void TickClock::SetTempoMap(TempoMap *pTempoMap) {
    TempoMap *pPrevious = mTempoMap;
    mTempoMap = pTempoMap;
    if (pTempoMap != nullptr) {
        ++pTempoMap->mRefs;
    }
    if (pPrevious != nullptr) {
        pPrevious->Release();
    }
}

// 0x004a5fd0
void TickClock::PostAt(Command *pCommand, Tick tick) {
    CmdID id;
    id.mValue = kUnallocatedCommand;
    TimedCommand *pTimed = new TimedCommand(pCommand, tick, kAbsolutePost);
    mWatchdog->QueueAbsolute(
        pTimed, tick.mValue - mNegatedOrigin, id, kNotRecordable, kDefaultOrder);
    Attachment::ReleaseIfSet(pTimed);
}

// 0x004a6248
void TickClock::PostAtSongTick(Command *pCommand,
                               long long nTick,
                               CmdID &id,
                               [[maybe_unused]] int nUnused) {
    const long long nTime = SongTickToTime(mTempoMap, nTick);
    TimedCommand *pTimed = new TimedCommand(pCommand, Tick{nTime}, kAbsolutePost);
    mWatchdog->QueueAbsolute(pTimed, nTime - mNegatedOrigin, id, kNotRecordable, kDefaultOrder);
    Attachment::ReleaseIfSet(pTimed);
}

// 0x004a6330
void TickClock::PostAtSongTick(Command *pCommand, long long nTick) {
    CmdID id;
    id.mValue = kUnallocatedCommand;
    const long long nTime = SongTickToTime(mTempoMap, nTick);
    TimedCommand *pTimed = new TimedCommand(pCommand, Tick{nTime}, kAbsolutePost);
    mWatchdog->QueueAbsolute(pTimed, nTime - mNegatedOrigin, id, kNotRecordable, kDefaultOrder);
    Attachment::ReleaseIfSet(pTimed);
}

} // namespace Sch
