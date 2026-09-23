#include "math/vector2.h"

#include <math.h>

// 0x00169818
void AddVec2(const float *pA, const float *pB, float *pOut) {
    pOut[1] = pA[1] + pB[1];
    pOut[0] = pA[0] + pB[0];
}

// 0x004bec40
void SubVec2(const float *pA, const float *pB, float *pOut) {
    pOut[1] = pA[1] - pB[1];
    pOut[0] = pA[0] - pB[0];
}

// 0x00169840
void ScaleVec2(const float *pSrc, float flScale, float *pOut) {
    pOut[1] = pSrc[1] * flScale;
    pOut[0] = pSrc[0] * flScale;
}

// 0x004bec88
void NegateVec2(const float *pSrc, float *pOut) {
    pOut[1] = -pSrc[1];
    pOut[0] = -pSrc[0];
}

// 0x004beca8
void NormalizeVec2(const float *pSrc, float *pOut) {
    if (pSrc[0] == 0.0f && pSrc[1] == 0.0f) {
        pOut[1] = 0.0f;
        pOut[0] = 0.0f;
        return;
    }
    const float flInvLength = 1.0f / Vec2Length(pSrc);
    pOut[1] = pSrc[1] * flInvLength;
    pOut[0] = pSrc[0] * flInvLength;
}

// 0x004bfcc0
float Vec2Length(const float *pSrc) {
    return sqrtf(pSrc[0] * pSrc[0] + pSrc[1] * pSrc[1]);
}

// 0x00551190
Vector2 IntersectLines(const Vector2 first[2], const Vector2 second[2]) {
    const Vector2 &firstPoint = first[0];
    const Vector2 &firstDirection = first[1];
    const Vector2 &secondPoint = second[0];
    const Vector2 &secondDirection = second[1];

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
