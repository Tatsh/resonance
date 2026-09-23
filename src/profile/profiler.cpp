#include "profile/profiler.h"

namespace {

// Records in each timer table.
constexpr int kProfileTimerCount = 20;

} // namespace

// 0x00720378
// The static initialiser at 0x0053d700 copies each record from a temporary whose first word it
// never writes.
std::vector<ProfileTimer> g_profileTimers(kProfileTimerCount);

// 0x00720388
std::vector<ProfileTimer> g_lastFrameProfileTimers(kProfileTimerCount);

// 0x00720394
// Zero until the frame timer reset at 0x0053dcf8 stores the conversion factor.
float g_flCyclesToMilliseconds;
