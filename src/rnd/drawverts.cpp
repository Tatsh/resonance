#include "rnd/drawverts.h"

#include "math/sphere.h"

namespace Rnd {

namespace {

// Lanes of one plane equation, the normal and then the distance term.
enum { kPlaneNormalX = 0, kPlaneNormalY = 1, kPlaneNormalZ = 2, kPlaneDistance = 3 };

} // namespace

// 0x005514a0
// The binary runs all six planes on VU0 in macro mode and reads the sticky sign bit of the status
// register once at the end, so every plane is evaluated before the answer is known.
int IsSphereInsideFrustum(const Sphere &sphere, const float *pPlanes) {
    int bAnyNegative = 0;
    for (int nPlane = 0; nPlane < kFrustumPlaneCount; ++nPlane) {
        const float *pPlane = &pPlanes[nPlane * kFrustumPlaneFloatCount];
        const float flDistance = pPlane[kPlaneNormalX] * sphere.mCenter.x +
                                 pPlane[kPlaneNormalY] * sphere.mCenter.y +
                                 pPlane[kPlaneNormalZ] * sphere.mCenter.z + pPlane[kPlaneDistance];
        if (flDistance - sphere.mRadius < 0.0f) {
            bAnyNegative = 1;
        }
    }
    return !bAnyNegative;
}

} // namespace Rnd
