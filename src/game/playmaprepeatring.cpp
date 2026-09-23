#include "game/playmaprepeatring.h"

#include <algorithm>
#include <vector>

#include "os/hxstr.h"

namespace {
// Terminates the position run. The value is the binary's literal and its meaning is unrecovered.
constexpr int kSpanTerminator = 10000000;

// Marks a position that slot 3 has reserved but not yet resolved.
constexpr int kUnresolvedPosition = -1;

// The capacity the constructor reserves in the position run.
constexpr std::vector<int>::size_type kInitialCapacity = 32;

// The label Slot19() gives every step.
static const char *const kStepLabel = "";
} // namespace

// 0x0012b660
PlayMapRepeatRing::PlayMapRepeatRing() {
    mUnknown3c.reserve(kInitialCapacity);
    mUnknown3c.push_back(0);
    mUnknown3c.push_back(kSpanTerminator);
}

// 0x0012d1b8
PlayMapRepeatRing::~PlayMapRepeatRing() {
}

// 0x0012d588, the out-of-line copy.
inline int PlayMapRepeatRing::SpanIndex(int nValue) {
    const std::vector<int>::iterator it =
        std::upper_bound(mUnknown3c.begin(), mUnknown3c.end(), nValue) - 1;
    return static_cast<int>(it - mUnknown3c.begin());
}

// 0x0012d500
int PlayMapRepeatRing::Slot5(int nValue) {
    const int nIndex = SpanIndex(nValue);
    const std::vector<int>::size_type nSection = nIndex % mSectionLengths.size();
    return mSteps[nSection] + ((nValue - mUnknown3c[nIndex]) % mSectionLengths[nSection]);
}

// 0x0012be68
std::vector<int> &PlayMapRepeatRing::Slot6(int nStart, int nMin, int nEnd) {
    mUnknown2c.clear();
    const int nStep = FindStepIndex(nStart);
    const int nOffset = nStart - mSteps[nStep];

    std::vector<int>::iterator span = std::lower_bound(mUnknown3c.begin(), mUnknown3c.end(), nMin);
    if (mUnknown3c.begin() < span) {
        --span;
    }
    // Bounded only by the terminator, which exceeds every recovered nEnd.
    for (; !(nEnd < *span); ++span) {
        const std::vector<int>::size_type nIndex = span - mUnknown3c.begin();
        if ((nIndex % mSectionLengths.size()) != static_cast<std::vector<int>::size_type>(nStep)) {
            continue;
        }
        for (int nPosition = *span + nOffset; nPosition < nEnd;
             nPosition += mSectionLengths[nStep]) {
            if (!(nPosition < nMin)) {
                mUnknown2c.push_back(nPosition);
            }
        }
    }
    return mUnknown2c;
}

// 0x0012bd00
int PlayMapRepeatRing::Slot16(int nBar) {
    const int nIndex = SpanIndex(nBar);
    if (mUnknown3c[nIndex + 1] != kSpanTerminator) {
        return 0;
    }
    const std::vector<int>::size_type nSection = nIndex % mSectionLengths.size();
    Slot15(((nBar - mUnknown3c[nIndex]) / mSectionLengths[nSection]) + 1);
    return 1;
}

// 0x0012bdd8
int PlayMapRepeatRing::Slot17(int nBar) {
    const int nIndex = SpanIndex(nBar);
    int &next = mUnknown3c[nIndex + 1];
    if (next == kSpanTerminator) {
        return 0;
    }
    next = kSpanTerminator;
    mUnknown3c.pop_back();
    return 1;
}

// 0x0012d5d0
int PlayMapRepeatRing::Slot18(int nBar) {
    if (Slot16(nBar) != 0) {
        return 1;
    }
    Slot17(nBar); // Yes, the binary discards this call's result.
    return 0;
}

// 0x0012d4b0
void PlayMapRepeatRing::Slot19(int nValue) {
    PlayMap::Slot2(nValue, HxStr(kStepLabel));
}

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
