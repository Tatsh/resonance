#include "os/r250.h"

namespace {

// The linear congruential step SeedR250() fills the table with.
constexpr unsigned int kSeedMultiplier = 0x41c64e6d;
constexpr unsigned int kSeedIncrement = 12345;
constexpr unsigned int kHighHalfShift = 16;
constexpr unsigned int kHighBitsMask = 0x7fff0000;

// Both indices wrap to zero here.
constexpr int kR250Wrap = 249;

// The lag index after seeding.
constexpr int kR250Lag = 103;

// RandomFloat() keeps the low sixteen bits of a draw and scales them into [0, 1).
constexpr int kFloatBitsMask = 0xffff;
constexpr float kFloatScale = 1.0f / 65536.0f;

} // namespace

// 0x0071c8e8
int g_aR250Table[kR250TableSize];

// 0x0071c8e0
int g_nR250Index;

// 0x0071c8e4
int g_nR250LagIndex = kR250Lag;

// 0x0052d038
void SeedR250(int nSeed) {
    unsigned int nValue = static_cast<unsigned int>(nSeed);
    for (int i = 0; i < kR250TableSize; ++i) {
        nValue = (nValue * kSeedMultiplier) + kSeedIncrement;
        const unsigned int nHigh = nValue >> kHighHalfShift;
        nValue = (nValue * kSeedMultiplier) + kSeedIncrement;
        g_aR250Table[i] = static_cast<int>(nHigh + (nValue & kHighBitsMask));
    }
    g_nR250Index = 0;
    g_nR250LagIndex = kR250Lag;
}

// 0x0052cfd0
int NextR250() {
    const int nValue = g_aR250Table[g_nR250Index] ^ g_aR250Table[g_nR250LagIndex];
    g_aR250Table[g_nR250Index] = nValue;
    if (++g_nR250Index >= kR250Wrap) {
        g_nR250Index = 0;
    }
    if (++g_nR250LagIndex >= kR250Wrap) {
        g_nR250LagIndex = 0;
    }
    return nValue;
}

// 0x0052d098
int RandomInt(int nLow, int nHigh) {
    return nLow + (NextR250() % (nHigh - nLow));
}

// 0x0052d0e0
float RandomFloat() {
    return static_cast<float>(NextR250() & kFloatBitsMask) * kFloatScale;
}
