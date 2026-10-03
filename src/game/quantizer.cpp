#include "game/quantizer.h"

#include "game/trackdata.h"
#include "mid/tick.h"

namespace {

// One bar at 480 ticks per quarter note.
constexpr int kBarLength = 1920;

// Round() biases every position this many ticks earlier.
constexpr unsigned kRoundingBias = 8;

} // namespace

// NTSC-U/C: 0x001ce670, PAL: 0x001d4528
Quantizer::Quantizer(const TrackData *pTrackData) : mTrackData(pTrackData) {
}

// NTSC-U/C: 0x001ce680, PAL: 0x001d4538
int Quantizer::Quantize(int nTick) {
    return Round(nTick, GetQuantum(nTick));
}

// NTSC-U/C: 0x001ce6b0, PAL: 0x001d4568
int Quantizer::GetQuantum(int nTick) {
    return mTrackData->GetQuant(nTick / Sch::Tick(kBarLength).mTick);
}

// NTSC-U/C: 0x001ce710, PAL: 0x001d45c8
unsigned Quantizer::Round(unsigned nTick, unsigned nQuantum) {
    const Sch::Tick rounded(((nTick - kRoundingBias + (nQuantum / 2)) / nQuantum) * nQuantum);
    return rounded.mTick;
}
