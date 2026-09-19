#include "rndartt/apalette.h"

#include <string.h>

namespace {

// Larger than the largest sum of three squared byte differences, 0x2fa43, so the first entry of
// any non empty range wins the comparison.
constexpr long kNoMatchDistance = 0x30000;

constexpr unsigned int kChannelMask = 0xff;
constexpr int kGreenShift = 8;
constexpr int kBlueShift = 16;

} // namespace

// 0x00613df8
void APalette::SetEntries(const unsigned int *pEntries, int nFirst, int nCount) {
    memcpy(&mEntries[nFirst], pEntries, nCount * sizeof(unsigned int));
    mEnd = nFirst + nCount;
}

// 0x00613f10
int APalette::FindNearestEntry(unsigned int nColor, int nFirst, int nLast) const {
    const long nRed = nColor & kChannelMask;
    const long nGreen = (nColor >> kGreenShift) & kChannelMask;
    const long nBlue = (nColor >> kBlueShift) & kChannelMask;
    long nBestDistance = kNoMatchDistance;
    int nBestIndex = 0;
    for (int i = nFirst; i <= nLast; ++i) {
        const unsigned int nEntry = mEntries[i];
        const long nDeltaRed = nRed - (nEntry & kChannelMask);
        const long nDeltaGreen = nGreen - ((nEntry >> kGreenShift) & kChannelMask);
        const long nDeltaBlue = nBlue - ((nEntry >> kBlueShift) & kChannelMask);
        const long nDistance =
            nDeltaRed * nDeltaRed + nDeltaGreen * nDeltaGreen + nDeltaBlue * nDeltaBlue;
        if (nDistance < nBestDistance) {
            nBestDistance = nDistance;
            nBestIndex = i;
        }
    }
    return nBestIndex & kChannelMask;
}
