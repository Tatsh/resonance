#include <math.h>

enum {
    kMatRowStride = 4,
};

void sceVu0MulAffineMatrix(float *pDst, const float *pA, const float *pB) {
    // The image pins the rows of pA in vector registers and streams the rows of pB, so either
    // factor may alias the destination. The copy below preserves that behaviour.
    float aflBasis[16];
    for (int nWord = 0; nWord < 16; ++nWord) {
        aflBasis[nWord] = pA[nWord];
    }

    for (int nRow = 0; nRow < 3; ++nRow) {
        const int nDst = nRow * kMatRowStride;
        const float flX = pB[nDst];
        const float flY = pB[nDst + 1];
        const float flZ = pB[nDst + 2];

        // The first three rows combine the first three rows of pA only.
        const float flOutX = (aflBasis[0] * flX) + (aflBasis[4] * flY) + (aflBasis[8] * flZ);
        const float flOutY = (aflBasis[1] * flX) + (aflBasis[5] * flY) + (aflBasis[9] * flZ);
        const float flOutZ = (aflBasis[2] * flX) + (aflBasis[6] * flY) + (aflBasis[10] * flZ);
        const float flOutW = (aflBasis[3] * flX) + (aflBasis[7] * flY) + (aflBasis[11] * flZ);

        pDst[nDst] = flOutX;
        pDst[nDst + 1] = flOutY;
        pDst[nDst + 2] = flOutZ;
        pDst[nDst + 3] = flOutW;
    }

    // The final row also adds the fourth row of pA across all four words.
    const float flX = pB[12];
    const float flY = pB[13];
    const float flZ = pB[14];
    const float flOutX = (aflBasis[0] * flX) + (aflBasis[4] * flY) + (aflBasis[8] * flZ);
    const float flOutY = (aflBasis[1] * flX) + (aflBasis[5] * flY) + (aflBasis[9] * flZ);
    const float flOutZ = (aflBasis[2] * flX) + (aflBasis[6] * flY) + (aflBasis[10] * flZ);
    const float flOutW = (aflBasis[3] * flX) + (aflBasis[7] * flY) + (aflBasis[11] * flZ);

    pDst[12] = flOutX + aflBasis[12];
    pDst[13] = flOutY + aflBasis[13];
    pDst[14] = flOutZ + aflBasis[14];
    pDst[15] = flOutW + aflBasis[15];
}

void sceVu0MulAffineMatrixXyz(float *pDst, const float *pA, const float *pB) {
    // The image pins the rows of pA in vector registers and streams the rows of pB, so either
    // factor may alias the destination. The copy below preserves that behaviour.
    float aflBasis[16];
    for (int nWord = 0; nWord < 16; ++nWord) {
        aflBasis[nWord] = pA[nWord];
    }

    for (int nRow = 0; nRow < 3; ++nRow) {
        const int nDst = nRow * kMatRowStride;
        const float flX = pB[nDst];
        const float flY = pB[nDst + 1];
        const float flZ = pB[nDst + 2];

        // The accumulate stage writes three words only, so the fourth word of each destination
        // row keeps whatever value it already held.
        pDst[nDst] = (aflBasis[0] * flX) + (aflBasis[4] * flY) + (aflBasis[8] * flZ);
        pDst[nDst + 1] = (aflBasis[1] * flX) + (aflBasis[5] * flY) + (aflBasis[9] * flZ);
        pDst[nDst + 2] = (aflBasis[2] * flX) + (aflBasis[6] * flY) + (aflBasis[10] * flZ);
    }

    // The final row also adds the fourth row of pA in three words only.
    const float flX = pB[12];
    const float flY = pB[13];
    const float flZ = pB[14];

    pDst[12] = ((aflBasis[0] * flX) + (aflBasis[4] * flY) + (aflBasis[8] * flZ)) + aflBasis[12];
    pDst[13] = ((aflBasis[1] * flX) + (aflBasis[5] * flY) + (aflBasis[9] * flZ)) + aflBasis[13];
    pDst[14] = ((aflBasis[2] * flX) + (aflBasis[6] * flY) + (aflBasis[10] * flZ)) + aflBasis[14];
}

