#include "game/playmaprepeatring.h"

#include <algorithm>
#include <vector>

#include "os/hxstr.h"

namespace {
// Terminates the span run. The value is the binary's literal and its meaning is unrecovered.
constexpr int kSpanTerminator = 10000000;

// Marks a span start that SetBarCount() has reserved but not yet resolved.
constexpr int kUnresolvedPosition = -1;

// The capacity the constructor reserves in the span run.
constexpr std::vector<int>::size_type kInitialCapacity = 32;

// The label AddUnlabeledStep() gives every step.
static const char *const kStepLabel = "";
} // namespace

// 0x0012b660
PlayMapRepeatRing::PlayMapRepeatRing() {
    mSpanStarts.reserve(kInitialCapacity);
    mSpanStarts.push_back(0);
    mSpanStarts.push_back(kSpanTerminator);
}

// 0x0012d1b8
PlayMapRepeatRing::~PlayMapRepeatRing() {
}

// 0x0012d588
inline int PlayMapRepeatRing::SpanIndex(int nBar) {
    const std::vector<int>::iterator it =
        std::upper_bound(mSpanStarts.begin(), mSpanStarts.end(), nBar) - 1;
    return static_cast<int>(it - mSpanStarts.begin());
}

// 0x0012d500
int PlayMapRepeatRing::MapBar(int nBar) {
    const int nIndex = SpanIndex(nBar);
    const std::vector<int>::size_type nSection = nIndex % mSectionLengths.size();
    return mSteps[nSection] + ((nBar - mSpanStarts[nIndex]) % mSectionLengths[nSection]);
}

// 0x0012be68
std::vector<int> &PlayMapRepeatRing::FindBarsPlaying(int nStart, int nMin, int nEnd) {
    mFoundBars.clear();
    const int nStep = FindStepIndex(nStart);
    const int nOffset = nStart - mSteps[nStep];

    std::vector<int>::iterator span =
        std::lower_bound(mSpanStarts.begin(), mSpanStarts.end(), nMin);
    if (mSpanStarts.begin() < span) {
        --span;
    }
    // Bounded only by the terminator, which exceeds every recovered nEnd.
    for (; !(nEnd < *span); ++span) {
        const std::vector<int>::size_type nIndex = span - mSpanStarts.begin();
        if ((nIndex % mSectionLengths.size()) != static_cast<std::vector<int>::size_type>(nStep)) {
            continue;
        }
        for (int nPosition = *span + nOffset; nPosition < nEnd;
             nPosition += mSectionLengths[nStep]) {
            if (!(nPosition < nMin)) {
                mFoundBars.push_back(nPosition);
            }
        }
    }
    return mFoundBars;
}

// 0x0012bd00
int PlayMapRepeatRing::EndLoop(int nBar) {
    const int nIndex = SpanIndex(nBar);
    if (mSpanStarts[nIndex + 1] != kSpanTerminator) {
        return 0;
    }
    const std::vector<int>::size_type nSection = nIndex % mSectionLengths.size();
    CloseSpan(((nBar - mSpanStarts[nIndex]) / mSectionLengths[nSection]) + 1);
    return 1;
}

// 0x0012bdd8
int PlayMapRepeatRing::StartLoop(int nBar) {
    const int nIndex = SpanIndex(nBar);
    int &next = mSpanStarts[nIndex + 1];
    if (next == kSpanTerminator) {
        return 0;
    }
    next = kSpanTerminator;
    mSpanStarts.pop_back();
    return 1;
}

// 0x0012d5d0
int PlayMapRepeatRing::ToggleLoop(int nBar) {
    if (EndLoop(nBar) != 0) {
        return 1;
    }
    StartLoop(nBar); // Yes, the binary discards this call's result.
    return 0;
}

// 0x0012d4b0
void PlayMapRepeatRing::AddUnlabeledStep(int nPosition) {
    PlayMap::AddStep(nPosition, HxStr(kStepLabel));
}

// 0x0012ba88
void PlayMapRepeatRing::ResetSpans() {
    mSpanStarts.clear();
    mSpanStarts.push_back(0);
    mSpanStarts.push_back(kSpanTerminator);
}

// 0x0012bb60
void PlayMapRepeatRing::SetBarCount(int nBarCount) {
    PlayMap::SetBarCount(nBarCount);
    ResetSpans(); // Dispatched through the table. A further subclass would run instead.
    const int nPrefix =
        static_cast<int>(std::find(mSteps.begin(), mSteps.end(), nBarCount) - mSteps.begin());
    for (int nIndex = 0; nIndex < nPrefix; ++nIndex) {
        mSpanStarts.push_back(kUnresolvedPosition);
    }
}

// 0x0012bc48
void PlayMapRepeatRing::CloseSpan(int nRepeats) {
    // The binary takes the remainder with an unsigned divide, so the sizes stay unsigned here
    // rather than being narrowed to int.
    const std::vector<int>::size_type nLast = mSpanStarts.size() - 1;
    const std::vector<int>::size_type nPrevious = nLast - 1;
    const int nGap = mSectionLengths[nPrevious % mSectionLengths.size()];
    mSpanStarts[nLast] = mSpanStarts[nPrevious] + (nRepeats * nGap);
    mSpanStarts.push_back(kSpanTerminator);
}
