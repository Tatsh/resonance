#include "game/lineartransform.h"

#include <algorithm>

void LinearTransform::Recalc() {
    mSlope = static_cast<float>(mOutMax - mOutMin) / static_cast<float>(mInMax - mInMin);
    mOffset = static_cast<float>(mOutMax) - mSlope * static_cast<float>(mInMax);
    mLower = std::min(mOutMin, mOutMax);
    mUpper = std::max(mOutMin, mOutMax);
}

int LinearTransform::Apply(int nValue) {
    const int nMapped = static_cast<int>(mSlope * static_cast<float>(nValue) + mOffset);
    if (nMapped < mLower) {
        return mLower;
    }
    return std::min(nMapped, mUpper);
}
