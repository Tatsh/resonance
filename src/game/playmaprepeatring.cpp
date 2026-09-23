#include "game/playmaprepeatring.h"

#include <algorithm>
#include <vector>

namespace {
// Terminates the position run. The value is the binary's literal and its meaning is unrecovered.
constexpr int kSpanTerminator = 10000000;

// Marks a position that slot 3 has reserved but not yet resolved.
constexpr int kUnresolvedPosition = -1;
} // namespace

// 0x0012ba88
void PlayMapRepeatRing::Slot4() {
    mUnknown3c.clear();
    mUnknown3c.push_back(0);
    mUnknown3c.push_back(kSpanTerminator);
}

// 0x0012bb60
void PlayMapRepeatRing::Slot3(int nValue) {
    PlayMap::Slot3(nValue);
    Slot4(); // Dispatched through the table, so a further subclass would run instead.
    const int nPrefix =
        static_cast<int>(std::find(mSteps.begin(), mSteps.end(), nValue) - mSteps.begin());
    for (int nIndex = 0; nIndex < nPrefix; ++nIndex) {
        mUnknown3c.push_back(kUnresolvedPosition);
    }
}

// 0x0012bc48
void PlayMapRepeatRing::Slot15(int nValue) {
    // The binary takes the remainder with an unsigned divide, so the sizes stay unsigned here
    // rather than being narrowed to int.
    const std::vector<int>::size_type nLast = mUnknown3c.size() - 1;
    const std::vector<int>::size_type nPrevious = nLast - 1;
    const int nGap = mSectionLengths[nPrevious % mSectionLengths.size()];
    mUnknown3c[nLast] = mUnknown3c[nPrevious] + (nValue * nGap);
    mUnknown3c.push_back(kSpanTerminator);
}
