#include "sch/tickclock.h"

#include "app/attachment.h"
#include "app/scheduler.h"
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

// NTSC-U/C: 0x004a79f8, PAL: 0x004e5b08
TickClock::TickClock(Scheduler *pWatchdog, TempoMap *pTempoMap) : TimeClock(pWatchdog) {
    // The store of pTempoMap sits in the branch delay slot of the null test and therefore runs
    // whether the test passes or not. The private-map branch then overwrites it.
    if (pTempoMap != nullptr) {
        mTempoMap = pTempoMap;
        ++mTempoMap->mRefs;
    } else {
        mTempoMap = new TempoMap(kDefaultMicrosecondsPerQuarter);
    }
}

// NTSC-U/C: 0x004a7aa8, PAL: 0x004e5bb8
TickClock::~TickClock() {
    if (mTempoMap != nullptr) {
        mTempoMap->Release();
    }
}

// NTSC-U/C: 0x004a7af8, PAL: 0x004e5c08
int TickClock::SongTick() {
    const long long nTime = Now() + mTempoMap->mCeilingBias;
    return Sch::Tick(static_cast<int>(nTime / mTempoMap->mNanosecondsPerTick)).mTick;
}

// NTSC-U/C: 0x004a7b60, PAL: 0x004e5c70
void TickClock::SetSongTick(Sch::Tick tick) {
    const long long nTime = SongTickToTime(mTempoMap, tick.mTick);
    if (nTime != Now()) {
        mPausedNs = nTime; // Yes, the binary stores this even while the clock runs.
    }
}

// NTSC-U/C: 0x004a7be0, PAL: 0x004e5cf0
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

// NTSC-U/C: 0x004a5fd0, PAL: 0x004e4070
void TickClock::PostAt(Command *pCommand, Time tick) {
    CmdID id;
    id.mValue = kUnallocatedCommand;
    TimedCommand *pTimed = new TimedCommand(pCommand, tick, kAbsolutePost);
    mWatchdog->QueueAbsolute(
        pTimed, tick.mValue - mNegatedOrigin, id, kNotRecordable, kDefaultOrder);
    Attachment::ReleaseIfSet(pTimed);
}

// NTSC-U/C: 0x004a6248, PAL: 0x004e42e8
void TickClock::PostAtSongTick(Command *pCommand,
                               long long nTick,
                               CmdID &id,
                               [[maybe_unused]] int nUnused) {
    const long long nTime = SongTickToTime(mTempoMap, nTick);
    TimedCommand *pTimed = new TimedCommand(pCommand, Time{nTime}, kAbsolutePost);
    mWatchdog->QueueAbsolute(pTimed, nTime - mNegatedOrigin, id, kNotRecordable, kDefaultOrder);
    Attachment::ReleaseIfSet(pTimed);
}

// NTSC-U/C: 0x004a6330, PAL: 0x004e43d0
void TickClock::PostAtSongTick(Command *pCommand, long long nTick) {
    CmdID id;
    id.mValue = kUnallocatedCommand;
    const long long nTime = SongTickToTime(mTempoMap, nTick);
    TimedCommand *pTimed = new TimedCommand(pCommand, Time{nTime}, kAbsolutePost);
    mWatchdog->QueueAbsolute(pTimed, nTime - mNegatedOrigin, id, kNotRecordable, kDefaultOrder);
    Attachment::ReleaseIfSet(pTimed);
}

} // namespace Sch
