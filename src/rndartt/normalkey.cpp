#include "rndartt/normalkey.h"

#include "math/color.h"

namespace {

// The largest summed ratio difference at which two keys count as one hue.
constexpr float kMatchTolerance = 0.02f;

constexpr float kUnusedKeyScale = 16.0f;

} // namespace

// 0x00725848
NormalKey g_normalKeyUnused(kUnusedKeyScale, 0.0f, 0.0f);

// 0x00558740
NormalKey::NormalKey(float flRed, float flGreen, float flBlue) {
    if (flGreen < flRed) {
        if (flBlue < flRed) {
            mScale = flRed;
            mRed = 1.0f;
            mGreen = flGreen / flRed;
            mBlue = flBlue / flRed;
            return;
        }
    } else if (flBlue < flGreen) {
        mScale = flGreen;
        mRed = flRed / flGreen;
        mGreen = 1.0f;
        mBlue = flBlue / flGreen;
        return;
    } else if (flBlue == 0.0) { // Yes, the binary widens blue to double for this test.
        mScale = 0.0f;
        mRed = 1.0f;
        mGreen = 1.0f;
        mBlue = 1.0f;
        return;
    }
    mScale = flBlue;
    mRed = flRed / flBlue;
    mGreen = flGreen / flBlue;
    mBlue = 1.0f;
}

// 0x00557858
void NormalKey::InsertUniqueNormalKey(std::vector<NormalKey> &keys, const Color &color) {
    const NormalKey key(color.r, color.g, color.b);
    for (auto &existing : keys) {
        if (existing.RatioDistance(key) < kMatchTolerance) {
            if (existing.mScale < key.mScale) {
                existing = key;
            }
            return;
        }
    }
    keys.push_back(key);
}
