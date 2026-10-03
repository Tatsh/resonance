#include "math/color.h"

// NTSC-U/C: 0x004924a8, PAL: 0x004d0358
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

// NTSC-U/C: 0x0052b258, PAL: 0x0056b8d0
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

// NTSC-U/C: 0x00453e08, PAL: 0x00491328
void ScaleColor(const Color &source, float flScale, Color &result) {
    const float flAlpha = source.a * flScale;
    const float flRed = source.r * flScale;
    const float flGreen = source.g * flScale;
    const float flBlue = source.b * flScale;
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

// NTSC-U/C: 0x00607268, PAL: 0x00647ed8
void ClampColorToUnitRange(const Color &source, Color &result) {
    result.r = ClampComponent(source.r);
    result.g = ClampComponent(source.g);
    result.b = ClampComponent(source.b);
    result.a = ClampComponent(source.a);
}
