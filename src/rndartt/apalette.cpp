#include "rndartt/apalette.h"

#include <string.h>

#include "rndartt/normalkey.h"

namespace {

// Larger than the largest sum of three squared byte differences, 0x2fa43, so the first entry of
// any non empty range wins the comparison.
constexpr long kNoMatchDistance = 0x30000;

constexpr unsigned int kChannelMask = 0xff;
constexpr int kGreenShift = 8;
constexpr int kBlueShift = 16;
constexpr unsigned int kAlphaOpaque = 0xff000000;
constexpr int kRGBByteCount = 3;

// BuildRampPalette() writes sixteen shades per key at steps of 1/17, scaled to byte channels, with
// the GS full alpha. Entry 16 then becomes black at that alpha and entry 0 clear.
constexpr int kRampLength = 16;
constexpr float kRampStep = 1.0f / 17.0f;
constexpr float kRampChannelScale = 255.0f;
constexpr unsigned int kRampAlpha = 0x80000000;
constexpr int kRampBlackEntry = 16;
constexpr int kRampClearEntry = 0;
constexpr int kRampMinimumEnd = 17;

} // namespace

// 0x00557970
void APalette::BuildRampPalette(const std::vector<NormalKey> &keys, APalette &palette) {
    palette.mEnd = static_cast<int>(keys.size()) * kRampLength;
    unsigned int *pEntry = palette.mEntries;
    for (const auto &key : keys) {
        const float flRed = key.mScale * key.mRed;
        const float flGreen = key.mScale * key.mGreen;
        const float flBlue = key.mScale * key.mBlue;
        float flShade = 0.0f;
        // Yes, more than kRampLength keys write past mEntries.
        for (int i = kRampLength; i != 0; --i) {
            const unsigned int nBlue =
                static_cast<unsigned int>(flBlue * flShade * kRampChannelScale);
            const unsigned int nGreen =
                static_cast<unsigned int>(flGreen * flShade * kRampChannelScale);
            const unsigned int nRed =
                static_cast<unsigned int>(flRed * flShade * kRampChannelScale);
            flShade += kRampStep;
            *pEntry++ = (nBlue << kBlueShift) | (nGreen << kGreenShift) | kRampAlpha | nRed;
        }
    }
    palette.mEntries[kRampBlackEntry] = kRampAlpha;
    palette.mEntries[kRampClearEntry] = 0;
    if (palette.mEnd < kRampMinimumEnd) {
        palette.mEnd = kRampMinimumEnd;
    }
}

// 0x00613df8
void APalette::SetEntries(const unsigned int *pEntries, int nFirst, int nCount) {
    memcpy(&mEntries[nFirst], pEntries, nCount * sizeof(unsigned int));
    mEnd = nFirst + nCount;
}

// 0x00613e48
void APalette::SetEntriesRGB(const unsigned char *pRGB, int nFirst, int nCount) {
    unsigned int *pEntry = &mEntries[nFirst];
    for (int nRemaining = nCount; nRemaining > 0; --nRemaining) {
        *pEntry++ = pRGB[0] | (pRGB[1] << kGreenShift) | (pRGB[2] << kBlueShift) | kAlphaOpaque;
        pRGB += kRGBByteCount;
    }
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
