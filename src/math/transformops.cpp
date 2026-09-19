#include "math/transformops.h"

#include <math.h>

#include "math/vector3.h"

namespace {

// A rotation matrix is three rows of four floats and an affine transform is four. A row index
// scales by this stride in both cases.
constexpr int kMatRowStride = 4;

// A quarter turn. The decomposition reports this as the X angle at the gimbal lock limit.
constexpr float kQuarterTurn = 1.570796251f;

// The X row component above which the Y and Z angles cannot be separated.
constexpr float kGimbalLockLimit = 0.9999998808f;

} // namespace

void EulerAnglesToMatrix3x3(const float *pAngles, float *pMat3Rows) {
    const float flSinZ = sinf(pAngles[2]);
    const float flCosZ = cosf(pAngles[2]);
    const float flSinY = sinf(pAngles[1]);
    const float flCosY = cosf(pAngles[1]);
    const float flSinX = sinf(pAngles[0]);
    const float flCosX = cosf(pAngles[0]);

    pMat3Rows[0] = (flCosY * flCosZ) - ((flSinY * flSinZ) * flSinX);
    pMat3Rows[1] = (flCosY * flSinZ) + ((flCosZ * flSinY) * flSinX);
    pMat3Rows[2] = -flSinY * flCosX;

    pMat3Rows[4] = -flCosX * flSinZ;
    pMat3Rows[5] = flCosX * flCosZ;
    pMat3Rows[6] = flSinX;

    pMat3Rows[8] = (flCosZ * flSinY) + ((flCosY * flSinZ) * flSinX);
    pMat3Rows[9] = (flSinY * flSinZ) - ((flCosY * flCosZ) * flSinX);
    pMat3Rows[10] = flCosY * flCosX;
}

void Mat33BuildOrthonormal(const float *pAxisY, const float *pReference, float *pMat3Rows) {
    pMat3Rows[4] = pAxisY[0];
    pMat3Rows[5] = pAxisY[1];
    pMat3Rows[6] = pAxisY[2];
    pMat3Rows[7] = pAxisY[3];
    Vec3Normalize(&pMat3Rows[4], &pMat3Rows[4]);

    // The cross products are an inline vopmula and vopmsub pair rather than a call.
    Vector3 axisX;
    axisX.x = (pMat3Rows[5] * pReference[2]) - (pMat3Rows[6] * pReference[1]);
    axisX.y = (pMat3Rows[6] * pReference[0]) - (pMat3Rows[4] * pReference[2]);
    axisX.z = (pMat3Rows[4] * pReference[1]) - (pMat3Rows[5] * pReference[0]);
    axisX.w = pMat3Rows[7];
    Vec3Normalize(&axisX.x, &axisX.x);

    pMat3Rows[0] = axisX.x;
    pMat3Rows[1] = axisX.y;
    pMat3Rows[2] = axisX.z;
    pMat3Rows[3] = axisX.w;

    pMat3Rows[8] = (pMat3Rows[1] * pMat3Rows[6]) - (pMat3Rows[2] * pMat3Rows[5]);
    pMat3Rows[9] = (pMat3Rows[2] * pMat3Rows[4]) - (pMat3Rows[0] * pMat3Rows[6]);
    pMat3Rows[10] = (pMat3Rows[0] * pMat3Rows[5]) - (pMat3Rows[1] * pMat3Rows[4]);
    pMat3Rows[11] = pMat3Rows[3];
}

void Mat34DecomposeEulerScale(const float *pMat3Rows, float *pAngles, float *pScale) {
    const float flLenZ = sqrtf((pMat3Rows[8] * pMat3Rows[8]) + (pMat3Rows[9] * pMat3Rows[9]) +
                               (pMat3Rows[10] * pMat3Rows[10]));
    const float flLenX = sqrtf((pMat3Rows[0] * pMat3Rows[0]) + (pMat3Rows[1] * pMat3Rows[1]) +
                               (pMat3Rows[2] * pMat3Rows[2]));
    const float flLenY = sqrtf((pMat3Rows[4] * pMat3Rows[4]) + (pMat3Rows[5] * pMat3Rows[5]) +
                               (pMat3Rows[6] * pMat3Rows[6]));

    Vector3 cross;
    cross.x = (pMat3Rows[1] * pMat3Rows[6]) - (pMat3Rows[2] * pMat3Rows[5]);
    cross.y = (pMat3Rows[2] * pMat3Rows[4]) - (pMat3Rows[0] * pMat3Rows[6]);
    cross.z = (pMat3Rows[0] * pMat3Rows[5]) - (pMat3Rows[1] * pMat3Rows[4]);

    const float flHandedness =
        (cross.x * pMat3Rows[8]) + (cross.y * pMat3Rows[9]) + (cross.z * pMat3Rows[10]);

    pScale[0] = flLenX;
    pScale[1] = flLenY;
    pScale[2] = (flHandedness > 0.0f) ? flLenZ : -flLenZ;

    // The image divides by the first length still in a register and reads the other two back out
    // of the destination.
    Vector3 aRot[3];
    Vec3Scale(&pMat3Rows[0], 1.0f / flLenX, &aRot[0].x);
    Vec3Scale(&pMat3Rows[4], 1.0f / pScale[1], &aRot[1].x);
    Vec3Scale(&pMat3Rows[8], 1.0f / pScale[2], &aRot[2].x);

    if (fabsf(aRot[1].z) > kGimbalLockLimit) {
        pAngles[0] = (aRot[1].z > 0.0f) ? kQuarterTurn : -kQuarterTurn;
        pAngles[2] = atan2f(aRot[0].y, aRot[0].x);
        pAngles[1] = 0.0f;
        return;
    }

    pAngles[2] = atan2f(-aRot[1].x, aRot[1].y);
    pAngles[0] = asinf(aRot[1].z);
    pAngles[1] = atan2f(-aRot[0].z, aRot[2].z);
}

