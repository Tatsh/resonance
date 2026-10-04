#include "sch/time.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace Sch {

namespace {

constexpr double kNanosecondsPerSecond = 1000000000.0;

// Save() and Load() move the count as two words, the low one first.
constexpr int kTimeWordBits = 32;
constexpr unsigned long long kTimeLowWordMask = 0xffffffffULL;

} // namespace

OBStream &Time::Save(OBStream &stream) {
    // The binary loads each half with its own word load rather than shifting the doubleword.
    const int nLow = static_cast<int>(mValue);
    const int nHigh = static_cast<int>(mValue >> kTimeWordBits);
    return stream.WriteLE(&nLow, sizeof(nLow)).WriteLE(&nHigh, sizeof(nHigh));
}

IBStream &Time::Load(IBStream &stream) {
    // The binary reads each half straight into its own word of the member.
    int nLow;
    int nHigh;
    IBStream &result = stream.ReadLE(&nLow, sizeof(nLow)).ReadLE(&nHigh, sizeof(nHigh));
    mValue = (static_cast<long long>(nHigh) << kTimeWordBits) |
             (static_cast<unsigned long long>(static_cast<unsigned>(nLow)) & kTimeLowWordMask);
    return result;
}

void Time::Print(std::ostream &stream) {
    stream << (static_cast<double>(mValue) / kNanosecondsPerSecond) << "s";
}

} // namespace Sch
