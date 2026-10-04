#include "mid/tick.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace {

// Beats in a measure and ticks in a beat. Print() divides by the beat count before the tick count
// rather than by their product.
constexpr int kBeatsPerMeasure = 4;
constexpr int kTicksPerBeat = 480;
constexpr int kTicksPerMeasure = kBeatsPerMeasure * kTicksPerBeat;

// Largest position Print() renders as a number, and the largest it renders as negative infinity.
constexpr int kTickPrintMaximum = 0x2aaaaaa8;
constexpr int kTickPrintNegativeInfinity = -715827881;

} // namespace

namespace Sch {

int Tick::IsInRange(int nTick) {
    // The sum wraps as an unsigned value, which folds both bounds into one comparison.
    return static_cast<unsigned int>(nTick) + static_cast<unsigned int>(-kTickMinimum) <=
           static_cast<unsigned int>(kTickMaximum) + static_cast<unsigned int>(-kTickMinimum);
}

void Tick::Print(std::ostream &stream) const {
    if (mTick > kTickPrintMaximum) {
        stream << "[inf]tk";
        return;
    }
    if (mTick <= kTickPrintNegativeInfinity) {
        stream << "[-inf]tk";
        return;
    }

    const int nInMeasure = mTick % kTicksPerMeasure;
    stream << mTick / kBeatsPerMeasure / kTicksPerBeat + 1 << ':' << nInMeasure / kTicksPerBeat + 1
           << ':' << nInMeasure % kTicksPerBeat << "tk";
}

OBStream &Tick::saveGuts(OBStream &stream) const {
    const int nTick = mTick;
    return stream.WriteLE(&nTick, sizeof(nTick));
}

IBStream &Tick::restoreGuts(IBStream &stream) {
    return stream.ReadLE(&mTick, sizeof(mTick));
}

} // namespace Sch
