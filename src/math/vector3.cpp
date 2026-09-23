#include "math/vector3.h"

#include <math.h>

// 0x0028c218
void AddVec3(const float *pA, const float *pB, float *pOut) {
    pOut[0] = pA[0] + pB[0];
    pOut[1] = pA[1] + pB[1];
    pOut[2] = pA[2] + pB[2];
}

// 0x00317160
void Vec3Sub(const float *pA, const float *pB, float *pOut) {
    pOut[0] = pA[0] - pB[0];
    pOut[1] = pA[1] - pB[1];
    pOut[2] = pA[2] - pB[2];
}

// 0x0024ecd0
void Vec3Scale(const float *pSrc, float flScale, float *pOut) {
    pOut[0] = pSrc[0] * flScale;
    pOut[1] = pSrc[1] * flScale;
    pOut[2] = pSrc[2] * flScale;
}

// 0x00492458
void NegateVec3(const float *pSrc, float *pOut) {
    pOut[0] = -pSrc[0];
    pOut[1] = -pSrc[1];
    pOut[2] = -pSrc[2];
}

// 0x00476140
void Vec3Normalize(const float *pSrc, float *pOut) {
    // The image reads the source as one quadword, divides 1.0 by the square root of the dot of the
    // first three components with the vrsqrt instruction, and stores the whole quadword back.
    const float flScale =
        1.0f / sqrtf((pSrc[0] * pSrc[0]) + (pSrc[1] * pSrc[1]) + (pSrc[2] * pSrc[2]));

    pOut[0] = pSrc[0] * flScale;
    pOut[1] = pSrc[1] * flScale;
    pOut[2] = pSrc[2] * flScale;
    pOut[3] = pSrc[3];
}
