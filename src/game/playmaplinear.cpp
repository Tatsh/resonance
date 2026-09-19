#include "game/playmaplinear.h"

#include <vector>

// 0x00129150
void PlayMapLinear::GrowPastLimit(int nLimit) {
    while (!(nLimit < Slot8())) {
        // The extent is re-read on every iteration rather than cached, and slot 8 is called once
        // per element rather than once per pass.
        for (std::vector<Entry>::size_type nIndex = 0; nIndex < mUnknown58.size(); ++nIndex) {
            mUnknown48.push_back(Slot8());
            const Entry entry{mUnknown58[nIndex].mValue, 1};
            mUnknown3c.push_back(entry);
        }
    }

    const std::vector<Entry>::size_type nPassed = mUnknown58.size();
    // The guard compares a byte offset against an element count. That mismatch is what the binary
    // computes, so the trim only fires once the window is more than eight times the source.
    if ((nPassed * sizeof(Entry)) < mUnknown3c.size()) {
        mUnknown3c.erase(mUnknown3c.begin(), mUnknown3c.begin() + nPassed);
        mUnknown48.erase(mUnknown48.begin(), mUnknown48.begin() + nPassed);
        mUnknown54 += static_cast<int>(nPassed);
    }
}
