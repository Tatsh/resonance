#include "game/playmaplinear.h"

#include <algorithm>
#include <vector>

#include "os/hxstr.h"
#include "script/testregistry.h"

namespace {

// The capacity the constructor reserves in the window vectors.
constexpr std::vector<int>::size_type kInitialCapacity = 8;

// The name SelfTest() registers under, and the label it gives every step.
static const char *const kTestName = "PlayMapLinear";
static const char *const kTestLabel = "";
// The steps, sections, and probe positions SelfTest() uses.
const int kTestSteps[] = {1, 3, 6, 10};
const int kTestSections[] = {3, 2, 1, 0, 3, 2, 2};
const int kTestProbes[] = {0, 2, 4, 8, 9};
const int kTestFirstToggle = 2;
const int kTestSecondToggle = 8;
const int kTestLateProbe = 13;
// SelfTest() constructs its map without the script tables.
constexpr int kSkipStepRings = 0;

// Registers the self-test from the unit's static initialiser.
class SelfTestRegistration {
public:
    SelfTestRegistration() {
        TestRegistry::Register(kTestName, PlayMapLinear::RunSelfTest);
    }
};
const SelfTestRegistration sSelfTestRegistration;

} // namespace

PlayMapLinear::PlayMapLinear(int bLoadStepRings)
    : mTrimmedCount(0), mPatternEnd(0), mStepRings(kSetCount) {
    mWindow.reserve(kInitialCapacity);
    mWindowStarts.reserve(kInitialCapacity);
    if (bLoadStepRings != 0) {
        LoadStepRings();
    }
}

void PlayMapLinear::AppendSection(int nSection) {
    mWindowStarts.push_back(GetLength());
    const Entry entry{nSection, 1};
    mWindow.push_back(entry);
}

void PlayMapLinear::GrowPastLimit(int nLimit) {
    while (!(nLimit < GetLength())) {
        // The extent is re-read on every iteration rather than cached, and GetLength() is called
        // once per element rather than once per pass.
        for (std::vector<Entry>::size_type nIndex = 0; nIndex < mPattern.size(); ++nIndex) {
            mWindowStarts.push_back(GetLength());
            const Entry entry{mPattern[nIndex].mSection, 1};
            mWindow.push_back(entry);
        }
    }

    const std::vector<Entry>::size_type nPassed = mPattern.size();
    // The guard compares a byte offset against an element count. That mismatch is what the binary
    // computes, so the trim only fires once the window is more than eight times the source.
    if ((nPassed * sizeof(Entry)) < mWindow.size()) {
        mWindow.erase(mWindow.begin(), mWindow.begin() + nPassed);
        mWindowStarts.erase(mWindowStarts.begin(), mWindowStarts.begin() + nPassed);
        mTrimmedCount += static_cast<int>(nPassed);
    }
}

inline int PlayMapLinear::WindowIndex(int nBar) {
    GrowPastLimit(nBar);
    const std::vector<int>::iterator it =
        std::upper_bound(mWindowStarts.begin(), mWindowStarts.end(), nBar);
    return static_cast<int>(it - mWindowStarts.begin()) - 1;
}

inline void PlayMapLinear::RestartFrom(std::vector<int>::size_type nFirst) {
    for (std::vector<Entry>::size_type nIndex = nFirst; nIndex < mWindow.size(); ++nIndex) {
        const Entry &previous = mWindow[nIndex - 1];
        mWindowStarts[nIndex] =
            mWindowStarts[nIndex - 1] + (mSectionLengths[previous.mSection] * previous.mRepeats);
    }
}

std::vector<int> &PlayMapLinear::FindBarsPlaying(int nStart, int nMin, int nEnd) {
    GrowPastLimit(nEnd);
    mFoundBars.clear();
    const int nStep = FindStepIndex(nStart);
    const int nLength = mSectionLengths[nStep];
    const int nOffset = nStart - mSteps[nStep];

    std::vector<int>::iterator start =
        std::upper_bound(mWindowStarts.begin(), mWindowStarts.end(), nMin);
    if (mWindowStarts.begin() < start) {
        --start;
    }
    // Unbounded by the end of the window. GrowPastLimit() has grown it past nEnd.
    for (; *start < nEnd; ++start) {
        const Entry &entry = mWindow[start - mWindowStarts.begin()];
        if (entry.mSection != nStep) {
            continue;
        }
        int nPosition = *start + nOffset;
        for (int nPass = 0; nPass < entry.mRepeats; ++nPass) {
            if (nPosition >= nMin) {
                if (nPosition >= nEnd) {
                    break;
                }
                mFoundBars.push_back(nPosition);
            }
            if (nPosition >= nEnd) {
                break;
            }
            nPosition += nLength;
        }
    }
    return mFoundBars;
}

