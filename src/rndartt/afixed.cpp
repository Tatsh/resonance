#include "rndartt/afixed.h"

namespace {

constexpr int kFixedOne = 0x100;

} // namespace

// 0x007a82c0
int g_nFixedHalf = kFixedOne / 2;

// 0x007a82c8
int g_nFixedE = 695;

// 0x007a82d0
int g_nFixedPi = 804;

// 0x007a82d8
int g_nFixedEpsilon = 1;

// 0x007a82e0
int g_nFixedMax = 0x7fffff;
