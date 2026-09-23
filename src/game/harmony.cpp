#include "game/harmony.h"

#include <algorithm>
#include <iostream>
#include <vector>

namespace {

// The range GetRange() reports for a harmony without notes.
constexpr int kDefaultLowNote = 20;
constexpr int kDefaultHighNote = 120;

} // namespace

// 0x001a4db0
void Harmony::AddNote(unsigned char nNote) {
    mNotes.insert(std::upper_bound(mNotes.begin(), mNotes.end(), nNote), nNote);
}

// 0x001a4e28
unsigned char Harmony::Snap(unsigned char nNote) {
    if (mNotes.empty()) {
        return nNote;
    }

    std::vector<unsigned char>::iterator high =
        std::lower_bound(mNotes.begin(), mNotes.end(), nNote);
    if (high == mNotes.end()) {
        --high;
    }
    std::vector<unsigned char>::iterator low = high;
    if (low != mNotes.begin()) {
        --low;
    }

    const unsigned int nMidpoint = (*low + *high) / 2u;
    if (nMidpoint < nNote) {
        return *high;
    }
    return *low;
}

// 0x001a4eb0
void Harmony::GetRange(int *pLow, int *pHigh) {
    if (mNotes.empty()) {
        *pLow = kDefaultLowNote;
        *pHigh = kDefaultHighNote;
        return;
    }
    *pLow = mNotes.front();
    *pHigh = mNotes.back();
}

// 0x001a4ee8
void Harmony::Print(std::ostream &stream) {
    stream << "(";
    for (std::vector<unsigned char>::iterator it = mNotes.begin(); it != mNotes.end(); ++it) {
        stream << static_cast<char>(*it) << " ";
    }
    stream << ")";
}
