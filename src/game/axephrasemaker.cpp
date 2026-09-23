#include "game/axephrasemaker.h"

#include "mid/mbt.h"

namespace {

// The origin every guitar phrase maker reports, in MIDI ticks.
constexpr int kPeriodOrigin = 6;

} // namespace

// 0x0019d438
int AxePhraseMaker::Slot5() {
    (void)IsFiniteMBT(kPeriodOrigin); // Yes, the binary discards this call's result.
    return kPeriodOrigin;
}
