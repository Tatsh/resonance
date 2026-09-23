#include "game/axephrasemaker.h"

#include "mid/mbt.h"

namespace {

// The origin every guitar phrase maker reports, in MIDI ticks.
constexpr int kPeriodOrigin = 6;

} // namespace

// 0x0019d438
int AxePhraseMaker::Slot5() {
    return Mid::MBT(kPeriodOrigin).mTick;
}
