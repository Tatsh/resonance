#include "math/vector2.h"

#include <math.h>

void AddVec2(const float *pA, const float *pB, float *pOut) {
    pOut[1] = pA[1] + pB[1];
    pOut[0] = pA[0] + pB[0];
}

void SubVec2(const float *pA, const float *pB, float *pOut) {
    pOut[1] = pA[1] - pB[1];
    pOut[0] = pA[0] - pB[0];
}

void ScaleVec2(const float *pSrc, float flScale, float *pOut) {
    pOut[1] = pSrc[1] * flScale;
    pOut[0] = pSrc[0] * flScale;
}

void NegateVec2(const float *pSrc, float *pOut) {
    pOut[1] = -pSrc[1];
    pOut[0] = -pSrc[0];
}

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

float Vec2Length(const float *pSrc) {
    return sqrtf(pSrc[0] * pSrc[0] + pSrc[1] * pSrc[1]);
}
