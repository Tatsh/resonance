#include "math/sphere.h"

#include <math.h>

#include "math/plane.h"

namespace {

constexpr float kHalf = 0.5f;

// Every temporary vector below is constructed, and construction sets the padding word to 1.0.
inline Vector3 ConstructVector3() {
    Vector3 vector{};
    vector.w = 1.0f;
    return vector;
}

// The length is a VU0 dot product and square root in the image rather than a call.
inline float Vec3Length(const Vector3 &vector) {
    return sqrtf((vector.x * vector.x) + (vector.y * vector.y) + (vector.z * vector.z));
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

// The plane with the given normal through the given point.
inline Plane PlaneThrough(const Vector3 &normal, const Vector3 &point) {
    Plane plane;
    plane.a = normal.x;
    plane.b = normal.y;
    plane.c = normal.z;
    plane.d = -((normal.x * point.x) + (normal.y * point.y) + (normal.z * point.z));
    return plane;
}

// The plane of the points as far from `from` as from `to`.
inline Plane BisectorPlane(const Vector3 &from, const Vector3 &to) {
    Vector3 midpoint = ConstructVector3();
    AddVec3(&from.x, &to.x, &midpoint.x);
    Vec3Scale(&midpoint.x, kHalf, &midpoint.x);
    Vector3 normal = ConstructVector3();
    Vec3Sub(&to.x, &from.x, &normal.x);
    return PlaneThrough(normal, midpoint);
}

// The sphere centred where three planes meet, through the given point.
inline Sphere SphereAtPlanes(const Plane &first,
                             const Plane &second,
                             const Plane &third,
                             const Vector3 &surfacePoint) {
    Sphere sphere;
    sphere.mCenter = IntersectPlanes(first, second, third);
    Vector3 radius = ConstructVector3();
    Vec3Sub(&surfacePoint.x, &sphere.mCenter.x, &radius.x);
    sphere.mRadius = Vec3Length(radius);
    return sphere;
}

} // namespace

// 0x00550330
Sphere &Sphere::GrowToContain(const Sphere &other) {
    if (other.mRadius == 0.0f) {
        return *this;
    }
    Vector3 direction = ConstructVector3();
    Vec3Sub(&other.mCenter.x, &mCenter.x, &direction.x);
    const float flDistance = Vec3Length(direction);
    if (flDistance + other.mRadius <= mRadius) {
        return *this;
    }
    if (flDistance + mRadius < other.mRadius) {
        mCenter = other.mCenter;
        mRadius = other.mRadius;
        return *this;
    }
    // Yes, the binary tests this even though the two tests above have already caught it.
    if (flDistance == 0.0f) {
        return *this;
    }

    Vec3Scale(&direction.x, 1.0f / flDistance, &direction.x);
    Vector3 nearEnd = ConstructVector3();
    Vec3Scale(&direction.x, mRadius, &nearEnd.x);
    Vec3Sub(&mCenter.x, &nearEnd.x, &nearEnd.x);
    Vector3 farEnd = ConstructVector3();
    Vec3Scale(&direction.x, other.mRadius, &farEnd.x);
    AddVec3(&other.mCenter.x, &farEnd.x, &farEnd.x);

    mCenter.x = (farEnd.x * kHalf) + (nearEnd.x * kHalf);
    mCenter.y = (farEnd.y * kHalf) + (nearEnd.y * kHalf);
    mCenter.z = (farEnd.z * kHalf) + (nearEnd.z * kHalf);
    mCenter.w = farEnd.w;
    mRadius = (mRadius + flDistance + other.mRadius) * kHalf;
    return *this;
}

// 0x005512b8
Sphere Sphere::Circumscribe(const Vector3 &first, const Vector3 &second) {
    Sphere sphere;
    sphere.mCenter = ConstructVector3();
    AddVec3(&first.x, &second.x, &sphere.mCenter.x);
    Vec3Scale(&sphere.mCenter.x, kHalf, &sphere.mCenter.x);
    Vector3 diameter = ConstructVector3();
    Vec3Sub(&first.x, &second.x, &diameter.x);
    sphere.mRadius = Vec3Length(diameter) * kHalf;
    return sphere;
}

// 0x00550510
Sphere Sphere::Circumscribe(const Vector3 &first, const Vector3 &second, const Vector3 &third) {
    Vector3 firstEdge = ConstructVector3();
    Vec3Sub(&second.x, &first.x, &firstEdge.x);
    Vector3 secondEdge = ConstructVector3();
    Vec3Sub(&third.x, &first.x, &secondEdge.x);
    Vector3 normal = ConstructVector3();
    Vec3Cross(&firstEdge.x, &secondEdge.x, &normal.x);

    const Plane triangle = PlaneThrough(normal, first);
    const Plane bisector = BisectorPlane(first, second);
    // Yes, the binary bisects the first two points again here rather than the first and third.
    const Plane repeatedBisector = BisectorPlane(first, second);
    return SphereAtPlanes(triangle, bisector, repeatedBisector, first);
}

// 0x00550850
Sphere Sphere::Circumscribe(const Vector3 &first,
                            const Vector3 &second,
                            const Vector3 &third,
                            const Vector3 &fourth) {
    const Plane secondBisector = BisectorPlane(first, second);
    const Plane thirdBisector = BisectorPlane(first, third);
    const Plane fourthBisector = BisectorPlane(first, fourth);
    return SphereAtPlanes(secondBisector, thirdBisector, fourthBisector, first);
}
