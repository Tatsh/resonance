#include "math/vector2.h"

#include <math.h>

#include "math/ray.h"

void Rnd::Add(const Vector2 &a, const Vector2 &b, Vector2 &out) {
    out.y = a.y + b.y;
    out.x = a.x + b.x;
}

void SubVec2(const float *pA, const float *pB, float *pOut) {
    pOut[1] = pA[1] - pB[1];
    pOut[0] = pA[0] - pB[0];
}

void Rnd::Multiply(const Vector2 &v, float flScale, Vector2 &out) {
    out.y = v.y * flScale;
    out.x = v.x * flScale;
}

void Rnd::Negate(const Vector2 &v, Vector2 &out) {
    out.y = -v.y;
    out.x = -v.x;
}

void Rnd::Normalize(const Vector2 &v, Vector2 &out) {
    if (v.x == 0.0f && v.y == 0.0f) {
        out.y = 0.0f;
        out.x = 0.0f;
        return;
    }
    const float flInvLength = 1.0f / Length(v);
    out.y = v.y * flInvLength;
    out.x = v.x * flInvLength;
}

float Rnd::Length(const Vector2 &v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

Vector2 Rnd::Intersect(const Ray &first, const Ray &second) {
    const Vector2 &firstPoint = first.mPoint;
    const Vector2 &firstDirection = first.mDirection;
    const Vector2 &secondPoint = second.mPoint;
    const Vector2 &secondDirection = second.mDirection;

    const float flDenominator =
        (secondDirection.x * firstDirection.y) - (firstDirection.x * secondDirection.y);
    if (flDenominator == 0.0f) {
        return firstPoint;
    }
    const float flT = ((firstDirection.y * (firstPoint.x - secondPoint.x)) +
                       (firstDirection.x * (secondPoint.y - firstPoint.y))) /
                      flDenominator;
    Vector2 crossing;
    crossing.y = secondPoint.y + (flT * secondDirection.y);
    crossing.x = secondPoint.x + (flT * secondDirection.x);
    return crossing;
}
