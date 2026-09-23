#include "rnd/raytest.h"

#include "math/sphere.h"
#include "math/vector3.h"
#include "rnd/collideable.h"
#include "rnd/mat.h"

namespace Rnd {

namespace {

// Lanes of a quadword vector. Every temporary the tests build carries 1 in the padding lane.
enum { kLaneX = 0, kLaneY = 1, kLaneZ = 2, kLaneW = 3, kLaneCount = 4 };

inline float Dot3(const float *pA, const float *pB) {
    return pA[kLaneX] * pB[kLaneX] + pA[kLaneY] * pB[kLaneY] + pA[kLaneZ] * pB[kLaneZ];
}

// The ray's direction, its far end less its origin.
inline void RayDirection(const Ray &ray, float *pDir) {
    pDir[kLaneW] = 1.0f;
    Vec3Sub(ray.mEnd, ray.mStart, pDir);
}

// The point a fraction flT of the way along the ray.
inline void PointAlongRay(const Ray &ray, const float *pDir, float flT, float *pPoint) {
    float step[kLaneCount];
    step[kLaneW] = 1.0f;
    Vec3Scale(pDir, flT, step);
    pPoint[kLaneW] = 1.0f;
    AddVec3(ray.mStart, step, pPoint);
}

} // namespace

// 0x0054fe98
int TestRayAgainstTriangle(const Ray &ray, const TriangleTest &tri, int nCull, float *pflDistance) {
    if (nCull != Mat::kCullModeNone) {
        float dir[kLaneCount];
        RayDirection(ray, dir);
        const float flFacing = Dot3(tri.mNormal, dir);
        if (nCull == Mat::kCullModeCw) {
            if (0.0f < flFacing) {
                return 0;
            }
        } else if (flFacing < 0.0f) {
            return 0;
        }
    }

    float dir[kLaneCount];
    RayDirection(ray, dir);
    float toVertex[kLaneCount];
    toVertex[kLaneW] = 1.0f;
    Vec3Sub(tri.mVertex, ray.mStart, toVertex);

    const float flT = Dot3(toVertex, tri.mNormal) / Dot3(dir, tri.mNormal);
    *pflDistance = flT;
    if (flT < 0.0f || 1.0f < flT) {
        return 0;
    }

    float hit[kLaneCount];
    PointAlongRay(ray, dir, flT, hit);
    float local[kLaneCount];
    local[kLaneW] = 1.0f;
    Vec3Sub(hit, tri.mVertex, local);

    // Solve in the first coordinate plane where the two edges are not parallel, trying XY, then
    // XZ, then YZ.
    int nAxisA = kLaneX;
    int nAxisB = kLaneY;
    float flDenominator =
        tri.mEdge2[nAxisA] * tri.mEdge1[nAxisB] - tri.mEdge1[nAxisA] * tri.mEdge2[nAxisB];
    if (flDenominator == 0.0f) {
        nAxisB = kLaneZ;
        flDenominator =
            tri.mEdge2[nAxisA] * tri.mEdge1[nAxisB] - tri.mEdge1[nAxisA] * tri.mEdge2[nAxisB];
        if (flDenominator == 0.0f) {
            nAxisA = kLaneY;
            flDenominator =
                tri.mEdge2[nAxisA] * tri.mEdge1[nAxisB] - tri.mEdge1[nAxisA] * tri.mEdge2[nAxisB];
        }
    }

    const float flV =
        (local[nAxisA] * tri.mEdge1[nAxisB] - tri.mEdge1[nAxisA] * local[nAxisB]) / flDenominator;
    if (flV < 0.0f || 1.0f < flV) {
        return 0;
    }

    float flU;
    if (tri.mEdge1[nAxisA] == 0.0f) {
        flU = (local[nAxisB] - tri.mEdge2[nAxisB] * flV) / tri.mEdge1[nAxisB];
    } else {
        flU = (local[nAxisA] - tri.mEdge2[nAxisA] * flV) / tri.mEdge1[nAxisA];
    }
    if (flU < 0.0f) {
        return 0;
    }
    return !(1.0f < flU + flV);
}

// 0x005501b0
int TestRayAgainstSphere(const Ray &ray, const Sphere &sphere, float *pflDistance) {
    float dir[kLaneCount];
    RayDirection(ray, dir);
    float toCenter[kLaneCount];
    toCenter[kLaneW] = 1.0f;
    Vec3Sub(&sphere.mCenter.x, ray.mStart, toCenter);

    const float flT = Dot3(dir, toCenter) / Dot3(dir, dir);
    *pflDistance = flT;

    float closest[kLaneCount];
    PointAlongRay(ray, dir, flT, closest);
    float offset[kLaneCount];
    offset[kLaneW] = 1.0f;
    Vec3Sub(closest, &sphere.mCenter.x, offset);

    return !(sphere.mRadius * sphere.mRadius < Dot3(offset, offset));
}

} // namespace Rnd
