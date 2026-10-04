#include "math/frustum.h"

#include <math.h>

#include "math/sphere.h"
#include "math/vector3.h"
#include "os/dbg.h"

namespace {

// One plane under its title, the way the frustum printer writes each of the six. Every call after
// the title goes to the sink the previous call returned.
void PrintPlane(Rnd::Dbg &sink, const char *pszTitle, const Plane &plane) {
    sink.Print(pszTitle)
        ->Print("(a:")
        ->Format("%.2f", plane.a)
        ->Print(" b:")
        ->Format("%.2f", plane.b)
        ->Print(" c:")
        ->Format("%.2f", plane.c)
        ->Print(" d:")
        ->Format("%.2f", plane.d)
        ->Print(")");
}

// Set a plane to the normal given and the distance that puts point on it.
inline void SetPlaneThrough(Plane &plane, const Vector3 &normal, const Vector3 &point) {
    plane.a = normal.x;
    plane.b = normal.y;
    plane.c = normal.z;
    plane.d = -(((normal.x * point.x) + (normal.y * point.y)) + (normal.z * point.z));
}

} // namespace

Frustum &Frustum::Set(float flNear, float flFar, float flFov, float flAspect) {
    SetPlaneThrough(mFront, Vector3{0.0f, 1.0f, 0.0f, 1.0f}, Vector3{0.0f, flNear, 0.0f, 1.0f});
    SetPlaneThrough(mBack, Vector3{0.0f, -1.0f, 0.0f, 1.0f}, Vector3{0.0f, flFar, 0.0f, 1.0f});

    const float flHalfFov = flFov * 0.5f;
    const float flCos = cosf(flHalfFov);
    const float flSin = sinf(flHalfFov);
    SetPlaneThrough(mLeft, Vector3{flCos, flSin, 0.0f, 1.0f}, Vector3{-1.0f, 0.0f, 0.0f, 1.0f});
    SetPlaneThrough(mRight, Vector3{-flCos, flSin, 0.0f, 1.0f}, Vector3{1.0f, 0.0f, 0.0f, 1.0f});

    const float flRise = flSin * flAspect;
    Vector3 normal{0.0f, flRise, -flCos, 1.0f};
    Vec3Normalize(&normal.x, &normal.x);
    SetPlaneThrough(mTop, normal, Vector3{0.0f, 0.0f, flAspect, 1.0f});
    normal = Vector3{0.0f, flRise, flCos, 1.0f};
    Vec3Normalize(&normal.x, &normal.x);
    SetPlaneThrough(mBottom, normal, Vector3{0.0f, 0.0f, -flAspect, 1.0f});

    if (flFov != 0.0f) {
        mLeft.d = 0.0f;
        mBottom.d = 0.0f;
        mTop.d = 0.0f;
        mRight.d = 0.0f;
    }
    return *this;
}

Rnd::Dbg &operator<<(Rnd::Dbg &sink, const Frustum &frustum) {
    PrintPlane(sink, "\n\tfront:", frustum.mFront);
    PrintPlane(sink, "\n\tback:", frustum.mBack);
    PrintPlane(sink, "\n\tleft:", frustum.mLeft);
    PrintPlane(sink, "\n\tright:", frustum.mRight);
    PrintPlane(sink, "\n\ttop:", frustum.mTop);
    PrintPlane(sink, "\n\tbottom:", frustum.mBottom);
    return sink;
}

int IsSphereOutsideFrustum(const Sphere &sphere, const Frustum &frustum) {
    const Plane *const apPlanes[] = {&frustum.mFront,
                                     &frustum.mBack,
                                     &frustum.mLeft,
                                     &frustum.mRight,
                                     &frustum.mTop,
                                     &frustum.mBottom};
    const Vector3 &center = sphere.mCenter;
    int nStatus = 0;
    for (const Plane *pPlane : apPlanes) {
        const float flDistance =
            (((pPlane->a * center.x) + (pPlane->b * center.y)) + (pPlane->c * center.z)) +
            pPlane->d;
        if (flDistance + sphere.mRadius < 0.0f) {
            nStatus = kVu0StatusStickySign;
        }
    }
    return nStatus;
}
