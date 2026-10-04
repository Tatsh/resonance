#include "profile/profiler.h"

#include "os/cycles.h"

namespace {

// Records in each timer table.
constexpr int kProfileTimerCount = 20;

// The EE runs at 294.912 MHz.
constexpr float kMillisecondsPerCycle = 1.0f / 294912.0f;

} // namespace

// NTSC-U/C: 0x00720378, PAL: 0x00763e00
// The static initialiser at 0x0053d700 copies each record from a temporary whose first word it
// never writes.
std::vector<ProfileTimer> g_profileTimers(kProfileTimerCount);

// NTSC-U/C: 0x00720388, PAL: 0x00763e10
std::vector<ProfileTimer> g_lastFrameProfileTimers(kProfileTimerCount);

// NTSC-U/C: 0x00720394, PAL: 0x00763e1c
// Zero until ResetFrameTimer() stores the conversion factor.
float g_flCyclesToMilliseconds;

// NTSC-U/C: 0x00720398, PAL: 0x00763e20
long long g_llFrameTimerCycles;

// NTSC-U/C: 0x007203a0, PAL: 0x00763e28
// The static initialiser zeroes every word except mStartCycles.
ProfileTimer g_frameTimer;

void ResetFrameTimer() {
    g_flCyclesToMilliseconds = kMillisecondsPerCycle;
    if (--g_frameTimer.mDepth == 0) {
        g_frameTimer.mCycles += ReadCycleCount() - g_frameTimer.mStartCycles;
    }
    g_frameTimer.mCycles = 0; // Yes, the binary discards the sum it just made.
    g_frameTimer.mDepth = 1;
    g_frameTimer.mStartCycles = ReadCycleCount();
    g_llFrameTimerCycles = 0;
}
