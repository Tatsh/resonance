#include "mid/mbt.h"

// 0x00100ab8
int IsFiniteMBT(int nTick) {
    // The sum wraps as an unsigned value, which folds both bounds into one comparison.
    return static_cast<unsigned int>(nTick) + static_cast<unsigned int>(-kMBTMinimum) <=
           static_cast<unsigned int>(kMBTMaximum) + static_cast<unsigned int>(-kMBTMinimum);
}