int PlayMapLinear::EndLoop(int nBar) {
    GrowPastLimit(nBar);
    const int nIndex = WindowIndex(nBar);
    Entry &entry = mWindow[nIndex];
    if (entry.mRepeats != kRepeatForever) {
        return 0;
    }
    entry.mRepeats = ((nBar - mWindowStarts[nIndex]) / mSectionLengths[entry.mSection]) + 1;
    RestartFrom(nIndex + 1);
    return 1;
}

int PlayMapLinear::StartLoop(int nBar) {
    GrowPastLimit(nBar);
    const int nIndex = WindowIndex(nBar);
    mWindow[nIndex].mRepeats = kRepeatForever;
    RestartFrom(nIndex + 1);
    return 1;
}

int PlayMapLinear::SelfTest() {
    PlayMapLinear map(kSkipStepRings);
    for (const int nStep : kTestSteps) {
        map.AddStep(nStep, HxStr(kTestLabel));
    }
    for (const int nSection : kTestSections) {
        map.AppendSection(nSection);
    }
    // Yes, the binary discards every result.
    for (const int nProbe : kTestProbes) {
        map.MapBar(nProbe);
    }
    map.ToggleLoop(kTestFirstToggle);
    map.MapBar(kTestLateProbe);
    map.ToggleLoop(kTestSecondToggle);
    map.MapBar(kTestLateProbe);
    return 1;
}

int PlayMapLinear::RunSelfTest() {
    return SelfTest();
}

PlayMapLinear::~PlayMapLinear() {
}

void PlayMapLinear::RecordPattern() {
    mPattern = mWindow;
    mPatternEnd = GetLength();
}

int PlayMapLinear::GetEndBar() {
    return mPatternEnd;
}

int PlayMapLinear::GetNumSections() const {
    return static_cast<int>(mPattern.size());
}

int PlayMapLinear::GetPatternSection(int nIndex) {
    return mPattern[nIndex].mSection;
}

int PlayMapLinear::MapBar(int nBar) {
    GrowPastLimit(nBar);
    const int nIndex = WindowIndex(nBar);
    const int nSection = mWindow[nIndex].mSection;
    return mSteps[nSection] + ((nBar - mWindowStarts[nIndex]) % mSectionLengths[nSection]);
}

int PlayMapLinear::GetLength() const {
    if (mWindowStarts.empty()) {
        return 0;
    }
    const Entry &last = mWindow.back();
    return mWindowStarts.back() + (mSectionLengths[last.mSection] * last.mRepeats);
}

int PlayMapLinear::MapToLinkedStep(int nPosition, int nSet) {
    const int nStep = FindStepIndex(nPosition);
    const std::vector<StepPair> &pairs = mStepRings[nSet];
    for (std::vector<StepPair>::const_iterator it = pairs.begin(); it != pairs.end(); ++it) {
        if (it->mStep == nStep) {
            return (mSteps[it->mPartner] - mSteps[nStep]) + nPosition;
        }
    }
    return nPosition;
}

int PlayMapLinear::GetPatternIndex(int nBar) {
    GrowPastLimit(nBar);
    return static_cast<int>(WindowIndex(nBar) % mPattern.size());
}

int PlayMapLinear::GetAbsoluteSectionIndex(int nBar) {
    GrowPastLimit(nBar);
    return WindowIndex(nBar) + mTrimmedCount;
}

int PlayMapLinear::IsLooping(int nBar) {
    GrowPastLimit(nBar);
    return mWindow[WindowIndex(nBar)].mRepeats == kRepeatForever;
}

int PlayMapLinear::ToggleLoop(int nBar) {
    if (EndLoop(nBar) != 0) {
        return 1;
    }
    StartLoop(nBar); // Yes, the binary discards this call's result.
    return 0;
}