void InversMatrix(float *pDst, const float *pSrc) {
    // The image shuffles the source rows through registers before storing anything, so every
    // value below is read before the destination is written and the pointers may alias.
    const float flXx = pSrc[0];
    const float flXy = pSrc[1];
    const float flXz = pSrc[2];
    const float flYx = pSrc[4];
    const float flYy = pSrc[5];
    const float flYz = pSrc[6];
    const float flZx = pSrc[8];
    const float flZy = pSrc[9];
    const float flZz = pSrc[10];
    const float flTx = pSrc[12];
    const float flTy = pSrc[13];
    const float flTz = pSrc[14];
    const float flTw = pSrc[15];

    // The basis rows are transposed into columns, and the shuffle feeds each new fourth word
    // from the cleared scratch row.
    pDst[0] = flXx;
    pDst[1] = flYx;
    pDst[2] = flZx;
    pDst[3] = 0.0f;

    pDst[4] = flXy;
    pDst[5] = flYy;
    pDst[6] = flZy;
    pDst[7] = 0.0f;

    pDst[8] = flXz;
    pDst[9] = flYz;
    pDst[10] = flZz;
    pDst[11] = 0.0f;

    // The translation row is the source translation dotted with each source basis row, subtracted
    // from the cleared scratch row, and its fourth word passes through unchanged.
    pDst[12] = 0.0f - (((flXx * flTx) + (flXy * flTy)) + (flXz * flTz));
    pDst[13] = 0.0f - (((flYx * flTx) + (flYy * flTy)) + (flYz * flTz));
    pDst[14] = 0.0f - (((flZx * flTx) + (flZy * flTy)) + (flZz * flTz));
    pDst[15] = flTw;
}

// Sine series weights for the ninth, seventh, fifth, and third powers, holding one vector lane
// each. The image loads these four words from 0x0077D950.
static const float kFoldedSineTable[4] = {
    0.00000275573192f,
    -0.0001984127f,
    0.008333334f,
    -0.16666667f,
};

// NTSC-U/C: 0x005e84e8, PAL: 0x0062a6d0
static float foldedSine(float flFolded) {
    // The image keeps one partial product per vector lane and slides the accumulation window
    // across the lanes, so the lanes finish at the ninth, seventh, fifth, and third powers and
    // are added in the order fourth, third, second, and first.
    const float flSquare = flFolded * flFolded;

    float flX = kFoldedSineTable[0] * flFolded;
    float flY = kFoldedSineTable[1] * flFolded;
    float flZ = kFoldedSineTable[2] * flFolded;
    float flW = kFoldedSineTable[3] * flFolded;

    flX *= flSquare;
    flY *= flSquare;
    flZ *= flSquare;
    flW *= flSquare;

    flX *= flSquare;
    flY *= flSquare;
    flZ *= flSquare;

    float flSine = flFolded + flW;

    flX *= flSquare;
    flY *= flSquare;
    flSine += flZ;

    flX *= flSquare;
    flSine += flY;

    flSine += flX;

    return flSine;
}

// NTSC-U/C: 0x005e84e0, PAL: 0x0062a6c8
void _ecossin(float flFolded, int nNegative, float *pOut) {
    // The rotation builders fold the angle about half pi and record whether it was negative, so
    // the folded sine below is the cosine of the original angle and the root below is the
    // magnitude of its sine.
    const float flCosine = foldedSine(flFolded);
    const float flRoot = sqrtf(1.0f - (flCosine * flCosine));

    pOut[0] = (nNegative != 0) ? -flRoot : flRoot;
    pOut[1] = flCosine;
    pOut[2] = 0.0f;
    pOut[3] = 0.0f;
}
