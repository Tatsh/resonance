#include "sch/tick.h"

namespace Sch {

namespace {

constexpr double kNanosecondsPerSecond = 1000000000.0;

} // namespace

// 0x00610050
void Tick::Print(ostream &stream) {
    stream << (static_cast<double>(mValue) / kNanosecondsPerSecond) << "s";
}

} // namespace Sch
