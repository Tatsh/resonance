#include "game/harmony.h"

#include <algorithm>
#include <iostream>
#include <vector>

namespace {

// The range GetRange() reports for a harmony without notes.
constexpr int kDefaultLowNote = 20;
constexpr int kDefaultHighNote = 120;

} // namespace

void Harmony::AddPitch(unsigned char nNote) {
    mNotes.insert(std::upper_bound(mNotes.begin(), mNotes.end(), nNote), nNote);
}

unsigned char Harmony::SnapToHarmony(unsigned char nNote) const {
    if (mNotes.empty()) {
        return nNote;
    }

    std::vector<unsigned char>::const_iterator high =
        std::lower_bound(mNotes.begin(), mNotes.end(), nNote);
    if (high == mNotes.end()) {
        --high;
    }
    std::vector<unsigned char>::const_iterator low = high;
    if (low != mNotes.begin()) {
        --low;
    }

    const unsigned int nMidpoint = (*low + *high) / 2u;
    if (nMidpoint < nNote) {
        return *high;
    }
    return *low;
}

void Harmony::GetRange(int *pLow, int *pHigh) {
    if (mNotes.empty()) {
        *pLow = kDefaultLowNote;
        *pHigh = kDefaultHighNote;
        return;
    }
    *pLow = mNotes.front();
    *pHigh = mNotes.back();
}

void Harmony::Print(std::ostream &stream) {
    stream << "(";
    for (std::vector<unsigned char>::iterator it = mNotes.begin(); it != mNotes.end(); ++it) {
        stream << static_cast<char>(*it) << " ";
    }
    stream << ")";
}
