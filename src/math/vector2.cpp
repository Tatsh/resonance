#include "math/vector2.h"

void AddVec2(const float *pA, const float *pB, float *pOut) {
    pOut[1] = pA[1] + pB[1];
    pOut[0] = pA[0] + pB[0];
}

void SubVec2(const float *pA, const float *pB, float *pOut) {
    pOut[1] = pA[1] - pB[1];
    pOut[0] = pA[0] - pB[0];
}
