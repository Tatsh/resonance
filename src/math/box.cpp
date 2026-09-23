#include "math/box.h"

namespace {

// The image loops over the three axes. Each pass is this test.
inline void GrowAxisToContain(float flValue, float &flMin, float &flMax) {
    if (flValue < flMin) {
        flMin = flValue;
    } else if (flMax < flValue) {
        flMax = flValue;
    }
}

} // namespace

// 0x00551020
void Box::GrowToContain(const Vector3 &point) {
    GrowAxisToContain(point.x, mMin.x, mMax.x);
    GrowAxisToContain(point.y, mMin.y, mMax.y);
    GrowAxisToContain(point.z, mMin.z, mMax.z);
}