void Mat44Multiply(float *pDst, const float *pA, const float *pB) {
    // The image loads the whole left factor into vector registers before the first store. That is
    // what permits the destination to alias it.
    const float flA00 = pA[0];
    const float flA01 = pA[1];
    const float flA02 = pA[2];
    const float flA10 = pA[4];
    const float flA11 = pA[5];
    const float flA12 = pA[6];
    const float flA20 = pA[8];
    const float flA21 = pA[9];
    const float flA22 = pA[10];

    for (int nRow = 0; nRow < 3; ++nRow) {
        const int nBase = nRow * kMatRowStride;
        const float flX = pB[nBase];
        const float flY = pB[nBase + 1];
        const float flZ = pB[nBase + 2];

        pDst[nBase] = (flA00 * flX) + (flA10 * flY) + (flA20 * flZ);
        pDst[nBase + 1] = (flA01 * flX) + (flA11 * flY) + (flA21 * flZ);
        pDst[nBase + 2] = (flA02 * flX) + (flA12 * flY) + (flA22 * flZ);
    }

    const float flX = pB[12];
    const float flY = pB[13];
    const float flZ = pB[14];

    pDst[12] = (flA00 * flX) + (flA10 * flY) + (flA20 * flZ) + pA[12];
    pDst[13] = (flA01 * flX) + (flA11 * flY) + (flA21 * flZ) + pA[13];
    pDst[14] = (flA02 * flX) + (flA12 * flY) + (flA22 * flZ) + pA[14];
}

void MultiplyMat3VU0(const float *pMatA, const float *pMatB, float *pOut) {
    if (pMatB == pOut) {
        // Each product row lands in a scratch quadword before the three are copied back over the
        // right factor. The 1.0 the image writes into each scratch fourth word is overwritten by
        // the transform below.
        Vector3 aScratch[3];
        TransformVec3ByMat3VU0(&pMatA[0], pMatB, &aScratch[0].x);
        TransformVec3ByMat3VU0(&pMatA[4], pMatB, &aScratch[1].x);
        TransformVec3ByMat3VU0(&pMatA[8], pMatB, &aScratch[2].x);

        for (int nRow = 0; nRow < 3; ++nRow) {
            const int nBase = nRow * kMatRowStride;
            pOut[nBase] = aScratch[nRow].x;
            pOut[nBase + 1] = aScratch[nRow].y;
            pOut[nBase + 2] = aScratch[nRow].z;
            pOut[nBase + 3] = aScratch[nRow].w;
        }
        return;
    }

    for (int nRow = 0; nRow < 3; ++nRow) {
        const int nBase = nRow * kMatRowStride;
        const float flX = pMatA[nBase];
        const float flY = pMatA[nBase + 1];
        const float flZ = pMatA[nBase + 2];

        pOut[nBase] = (pMatB[0] * flX) + (pMatB[4] * flY) + (pMatB[8] * flZ);
        pOut[nBase + 1] = (pMatB[1] * flX) + (pMatB[5] * flY) + (pMatB[9] * flZ);
        pOut[nBase + 2] = (pMatB[2] * flX) + (pMatB[6] * flY) + (pMatB[10] * flZ);
        pOut[nBase + 3] = pMatA[nBase + 3];
    }
}

