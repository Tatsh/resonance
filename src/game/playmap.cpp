#include "game/playmap.h"

#include <algorithm>
#include <cstddef>
#include <vector>

#include "os/hxstr.h"
#include "os/mem.h"

namespace {

// The capacity the constructor reserves in mSteps and mSectionLengths.
constexpr std::vector<int>::size_type kInitialCapacity = 8;

} // namespace

// 0x00127118
void *PlayMap::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, "PlayMap");
}

// 0x00127138
void PlayMap::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, "PlayMap");
}

// 0x001263c0
PlayMap::PlayMap() : mBarCount(0), mUnusedValue(0) {
    mSteps.reserve(kInitialCapacity);
    mSteps.push_back(0);
    mSectionLengths.reserve(kInitialCapacity);
}

// 0x001268b0
void PlayMap::AddStep(int nPosition, HxStr strLabel) {
    mSectionLengths.push_back(nPosition - mSteps.back()); // Unguarded on the first call.
    mSteps.push_back(nPosition);
    mSectionNames.push_back(strLabel);
}

// 0x00127488
void PlayMap::SetBarCount(int nBarCount) {
    mBarCount = nBarCount;
}

// 0x00127398
void PlayMap::ResetSpans() {
}

// 0x001273b8
int PlayMap::MapToLinkedStep(int nPosition, [[maybe_unused]] int nSet) {
    return nPosition;
}

// 0x001273c0
int PlayMap::GetExtent() {
    return mSteps.back();
}

// 0x001273d0
int PlayMap::GetEndBar() {
    return GetExtent(); // Dispatched through the table, not called directly.
}

// 0x00127440
int PlayMap::GetSectionCount() {
    return static_cast<int>(mSteps.size()) - 1;
}

// 0x00127458
int PlayMap::GetPatternSection(int nIndex) {
    return nIndex;
}

// 0x001276a0
int PlayMap::GetPatternIndex(int nBar) {
    const std::vector<int>::iterator it =
        std::upper_bound(mSteps.begin(), mSteps.end(), MapBar(nBar));
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
    const int nPosition = MapBar(nBar);
    return std::find(mSteps.begin(), mSteps.end(), nPosition) != mSteps.end();
}

// 0x00127548
int PlayMap::StepStartBar(int nBar) {
    const int nPosition = MapBar(nBar);
    const std::vector<int>::iterator it = std::upper_bound(mSteps.begin(), mSteps.end(), nPosition);
    return nBar - (nPosition - *(it - 1));
}

// 0x001275b0
int PlayMap::NextStepBar(int nBar) {
    if (nBar < 0) {
        return 0;
    }
    const int nPosition = MapBar(nBar);
    const std::vector<int>::iterator it = std::lower_bound(mSteps.begin(), mSteps.end(), nPosition);
    return nBar - (nPosition - *it);
}

// 0x00127628
int PlayMap::FollowingStepBar(int nBar) {
    if (nBar < 0) {
        return 0;
    }
    const int nPosition = MapBar(nBar);
    const std::vector<int>::iterator it = std::upper_bound(mSteps.begin(), mSteps.end(), nPosition);
    return nBar - (nPosition - *it);
}

// 0x00127700
int PlayMap::GetAbsoluteSectionIndex(int nBar) {
    const std::vector<int>::iterator it =
        std::upper_bound(mSteps.begin(), mSteps.end(), MapBar(nBar));
    const int nIndex = static_cast<int>(it - mSteps.begin()) - 1;
    const int nTotal = mSteps.back();
    // Scales the folded term by nTotal twice. That is what the binary computes.
    return (nTotal * (nBar - (nBar % nTotal))) + nIndex;
}

// 0x00127460
int PlayMap::IsLooping(int) {
    return 0;
}

// 0x00127468
void PlayMap::CloseSpan(int) {
}

// 0x00127470
int PlayMap::EndLoop(int) {
    return 0;
}

// 0x00127478
int PlayMap::StartLoop(int) {
    return 0;
}

// 0x00127480
int PlayMap::ToggleLoop(int) {
    return 0;
}
