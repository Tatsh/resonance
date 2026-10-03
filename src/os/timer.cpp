#include "os/timer.h"

#include "os/cycles.h"

float Timer::sClock2Ms;

int Timer::sElapsedMs;

long long Timer::sElapsedCycles;

unsigned Timer::sElapsedTimer;

unsigned Timer::sLastCycleDelta;

// NTSC-U/C: 0x004fefb0, PAL: 0x0053dd60
void Timer::Init() {
    sClock2Ms = 1.0f / kCyclesPerMillisecond;
    (void)ReadCycleCount(); // Yes, the binary discards this reading.
    sLastCycleDelta = 0;
    sElapsedTimer = ReadCycleCount();
    sElapsedMs = 0;
    sElapsedCycles = 0;
}
