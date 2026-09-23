#include "math/plane.h"

namespace {

// Every temporary vector below is constructed, and construction sets the padding word to 1.0.
inline Vector3 ConstructVector3() {
    Vector3 vector{};
    vector.w = 1.0f;
    return vector;
}

// The cross product is a VU0 outer-product pair in the image, vopmula followed by vopmsub, rather
// than a call.
inline void Vec3Cross(const float *pLeft, const float *pRight, float *pOut) {
    const float flX = pLeft[1] * pRight[2] - pLeft[2] * pRight[1];
    const float flY = pLeft[2] * pRight[0] - pLeft[0] * pRight[2];
    const float flZ = pLeft[0] * pRight[1] - pLeft[1] * pRight[0];
    pOut[0] = flX;
    pOut[1] = flY;
    pOut[2] = flZ;
}

// The point a segment test places at its fraction along a segment.
inline Vector3 PointAlongSegment(const Segment &segment, float flT) {
    Vector3 point = ConstructVector3();
    Vec3Sub(&segment.mEnds[1].x, &segment.mEnds[0].x, &point.x);
    Vec3Scale(&point.x, flT, &point.x);
    AddVec3(&segment.mEnds[0].x, &point.x, &point.x);
    return point;
}

} // namespace

Plane TransformPlaneToWorld(const Plane &plane, const float *pXfm) {
    Plane result;
    result.a = (pXfm[0] * plane.a) + (pXfm[4] * plane.b) + (pXfm[8] * plane.c);
    result.b = (pXfm[1] * plane.a) + (pXfm[5] * plane.b) + (pXfm[9] * plane.c);
    result.c = (pXfm[2] * plane.a) + (pXfm[6] * plane.b) + (pXfm[10] * plane.c);
    result.d = plane.d - ((pXfm[12] * result.a) + (pXfm[13] * result.b) + (pXfm[14] * result.c));
    return result;
}

void InterpolateFourFloats(const float *pFrom, const float *pTo, float *pOut, float flT) {
    constexpr int kValueCount = 4;
    for (int i = 0; i < kValueCount; ++i) {
        pOut[i] = ((pTo[i] - pFrom[i]) * flT) + pFrom[i];
    }
}

bool IntersectSegmentWithPlane(const Vector3 segment[2], const Plane &plane, float *pT) {
    const auto &start = segment[0];
    const auto &end = segment[1];
    float flStart = (plane.a * start.x) + (plane.b * start.y) + (plane.c * start.z) + plane.d;
    float flEnd = (plane.a * end.x) + (plane.b * end.y) + (plane.c * end.z) + plane.d;
    *pT = flStart / (flStart - flEnd);
    return (0.0f <= *pT) && (*pT <= 1.0f);
}

// 0x0054fcf8
Segment IntersectPlanes(const Plane &first, const Plane &second) {
    Vector3 direction = ConstructVector3();
    Vec3Cross(&first.a, &second.a, &direction.x);

    Segment probe;
    probe.mEnds[0] = ConstructVector3();
    probe.mEnds[0].x = -first.d * first.a;
    probe.mEnds[0].y = -first.d * first.b;
    probe.mEnds[0].z = -first.d * first.c;
    probe.mEnds[1] = ConstructVector3();
    Vec3Cross(&direction.x, &first.a, &probe.mEnds[1].x);

    float flT;
    IntersectSegmentWithPlane(probe.mEnds, second, &flT); // Yes, the binary discards the result.

    Segment line;
    line.mEnds[0] = PointAlongSegment(probe, flT);
    line.mEnds[1] = ConstructVector3();
    AddVec3(&line.mEnds[0].x, &direction.x, &line.mEnds[1].x);
    return line;
}

// 0x005510e0
Vector3 IntersectPlanes(const Plane &first, const Plane &second, const Plane &third) {
    const Segment line = IntersectPlanes(first, second);
    float flT;
    IntersectSegmentWithPlane(line.mEnds, third, &flT); // Yes, the binary discards the result.
    return PointAlongSegment(line, flT);
}
