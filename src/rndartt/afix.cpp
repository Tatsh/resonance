#include "rndartt/afix.h"

namespace {

constexpr int kFixedOne = 0x100;

} // namespace

// NTSC-U/C: 0x007a82c0, PAL: 0x007ebfc0
int AFix::onehalf = kFixedOne / 2;

// NTSC-U/C: 0x007a82c8, PAL: 0x007ebfc8
int AFix::e = 695;

// NTSC-U/C: 0x007a82d0, PAL: 0x007ebfd0
int AFix::pi = 804;

// NTSC-U/C: 0x007a82d8, PAL: 0x007ebfd8
int AFix::epsilon = 1;

// NTSC-U/C: 0x007a82e0, PAL: 0x007ebfe0
int AFix::max = 0x7fffff;
