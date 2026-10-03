#include "game/lineartransform.h"

#include <algorithm>

// NTSC-U/C: 0x00536f50, PAL: 0x00576810
void LinearTransform::Init() {
    mSlope = static_cast<float>(mOutMax - mOutMin) / static_cast<float>(mInMax - mInMin);
    mOffset = static_cast<float>(mOutMax) - mSlope * static_cast<float>(mInMax);
    mLower = std::min(mOutMin, mOutMax);
    mUpper = std::max(mOutMin, mOutMax);
}

// NTSC-U/C: 0x00536fe0, PAL: 0x005768a0
int LinearTransform::Map(int nValue) {
    const int nMapped = static_cast<int>(mSlope * static_cast<float>(nValue) + mOffset);
    if (nMapped < mLower) {
        return mLower;
    }
    return std::min(nMapped, mUpper);
}
