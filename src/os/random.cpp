#include "os/random.h"

namespace {

constexpr unsigned int kSeedMultiplier = 65539;
constexpr unsigned int kSeedMask = 0x7fffffff;

} // namespace

// 0x0072407c
int g_nRandomSeed = 1;

// 0x0054f770
int NextRandomValue() {
    // The product wraps as the 32-bit mult does, which a signed multiply would not guarantee.
    g_nRandomSeed =
        static_cast<int>((static_cast<unsigned int>(g_nRandomSeed) * kSeedMultiplier) & kSeedMask);
    return g_nRandomSeed;
}
