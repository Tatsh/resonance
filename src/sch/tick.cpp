#include "sch/tick.h"

#include <iostream>

namespace Sch {

namespace {

constexpr double kNanosecondsPerSecond = 1000000000.0;

} // namespace

// 0x00610050
void Tick::Print(std::ostream &stream) {
    stream << (static_cast<double>(mValue) / kNanosecondsPerSecond) << "s";
}

} // namespace Sch
