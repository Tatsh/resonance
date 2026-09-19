#include "sch/tempomap.h"

namespace Sch {

namespace {

// 1000 nanoseconds per microsecond over 480 MIDI ticks per quarter note, reduced. The binary
// multiplies by 25 with an unsigned 32-bit multiply and divides the 64-bit product by 12, so the
// reduced pair is what the instructions at 0x0052d138 and 0x0052d164 use.
constexpr long long kNanosecondScaleNumerator = 25;
constexpr long long kNanosecondScaleDenominator = 12;

long long NanosecondsPerTickFor(int nMicrosecondsPerQuarter) {
    unsigned long long ullProduct =
        static_cast<unsigned>(nMicrosecondsPerQuarter) * kNanosecondScaleNumerator;
    return static_cast<long long>(ullProduct) / kNanosecondScaleDenominator;
}

} // namespace

// 0x0052d118
TempoMap::TempoMap(int nMicrosecondsPerQuarter)
    : mNanosecondsPerTick(NanosecondsPerTickFor(nMicrosecondsPerQuarter)), mOriginNanoseconds(0),
      mCeilingBias(NanosecondsPerTickFor(nMicrosecondsPerQuarter) - 1),
      mMicrosecondsPerQuarter(nMicrosecondsPerQuarter) {
}

// 0x0052d240
TempoMap::~TempoMap() {
}

// 0x0052d198
void TempoMap::SetTempo(int nMicrosecondsPerQuarter, long long nTick) {
    long long nWhen = (mNanosecondsPerTick * nTick) + mOriginNanoseconds;
    // The binary stores the tempo before it computes the new scale, at 0x0052d1d4.
    mMicrosecondsPerQuarter = nMicrosecondsPerQuarter;
    mNanosecondsPerTick = NanosecondsPerTickFor(nMicrosecondsPerQuarter);
    mOriginNanoseconds = nWhen - (mNanosecondsPerTick * nTick);
    mCeilingBias = mNanosecondsPerTick - mOriginNanoseconds - 1;
}

} // namespace Sch
