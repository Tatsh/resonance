#include "game/playmapring.h"

#include <vector>

#include "script/configquery.h"

namespace {

// The configuration code for the number of turns GetExtent() scales one turn by.
constexpr int kRepeatCountConfigCode = 0x385;

} // namespace

// 0x0012e408
int PlayMapRing::MapBar(int nBar) {
    return (nBar + mBarCount) % mSteps.back();
}

// 0x0012dad8
std::vector<int> &PlayMapRing::FindBarsPlaying(int nStart, int nMin, int nEnd) {
    mFoundBars.clear();
    for (int nPosition = nStart; nPosition < nEnd; nPosition += mSteps.back()) {
        if (nPosition >= nMin) {
            mFoundBars.push_back(nPosition);
        }
    }
    return mFoundBars;
}

// 0x0012e430
int PlayMapRing::GetExtent() {
    return mSteps.back() * QueryConfigValue(kRepeatCountConfigCode);
}
