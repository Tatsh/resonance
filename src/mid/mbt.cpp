#include "mid/mbt.h"

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
constexpr int kMBTPrintMaximum = 0x2aaaaaa8;
constexpr int kMBTPrintNegativeInfinity = -715827881;

} // namespace

// NTSC-U/C: 0x00100ab8, PAL: 0x00100ab8
int IsFiniteMBT(int nTick) {
    // The sum wraps as an unsigned value, which folds both bounds into one comparison.
    return static_cast<unsigned int>(nTick) + static_cast<unsigned int>(-kMBTMinimum) <=
           static_cast<unsigned int>(kMBTMaximum) + static_cast<unsigned int>(-kMBTMinimum);
}

namespace Mid {

// NTSC-U/C: 0x004ace18, PAL: 0x004eafb8
void MBT::Print(std::ostream &stream) {
    if (mTick > kMBTPrintMaximum) {
        stream << "[inf]tk";
        return;
    }
    if (mTick <= kMBTPrintNegativeInfinity) {
        stream << "[-inf]tk";
        return;
    }

    const int nInMeasure = mTick % kTicksPerMeasure;
    stream << mTick / kBeatsPerMeasure / kTicksPerBeat + 1 << ':' << nInMeasure / kTicksPerBeat + 1
           << ':' << nInMeasure % kTicksPerBeat << "tk";
}

// NTSC-U/C: 0x004acf28, PAL: 0x004eb0c8
OBStream &MBT::Save(OBStream &stream) {
    const int nTick = mTick;
    return stream.Write(&nTick, sizeof(nTick));
}

// NTSC-U/C: 0x004acf68, PAL: 0x004eb108
IBStream &MBT::Load(IBStream &stream) {
    return stream.Read(&mTick, sizeof(mTick));
}

} // namespace Mid
