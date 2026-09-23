#include "game/playmap.h"

#include <algorithm>
#include <cstddef>
#include <vector>

#include "os/hxstr.h"
#include "os/mem.h"

// 0x00127118
void *PlayMap::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, "PlayMap");
}

// 0x00127138
void PlayMap::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, "PlayMap");
}

// 0x00127158
PlayMap::~PlayMap() {
}

// 0x001268b0
void PlayMap::Slot2(int nValue, HxStr strLabel) {
    mSectionLengths.push_back(nValue - mSteps.back()); // Unguarded on the first call.
    mSteps.push_back(nValue);
    mSectionNames.push_back(strLabel);
}

// 0x00127488
void PlayMap::Slot3(int nValue) {
    mUnknown00 = nValue;
}

// 0x00127398
void PlayMap::Slot4() {
}

// 0x001273b8
int PlayMap::Slot7(int nValue, [[maybe_unused]] int nSet) {
    return nValue;
}

// 0x001273c0
int PlayMap::Slot8() {
    return mSteps.back();
}

// 0x001273d0
int PlayMap::Slot9() {
    return Slot8(); // Dispatched through the table, not called directly.
}

// 0x00127440
int PlayMap::Slot10() {
    return static_cast<int>(mSteps.size()) - 1;
}

// 0x00127458
int PlayMap::Slot11(int nValue) {
    return nValue;
}

// 0x001276a0
int PlayMap::Slot12(int nValue) {
    const std::vector<int>::iterator it =
        std::upper_bound(mSteps.begin(), mSteps.end(), Slot5(nValue));
    return static_cast<int>(it - mSteps.begin()) - 1;
}

// 0x00127490
int PlayMap::FindStepIndex(int nPosition) {
    const std::vector<int>::iterator it = std::upper_bound(mSteps.begin(), mSteps.end(), nPosition);
    return static_cast<int>(it - mSteps.begin()) - 1;
}

// 0x001274d8
int PlayMap::IsStepStart(int nBar) {
    if (nBar < 0) {
        return 0;
    }
    const int nPosition = Slot5(nBar);
    return std::find(mSteps.begin(), mSteps.end(), nPosition) != mSteps.end();
}

// 0x00127548
int PlayMap::StepStartBar(int nBar) {
    const int nPosition = Slot5(nBar);
    const std::vector<int>::iterator it = std::upper_bound(mSteps.begin(), mSteps.end(), nPosition);
    return nBar - (nPosition - *(it - 1));
}

// 0x001275b0
int PlayMap::NextStepBar(int nBar) {
    if (nBar < 0) {
        return 0;
    }
    const int nPosition = Slot5(nBar);
    const std::vector<int>::iterator it = std::lower_bound(mSteps.begin(), mSteps.end(), nPosition);
    return nBar - (nPosition - *it);
}

// 0x00127628
int PlayMap::FollowingStepBar(int nBar) {
    if (nBar < 0) {
        return 0;
    }
    const int nPosition = Slot5(nBar);
    const std::vector<int>::iterator it = std::upper_bound(mSteps.begin(), mSteps.end(), nPosition);
    return nBar - (nPosition - *it);
}

// 0x00127700
int PlayMap::Slot13(int nValue) {
    const std::vector<int>::iterator it =
        std::upper_bound(mSteps.begin(), mSteps.end(), Slot5(nValue));
    const int nIndex = static_cast<int>(it - mSteps.begin()) - 1;
    const int nTotal = mSteps.back();
    // Scales the folded term by nTotal twice. That is what the binary computes.
    return (nTotal * (nValue - (nValue % nTotal))) + nIndex;
}

// 0x00127460
int PlayMap::Slot14(int) {
    return 0;
}

// 0x00127468
void PlayMap::Slot15(int) {
}

// 0x00127470
int PlayMap::Slot16(int) {
    return 0;
}

// 0x00127478
int PlayMap::Slot17(int) {
    return 0;
}

// 0x00127480
int PlayMap::Slot18(int) {
    return 0;
}
