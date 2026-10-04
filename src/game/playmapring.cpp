#include "game/playmapring.h"

#include <vector>

#include "script/configquery.h"

namespace {

// The configuration code for the number of turns GetLength() scales one turn by.
constexpr int kRepeatCountConfigCode = 0x385;

} // namespace

int PlayMapRing::MapBar(int nBar) {
    return (nBar + mBarCount) % mSteps.back();
}

std::vector<int> &PlayMapRing::FindBarsPlaying(int nStart, int nMin, int nEnd) {
    mFoundBars.clear();
    for (int nPosition = nStart; nPosition < nEnd; nPosition += mSteps.back()) {
        if (nPosition >= nMin) {
            mFoundBars.push_back(nPosition);
        }
    }
    return mFoundBars;
}

int PlayMapRing::GetLength() const {
    return mSteps.back() * QueryConfigValue(kRepeatCountConfigCode);
}
