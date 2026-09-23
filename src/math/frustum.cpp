#include "math/frustum.h"

#include "math/sphere.h"
#include "os/failsink.h"

namespace {

// One plane under its title, the way the frustum printer writes each of the six.
void PrintPlane(FailSink &sink, const char *pszTitle, const Plane &plane) {
    sink.Print(pszTitle);
    sink.Print("(a:");
    sink.Format("%.2f", plane.a);
    sink.Print(" b:");
    sink.Format("%.2f", plane.b);
    sink.Print(" c:");
    sink.Format("%.2f", plane.c);
    sink.Print(" d:");
    sink.Format("%.2f", plane.d);
    sink.Print(")");
}

} // namespace

FailSink &operator<<(FailSink &sink, const Frustum &frustum) {
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
