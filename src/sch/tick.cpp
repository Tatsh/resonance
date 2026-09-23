#include "sch/tick.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace Sch {

namespace {

constexpr double kNanosecondsPerSecond = 1000000000.0;

// Save() and Load() move the count as two words, the low one first.
constexpr int kTickWordBits = 32;
constexpr unsigned long long kTickLowWordMask = 0xffffffffULL;

} // namespace

// 0x006100a8
OBStream &Tick::Save(OBStream &stream) {
    // The binary loads each half with its own word load rather than shifting the doubleword.
    const int nLow = static_cast<int>(mValue);
    const int nHigh = static_cast<int>(mValue >> kTickWordBits);
    return stream.Write(&nLow, sizeof(nLow)).Write(&nHigh, sizeof(nHigh));
}

// 0x00610118
IBStream &Tick::Load(IBStream &stream) {
    // The binary reads each half straight into its own word of the member.
    int nLow;
    int nHigh;
    IBStream &result = stream.Read(&nLow, sizeof(nLow)).Read(&nHigh, sizeof(nHigh));
    mValue = (static_cast<long long>(nHigh) << kTickWordBits) |
             (static_cast<unsigned long long>(static_cast<unsigned>(nLow)) & kTickLowWordMask);
    return result;
}

// 0x00610050
void Tick::Print(std::ostream &stream) {
    stream << (static_cast<double>(mValue) / kNanosecondsPerSecond) << "s";
}

} // namespace Sch
