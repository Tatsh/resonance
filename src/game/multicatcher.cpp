#include "game/multicatcher.h"

namespace {

// The value MultiCatcher supplies for Catcher's int parameter.
constexpr int kMultiCatcherFlag = 1;

} // namespace

// 0x001b1a60
MultiCatcher::MultiCatcher(PhraseMgr *pPhraseMgr,
                           Quantizer *pQuantizer,
                           const TrackData *pTrackData,
                           Sch::TickClock *pClock,
                           Sch::Tick tick)
    : Catcher(pPhraseMgr, pQuantizer, pTrackData, pClock, kMultiCatcherFlag, tick) {
}

// 0x001b0d98
MultiCatcher::~MultiCatcher() {
}

// 0x001b0e00
void MultiCatcher::Slot10(int) {
}
