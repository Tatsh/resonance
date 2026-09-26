#include "rnd/drawverts.h"

#include "math/frustum.h"
#include "math/sphere.h"

namespace Rnd {

// 0x005514a0
// The binary runs all six planes on VU0 in macro mode and reads the sticky sign bit of the status
// register once at the end, so every plane is evaluated before the answer is known.
int IsSphereInsideFrustum(const Sphere &sphere, const Frustum &frustum) {
    const Plane *const apPlanes[] = {&frustum.mFront,
                                     &frustum.mBack,
                                     &frustum.mLeft,
                                     &frustum.mRight,
                                     &frustum.mTop,
                                     &frustum.mBottom};
    int bAnyNegative = 0;
    for (const Plane *pPlane : apPlanes) {
        const float flDistance = pPlane->a * sphere.mCenter.x + pPlane->b * sphere.mCenter.y +
                                 pPlane->c * sphere.mCenter.z + pPlane->d;
        if (flDistance - sphere.mRadius < 0.0f) {
            bAnyNegative = 1;
        }
    }
    return !bAnyNegative;
}

} // namespace Rnd
