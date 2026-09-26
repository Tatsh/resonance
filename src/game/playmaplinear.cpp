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
struct SelfTestRegistration {
    SelfTestRegistration() {
        TestRegistry::Register(kTestName, PlayMapLinear::RunSelfTest);
    }
};
const SelfTestRegistration sSelfTestRegistration;

} // namespace

// 0x00127a80
PlayMapLinear::PlayMapLinear(int bLoadStepRings)
    : mUnknown54(0), mUnknown64(0), mUnknown68(kSetCount) {
    mUnknown3c.reserve(kInitialCapacity);
    mUnknown48.reserve(kInitialCapacity);
    if (bLoadStepRings != 0) {
        LoadStepRings();
    }
}

// 0x00128c38
void PlayMapLinear::Slot20(int nSection) {
    mUnknown48.push_back(Slot8());
    const Entry entry{nSection, 1};
    mUnknown3c.push_back(entry);
}

// 0x00129150
void PlayMapLinear::GrowPastLimit(int nLimit) {
    while (!(nLimit < Slot8())) {
        // The extent is re-read on every iteration rather than cached, and slot 8 is called once
        // per element rather than once per pass.
        for (std::vector<Entry>::size_type nIndex = 0; nIndex < mUnknown58.size(); ++nIndex) {
            mUnknown48.push_back(Slot8());
            const Entry entry{mUnknown58[nIndex].mSection, 1};
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

// 0x0012ade8
inline int PlayMapLinear::WindowIndex(int nValue) {
    GrowPastLimit(nValue);
    const std::vector<int>::iterator it =
        std::upper_bound(mUnknown48.begin(), mUnknown48.end(), nValue);
    return static_cast<int>(it - mUnknown48.begin()) - 1;
}

inline void PlayMapLinear::RestartFrom(std::vector<int>::size_type nFirst) {
    for (std::vector<Entry>::size_type nIndex = nFirst; nIndex < mUnknown3c.size(); ++nIndex) {
        const Entry &previous = mUnknown3c[nIndex - 1];
        mUnknown48[nIndex] =
            mUnknown48[nIndex - 1] + (mSectionLengths[previous.mSection] * previous.mRepeats);
    }
}

// 0x00128d08
std::vector<int> &PlayMapLinear::Slot6(int nStart, int nMin, int nEnd) {
    GrowPastLimit(nEnd);
    mUnknown2c.clear();
    const int nStep = FindStepIndex(nStart);
    const int nLength = mSectionLengths[nStep];
    const int nOffset = nStart - mSteps[nStep];

    std::vector<int>::iterator start = std::upper_bound(mUnknown48.begin(), mUnknown48.end(), nMin);
    if (mUnknown48.begin() < start) {
        --start;
    }
    // Unbounded by the end of the window. GrowPastLimit() has grown it past nEnd.
    for (; *start < nEnd; ++start) {
        const Entry &entry = mUnknown3c[start - mUnknown48.begin()];
        if (entry.mSection != nStep) {
            continue;
        }
        int nPosition = *start + nOffset;
        for (int nPass = 0; nPass < entry.mRepeats; ++nPass) {
            if (nPosition >= nMin) {
                if (nPosition >= nEnd) {
                    break;
                }
                mUnknown2c.push_back(nPosition);
            }
            if (nPosition >= nEnd) {
                break;
            }
            nPosition += nLength;
        }
    }
    return mUnknown2c;
}

// 0x00128ed8
int PlayMapLinear::Slot16(int nBar) {
    GrowPastLimit(nBar);
    const int nIndex = WindowIndex(nBar);
    Entry &entry = mUnknown3c[nIndex];
    if (entry.mRepeats != kRepeatForever) {
        return 0;
    }
    entry.mRepeats = ((nBar - mUnknown48[nIndex]) / mSectionLengths[entry.mSection]) + 1;
    RestartFrom(nIndex + 1);
    return 1;
}

// 0x00129038
int PlayMapLinear::Slot17(int nBar) {
    GrowPastLimit(nBar);
    const int nIndex = WindowIndex(nBar);
    mUnknown3c[nIndex].mRepeats = kRepeatForever;
    RestartFrom(nIndex + 1);
    return 1;
}

// 0x001293b0
int PlayMapLinear::SelfTest() {
    PlayMapLinear map(kSkipStepRings);
    for (const int nStep : kTestSteps) {
        map.Slot2(nStep, HxStr(kTestLabel));
    }
    for (const int nSection : kTestSections) {
        map.Slot20(nSection);
    }
    // Yes, the binary discards every result.
    for (const int nProbe : kTestProbes) {
        map.Slot5(nProbe);
    }
    map.Slot18(kTestFirstToggle);
    map.Slot5(kTestLateProbe);
    map.Slot18(kTestSecondToggle);
    map.Slot5(kTestLateProbe);
    return 1;
}

// 0x0012b110
int PlayMapLinear::RunSelfTest() {
    return SelfTest();
}

// 0x0012a4d8
PlayMapLinear::~PlayMapLinear() {
}

// 0x0012aa18
void PlayMapLinear::Slot19() {
    mUnknown58 = mUnknown3c;
    mUnknown64 = Slot8();
}

// 0x0012aa60
int PlayMapLinear::Slot9() {
    return mUnknown64;
}

// 0x0012aa68
int PlayMapLinear::Slot10() {
    return static_cast<int>(mUnknown58.size());
}

// 0x0012aa80
int PlayMapLinear::Slot11(int nValue) {
    return mUnknown58[nValue].mSection;
}

// 0x0012ad58
int PlayMapLinear::Slot5(int nValue) {
    GrowPastLimit(nValue);
    const int nIndex = WindowIndex(nValue);
    const int nSection = mUnknown3c[nIndex].mSection;
    return mSteps[nSection] + ((nValue - mUnknown48[nIndex]) % mSectionLengths[nSection]);
}

// 0x0012ae38
int PlayMapLinear::Slot8() {
    if (mUnknown48.empty()) {
        return 0;
    }
    const Entry &last = mUnknown3c.back();
    return mUnknown48.back() + (mSectionLengths[last.mSection] * last.mRepeats);
}

// 0x0012ae88
int PlayMapLinear::Slot7(int nValue, int nSet) {
    const int nStep = FindStepIndex(nValue);
    const std::vector<StepPair> &pairs = mUnknown68[nSet];
    for (std::vector<StepPair>::const_iterator it = pairs.begin(); it != pairs.end(); ++it) {
        if (it->mStep == nStep) {
            return (mSteps[it->mPartner] - mSteps[nStep]) + nValue;
        }
    }
    return nValue;
}

// 0x0012af30
int PlayMapLinear::Slot12(int nValue) {
    GrowPastLimit(nValue);
    return static_cast<int>(WindowIndex(nValue) % mUnknown58.size());
}

// 0x0012afb8
int PlayMapLinear::Slot13(int nValue) {
    GrowPastLimit(nValue);
    return WindowIndex(nValue) + mUnknown54;
}

// 0x0012b028
int PlayMapLinear::Slot14(int nValue) {
    GrowPastLimit(nValue);
    return mUnknown3c[WindowIndex(nValue)].mRepeats == kRepeatForever;
}

// 0x0012b0a8
int PlayMapLinear::Slot18(int nBar) {
    if (Slot16(nBar) != 0) {
        return 1;
    }
    Slot17(nBar); // Yes, the binary discards this call's result.
    return 0;
}
