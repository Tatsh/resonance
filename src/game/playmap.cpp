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

// NTSC-U/C: 0x00127118, PAL: 0x00127820
void *PlayMap::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, "PlayMap");
}

// NTSC-U/C: 0x00127138, PAL: 0x00127840
void PlayMap::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, "PlayMap");
}

// NTSC-U/C: 0x001263c0, PAL: 0x00126a70
PlayMap::PlayMap() : mBarCount(0), mUnusedValue(0) {
    mSteps.reserve(kInitialCapacity);
    mSteps.push_back(0);
    mSectionLengths.reserve(kInitialCapacity);
}

// NTSC-U/C: 0x001268b0, PAL: 0x00126f70
void PlayMap::AddStep(int nPosition, HxStr strLabel) {
    mSectionLengths.push_back(nPosition - mSteps.back()); // Unguarded on the first call.
    mSteps.push_back(nPosition);
    mSectionNames.push_back(strLabel);
}

// NTSC-U/C: 0x00127488, PAL: 0x00127ba8
void PlayMap::SetBarCount(int nBarCount) {
    mBarCount = nBarCount;
}

// NTSC-U/C: 0x00127398, PAL: 0x00127ab8
void PlayMap::ResetSpans() {
}

// NTSC-U/C: 0x001273b8, PAL: 0x00127ad8
int PlayMap::MapToLinkedStep(int nPosition, [[maybe_unused]] int nSet) {
    return nPosition;
}

// NTSC-U/C: 0x001273c0, PAL: 0x00127ae0
int PlayMap::GetExtent() {
    return mSteps.back();
}

// NTSC-U/C: 0x001273d0, PAL: 0x00127af0
int PlayMap::GetEndBar() {
    return GetExtent(); // Dispatched through the table, not called directly.
}

// NTSC-U/C: 0x00127440, PAL: 0x00127b60
int PlayMap::GetSectionCount() {
    return static_cast<int>(mSteps.size()) - 1;
}

// NTSC-U/C: 0x00127458, PAL: 0x00127b78
int PlayMap::GetPatternSection(int nIndex) {
    return nIndex;
}

// NTSC-U/C: 0x001276a0, PAL: 0x00127dc0
int PlayMap::GetPatternIndex(int nBar) {
    const std::vector<int>::iterator it =
        std::upper_bound(mSteps.begin(), mSteps.end(), MapBar(nBar));
    return static_cast<int>(it - mSteps.begin()) - 1;
}

// NTSC-U/C: 0x00127490, PAL: 0x00127bb0
int PlayMap::FindStepIndex(int nPosition) {
    const std::vector<int>::iterator it = std::upper_bound(mSteps.begin(), mSteps.end(), nPosition);
    return static_cast<int>(it - mSteps.begin()) - 1;
}

// NTSC-U/C: 0x001274d8, PAL: 0x00127bf8
int PlayMap::IsStepStart(int nBar) {
    if (nBar < 0) {
        return 0;
    }
    const int nPosition = MapBar(nBar);
    return std::find(mSteps.begin(), mSteps.end(), nPosition) != mSteps.end();
}

// NTSC-U/C: 0x00127548, PAL: 0x00127c68
int PlayMap::StepStartBar(int nBar) {
    const int nPosition = MapBar(nBar);
    const std::vector<int>::iterator it = std::upper_bound(mSteps.begin(), mSteps.end(), nPosition);
    return nBar - (nPosition - *(it - 1));
}

// NTSC-U/C: 0x001275b0, PAL: 0x00127cd0
int PlayMap::NextStepBar(int nBar) {
    if (nBar < 0) {
        return 0;
    }
    const int nPosition = MapBar(nBar);
    const std::vector<int>::iterator it = std::lower_bound(mSteps.begin(), mSteps.end(), nPosition);
    return nBar - (nPosition - *it);
}

// NTSC-U/C: 0x00127628, PAL: 0x00127d48
int PlayMap::FollowingStepBar(int nBar) {
    if (nBar < 0) {
        return 0;
    }
    const int nPosition = MapBar(nBar);
    const std::vector<int>::iterator it = std::upper_bound(mSteps.begin(), mSteps.end(), nPosition);
    return nBar - (nPosition - *it);
}

// NTSC-U/C: 0x00127700, PAL: 0x00127e20
int PlayMap::GetAbsoluteSectionIndex(int nBar) {
    const std::vector<int>::iterator it =
        std::upper_bound(mSteps.begin(), mSteps.end(), MapBar(nBar));
    const int nIndex = static_cast<int>(it - mSteps.begin()) - 1;
    const int nTotal = mSteps.back();
    // Scales the folded term by nTotal twice. That is what the binary computes.
    return (nTotal * (nBar - (nBar % nTotal))) + nIndex;
}

// NTSC-U/C: 0x00127460, PAL: 0x00127b80
int PlayMap::IsLooping(int) {
    return 0;
}

// NTSC-U/C: 0x00127468, PAL: 0x00127b88
void PlayMap::CloseSpan(int) {
}

// NTSC-U/C: 0x00127470, PAL: 0x00127b90
int PlayMap::EndLoop(int) {
    return 0;
}

// NTSC-U/C: 0x00127478, PAL: 0x00127b98
int PlayMap::StartLoop(int) {
    return 0;
}

// NTSC-U/C: 0x00127480, PAL: 0x00127ba0
int PlayMap::ToggleLoop(int) {
    return 0;
}
