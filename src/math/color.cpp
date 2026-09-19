#include "math/color.h"

// 0x004924a8
void AddColor(const Color &left, const Color &right, Color &result) {
    const float flAlpha = left.a + right.a;
    const float flRed = left.r + right.r;
    const float flGreen = left.g + right.g;
    const float flBlue = left.b + right.b;
    result.a = flAlpha;
    result.r = flRed;
    result.g = flGreen;
    result.b = flBlue;
}

// 0x0052b258
void SubColor(const Color &left, const Color &right, Color &result) {
    const float flAlpha = left.a - right.a;
    const float flRed = left.r - right.r;
    const float flGreen = left.g - right.g;
    const float flBlue = left.b - right.b;
    result.a = flAlpha;
    result.r = flRed;
    result.g = flGreen;
    result.b = flBlue;
}

namespace {

// The ceiling the vector unit broadcasts into the minimum stage.
constexpr float kColorCeiling = 1.0f;

// The floor the vector unit broadcasts into the maximum stage.
constexpr float kColorFloor = 0.0f;

inline float ClampComponent(float flValue) {
    if (flValue < kColorFloor) {
        return kColorFloor;
    }
    if (flValue > kColorCeiling) {
        return kColorCeiling;
    }
    return flValue;
}

} // namespace

// 0x00607268
void ClampColorToUnitRange(const Color &source, Color &result) {
    result.r = ClampComponent(source.r);
    result.g = ClampComponent(source.g);
    result.b = ClampComponent(source.b);
    result.a = ClampComponent(source.a);
}
