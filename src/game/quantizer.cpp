#include "game/quantizer.h"

#include "game/trackdata.h"
#include "mid/mbt.h"

namespace {

// One bar at 480 ticks per quarter note.
constexpr int kBarLength = 1920;

// Round() biases every position this many ticks earlier.
constexpr unsigned kRoundingBias = 8;

} // namespace

// 0x001ce670
Quantizer::Quantizer(const TrackData *pTrackData) : mTrackData(pTrackData) {
}

// 0x001ce680
unsigned Quantizer::Quantize(int nTick) {
    return Round(nTick, GetQuantum(nTick));
}

// 0x001ce6b0
int Quantizer::GetQuantum(int nTick) {
    return mTrackData->GetQuant(nTick / Mid::MBT(kBarLength).mTick);
}

// 0x001ce710
unsigned Quantizer::Round(unsigned nTick, unsigned nQuantum) {
    const Mid::MBT rounded(((nTick - kRoundingBias + (nQuantum / 2)) / nQuantum) * nQuantum);
    return rounded.mTick;
}
