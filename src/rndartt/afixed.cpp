#include "rndartt/afixed.h"

namespace {

constexpr int kFixedOne = 0x100;

} // namespace

// NTSC-U/C: 0x007a82c0, PAL: 0x007ebfc0
int g_nFixedHalf = kFixedOne / 2;

// NTSC-U/C: 0x007a82c8, PAL: 0x007ebfc8
int g_nFixedE = 695;

// NTSC-U/C: 0x007a82d0, PAL: 0x007ebfd0
int g_nFixedPi = 804;

// NTSC-U/C: 0x007a82d8, PAL: 0x007ebfd8
int g_nFixedEpsilon = 1;

// NTSC-U/C: 0x007a82e0, PAL: 0x007ebfe0
int g_nFixedMax = 0x7fffff;
