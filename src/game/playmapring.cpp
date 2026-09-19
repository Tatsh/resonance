#include "game/playmapring.h"

#include <vector>

// 0x0012e408
int PlayMapRing::Slot5(int nValue) {
    return (nValue + mUnknown00) % mSteps.back();
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
