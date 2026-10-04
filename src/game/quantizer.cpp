#include "game/quantizer.h"

#include "game/trackdata.h"
#include "mid/tick.h"

namespace {

// One bar at 480 ticks per quarter note.
constexpr int kBarLength = 1920;

// Round() biases every position this many ticks earlier.
constexpr unsigned kRoundingBias = 8;

} // namespace

Quantizer::Quantizer(const TrackData *pTrackData) : mTrackData(pTrackData) {
}

int Quantizer::Quantize(int nTick) {
    return Round(nTick, GetQuantum(nTick));
}

int Quantizer::GetQuantum(int nTick) {
    return mTrackData->GetQuant(nTick / Sch::Tick(kBarLength).mTick);
}

unsigned Quantizer::Round(unsigned nTick, unsigned nQuantum) {
    const Sch::Tick rounded(((nTick - kRoundingBias + (nQuantum / 2)) / nQuantum) * nQuantum);
    return rounded.mTick;
}
