#include "os/random.h"

namespace {

constexpr unsigned int kSeedMultiplier = 65539;
constexpr unsigned int kSeedMask = 0x7fffffff;

} // namespace

// NTSC-U/C: 0x0072407c, PAL: 0x00767c6c
int g_nRandomSeed = 1;

// NTSC-U/C: 0x0054f770, PAL: 0x0058fdb0
int NextRandomValue() {
    // The product wraps as the 32-bit mult does, which a signed multiply would not guarantee.
    g_nRandomSeed =
        static_cast<int>((static_cast<unsigned int>(g_nRandomSeed) * kSeedMultiplier) & kSeedMask);
    return g_nRandomSeed;
}