void ScaleRows3x3(const float *pScale, const float *pMat3Rows, float *pOut) {
    pOut[0] = pMat3Rows[0] * pScale[0];
    pOut[1] = pMat3Rows[1] * pScale[0];
    pOut[2] = pMat3Rows[2] * pScale[0];

    pOut[4] = pMat3Rows[4] * pScale[1];
    pOut[5] = pMat3Rows[5] * pScale[1];
    pOut[6] = pMat3Rows[6] * pScale[1];

    pOut[8] = pMat3Rows[8] * pScale[2];
    pOut[9] = pMat3Rows[9] * pScale[2];
    pOut[10] = pMat3Rows[10] * pScale[2];
}

void TransformVec3ByMat3VU0(const float *pVec, const float *pMat3Rows, float *pOut) {
    const float flX = pVec[0];
    const float flY = pVec[1];
    const float flZ = pVec[2];
    const float flW = pVec[3];

    pOut[0] = (pMat3Rows[0] * flX) + (pMat3Rows[4] * flY) + (pMat3Rows[8] * flZ);
    pOut[1] = (pMat3Rows[1] * flX) + (pMat3Rows[5] * flY) + (pMat3Rows[9] * flZ);
    pOut[2] = (pMat3Rows[2] * flX) + (pMat3Rows[6] * flY) + (pMat3Rows[10] * flZ);
    pOut[3] = flW;
}

void XfmConcat(const float *pA, const float *pB, float *pOut) {
    Mat44Multiply(pOut, pB, pA);
}

void XfmInvertRigid(float *pDst, const float *pSrc) {
    const float flBasisXx = pSrc[0];
    const float flBasisXy = pSrc[1];
    const float flBasisXz = pSrc[2];
    const float flBasisYx = pSrc[4];
    const float flBasisYy = pSrc[5];
    const float flBasisYz = pSrc[6];
    const float flBasisZx = pSrc[8];
    const float flBasisZy = pSrc[9];
    const float flBasisZz = pSrc[10];
    const float flTransX = pSrc[12];
    const float flTransY = pSrc[13];
    const float flTransZ = pSrc[14];
    const float flTransW = pSrc[15];

    // The parallel pack instructions transpose the three basis rows and zero the fourth word of
    // each.
    pDst[0] = flBasisXx;
    pDst[1] = flBasisYx;
    pDst[2] = flBasisZx;
    pDst[3] = 0.0f;

    pDst[4] = flBasisXy;
    pDst[5] = flBasisYy;
    pDst[6] = flBasisZy;
    pDst[7] = 0.0f;

    pDst[8] = flBasisXz;
    pDst[9] = flBasisYz;
    pDst[10] = flBasisZz;
    pDst[11] = 0.0f;

    pDst[12] = -((flBasisXx * flTransX) + (flBasisXy * flTransY) + (flBasisXz * flTransZ));
    pDst[13] = -((flBasisYx * flTransX) + (flBasisYy * flTransY) + (flBasisYz * flTransZ));
    pDst[14] = -((flBasisZx * flTransX) + (flBasisZy * flTransY) + (flBasisZz * flTransZ));
    pDst[15] = flTransW;
}

void Mat44Concat(float *pDst, const float *pA, const float *pB) {
    // The image loads the whole left factor into vector registers before the first store, which is
    // what permits the destination to alias it.
    const float flA00 = pA[0];
    const float flA01 = pA[1];
    const float flA02 = pA[2];
    const float flA03 = pA[3];
    const float flA10 = pA[4];
    const float flA11 = pA[5];
    const float flA12 = pA[6];
    const float flA13 = pA[7];
    const float flA20 = pA[8];
    const float flA21 = pA[9];
    const float flA22 = pA[10];
    const float flA23 = pA[11];

    for (int nRow = 0; nRow < 3; ++nRow) {
        const int nBase = nRow * kMatRowStride;
        const float flX = pB[nBase];
        const float flY = pB[nBase + 1];
        const float flZ = pB[nBase + 2];

        pDst[nBase] = (flA00 * flX) + (flA10 * flY) + (flA20 * flZ);
        pDst[nBase + 1] = (flA01 * flX) + (flA11 * flY) + (flA21 * flZ);
        pDst[nBase + 2] = (flA02 * flX) + (flA12 * flY) + (flA22 * flZ);
        pDst[nBase + 3] = (flA03 * flX) + (flA13 * flY) + (flA23 * flZ);
    }

    const float flX = pB[12];
    const float flY = pB[13];
    const float flZ = pB[14];

    pDst[12] = (flA00 * flX) + (flA10 * flY) + (flA20 * flZ) + pA[12];
    pDst[13] = (flA01 * flX) + (flA11 * flY) + (flA21 * flZ) + pA[13];
    pDst[14] = (flA02 * flX) + (flA12 * flY) + (flA22 * flZ) + pA[14];
    pDst[15] = (flA03 * flX) + (flA13 * flY) + (flA23 * flZ) + pA[15];
}
