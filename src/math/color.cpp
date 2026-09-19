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
