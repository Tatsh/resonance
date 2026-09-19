#include "sch/tickclock.h"

#include "sch/tempomap.h"

namespace Sch {

namespace {

// The tempo a Standard MIDI File assumes when it declares none, which is 120 quarter notes per
// minute.
constexpr int kDefaultMicrosecondsPerQuarter = 500000;

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

} // namespace Sch
