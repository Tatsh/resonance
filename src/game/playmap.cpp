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
    mUnknown10.push_back(nValue - mSteps.back()); // Unguarded on the first call.
    mSteps.push_back(nValue);
    mUnknown1c.push_back(strLabel);
}

// 0x00127488
void PlayMap::Slot3(int nValue) {
    mUnknown00 = nValue;
}

// 0x00127398
void PlayMap::Slot4() {
}

// 0x001273b8
int PlayMap::Slot7(int nValue) {
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
int PlayMap::Slot14() {
    return 0;
}

// 0x00127468
void PlayMap::Slot15(int) {
}

// 0x00127470
int PlayMap::Slot16() {
    return 0;
}

// 0x00127478
int PlayMap::Slot17() {
    return 0;
}

// 0x00127480
int PlayMap::Slot18() {
    return 0;
}
