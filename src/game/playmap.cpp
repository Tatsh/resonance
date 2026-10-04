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

void *PlayMap::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, "PlayMap");
}

void PlayMap::operator delete(void *pBlock) {
    OperatorDeleteOverride(pBlock, "PlayMap");
}

PlayMap::PlayMap() : mBarCount(0), mUnusedValue(0) {
    mSteps.reserve(kInitialCapacity);
    mSteps.push_back(0);
    mSectionLengths.reserve(kInitialCapacity);
}

void PlayMap::AddStep(int nPosition, HxStr strLabel) {
    mSectionLengths.push_back(nPosition - mSteps.back()); // Unguarded on the first call.
    mSteps.push_back(nPosition);
    mSectionNames.push_back(strLabel);
}

void PlayMap::SetBarCount(int nBarCount) {
    mBarCount = nBarCount;
}

void PlayMap::ResetSpans() {
}

int PlayMap::MapToLinkedStep(int nPosition, [[maybe_unused]] int nSet) {
    return nPosition;
}

int PlayMap::GetLength() const {
    return mSteps.back();
}

int PlayMap::GetEndBar() {
    return GetLength(); // Dispatched through the table, not called directly.
}

int PlayMap::GetNumSections() const {
    return static_cast<int>(mSteps.size()) - 1;
}

int PlayMap::GetPatternSection(int nIndex) {
    return nIndex;
}

int PlayMap::GetPatternIndex(int nBar) {
    const std::vector<int>::iterator it =
        std::upper_bound(mSteps.begin(), mSteps.end(), MapBar(nBar));
    return static_cast<int>(it - mSteps.begin()) - 1;
}

int PlayMap::FindStepIndex(int nPosition) {
    const std::vector<int>::iterator it = std::upper_bound(mSteps.begin(), mSteps.end(), nPosition);
    return static_cast<int>(it - mSteps.begin()) - 1;
}

int PlayMap::IsStepStart(int nBar) {
    if (nBar < 0) {
        return 0;
    }
    const int nPosition = MapBar(nBar);
    return std::find(mSteps.begin(), mSteps.end(), nPosition) != mSteps.end();
}

int PlayMap::StepStartBar(int nBar) {
    const int nPosition = MapBar(nBar);
    const std::vector<int>::iterator it = std::upper_bound(mSteps.begin(), mSteps.end(), nPosition);
    return nBar - (nPosition - *(it - 1));
}

int PlayMap::NextStepBar(int nBar) {
    if (nBar < 0) {
        return 0;
    }
    const int nPosition = MapBar(nBar);
    const std::vector<int>::iterator it = std::lower_bound(mSteps.begin(), mSteps.end(), nPosition);
    return nBar - (nPosition - *it);
}

int PlayMap::FollowingStepBar(int nBar) {
    if (nBar < 0) {
        return 0;
    }
    const int nPosition = MapBar(nBar);
    const std::vector<int>::iterator it = std::upper_bound(mSteps.begin(), mSteps.end(), nPosition);
    return nBar - (nPosition - *it);
}

int PlayMap::GetAbsoluteSectionIndex(int nBar) {
    const std::vector<int>::iterator it =
        std::upper_bound(mSteps.begin(), mSteps.end(), MapBar(nBar));
    const int nIndex = static_cast<int>(it - mSteps.begin()) - 1;
    const int nTotal = mSteps.back();
    // Scales the folded term by nTotal twice. That is what the binary computes.
    return (nTotal * (nBar - (nBar % nTotal))) + nIndex;
}

int PlayMap::IsLooping(int) {
    return 0;
}

void PlayMap::CloseSpan(int) {
}

int PlayMap::EndLoop(int) {
    return 0;
}

int PlayMap::StartLoop(int) {
    return 0;
}

int PlayMap::ToggleLoop(int) {
    return 0;
}
