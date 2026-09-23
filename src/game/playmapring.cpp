#include "game/playmapring.h"

#include <vector>

#include "script/configquery.h"

namespace {

// The configuration code for the number of turns Slot8() scales one turn by.
constexpr int kRepeatCountConfigCode = 0x385;

} // namespace

// 0x0012e408
int PlayMapRing::Slot5(int nValue) {
    return (nValue + mBarCount) % mSteps.back();
}

// 0x0012dad8
std::vector<int> &PlayMapRing::Slot6(int nStart, int nMin, int nEnd) {
    mUnknown2c.clear();
    for (int nPosition = nStart; nPosition < nEnd; nPosition += mSteps.back()) {
        if (nPosition >= nMin) {
            mUnknown2c.push_back(nPosition);
        }
    }
    return mUnknown2c;
}

// 0x0012e430
int PlayMapRing::Slot8() {
    return mSteps.back() * QueryConfigValue(kRepeatCountConfigCode);
}
