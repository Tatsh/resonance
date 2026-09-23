#include "math/quaternion.h"

#include <math.h>

#include "math/vector3.h"

namespace {

// A rotation matrix is three rows of four floats. A row index scales by this stride.
constexpr int kMat3RowStride = 4;

// The cyclic successor of each axis. The Shoemake construction indexes it by the axis whose
// diagonal term is largest. The image copies the table onto the stack from 0x008243f8.
constexpr int kNextAxis[] = {1, 2, 0};

// Two quaternions closer than this interpolate linearly. Below the threshold the sine of the half
// angle underflows.
constexpr float kSlerpLinearEpsilon = 1e-5f;

// Row indices of a rotation matrix, the components of a row, and the Euler angle slots.
enum Mat3Row { kRowX = 0, kRowY = 1, kRowZ = 2 };
enum Component { kX = 0, kY = 1, kZ = 2 };
constexpr int kComponentCount = 3;

// A quarter turn, reported as the X angle at the gimbal lock limit, and the Y row Z component
// beyond which the Y and Z angles cannot be separated.
constexpr float kQuarterTurn = 1.570796251f;
constexpr float kGimbalLockLimit = 0.9999998808f;

// Pi and two pi as the image stores them, one unit in the last place below the nearest float.
constexpr float kHalfTurn = 3.1415925f;
constexpr float kFullTurn = 6.28318501f;

inline float MatAt(const float *pMat3Rows, int nRow, int nComponent) {
    return pMat3Rows[(nRow * kMat3RowStride) + nComponent];
}

inline float RowLength(const float *pMat3Rows, int nRow) {
    const float flX = MatAt(pMat3Rows, nRow, kX);
    const float flY = MatAt(pMat3Rows, nRow, kY);
    const float flZ = MatAt(pMat3Rows, nRow, kZ);
    return sqrtf((flX * flX) + (flY * flY) + (flZ * flZ));
}

} // namespace

// 0x004efd80
Quat AxisAngleToQuat(const float *pAxis, float flAngle) {
    const float flHalf = flAngle * 0.5f;
    const float flSin = sinf(flHalf);

    Quat out;
    out.w = cosf(flHalf);
    out.x = pAxis[0] * flSin;
    out.y = pAxis[1] * flSin;
    out.z = pAxis[2] * flSin;
    return out;
}

// 0x004f0350
void QuatDecomposeAxisAngle(const Quat &quat, float *pAxis, float *pflAngle) {
    *pflAngle = (quat.w > 1.0f) ? 0.0f : (2.0f * acosf(quat.w));

    if (*pflAngle == 0.0f) {
        pAxis[0] = 0.0f;
        pAxis[1] = 0.0f;
        pAxis[2] = 1.0f;
        return;
    }

    const float flScale = 1.0f / sinf(*pflAngle * 0.5f);
    pAxis[0] = quat.x * flScale;
    pAxis[1] = quat.y * flScale;
    pAxis[2] = quat.z * flScale;
}

// 0x004f0230
Quat EulerAnglesToQuat(const float *pAngles) {
    Vector3 half;
    half.w = 1.0f;
    Vec3Scale(pAngles, 0.5f, &half.x);

    const float flSinX = sinf(half.x);
    const float flCosX = cosf(half.x);
    const float flSinY = sinf(half.y);
    const float flCosY = cosf(half.y);
    const float flSinZ = sinf(half.z);
    const float flCosZ = cosf(half.z);

    // The X rotation composed with the Y rotation. The image builds it in the destination,
    // then folds the Z rotation in on top of it.
    const float flXyX = flSinX * flCosY;
    const float flXyY = flCosX * flSinY;
    const float flXyZ = flSinX * flSinY;
    const float flXyW = flCosX * flCosY;

    Quat out;
    out.x = (flCosZ * flXyX) - (flSinZ * flXyY);
    out.y = (flCosZ * flXyY) + (flSinZ * flXyX);
    out.z = (flCosZ * flXyZ) + (flSinZ * flXyW);
    out.w = (flCosZ * flXyW) - (flSinZ * flXyZ);
    return out;
}

// 0x004ee9c0
Quat Mat33ToQuat(const float *pMat3Rows) {
    // The image writes the destination in place and indexes it by axis. Returning by value
    // makes gathering the components into a local indistinguishable from that.
    float aflQuat[4];

    const float flTrace = pMat3Rows[0] + pMat3Rows[5] + pMat3Rows[10];
    if (flTrace > 0.0f) {
        const float flRoot = sqrtf(flTrace + 1.0f);
        aflQuat[3] = flRoot * 0.5f;

        const float flScale = 0.5f / flRoot;
        aflQuat[0] = (pMat3Rows[6] - pMat3Rows[9]) * flScale;
        aflQuat[1] = (pMat3Rows[8] - pMat3Rows[2]) * flScale;
        aflQuat[2] = (pMat3Rows[1] - pMat3Rows[4]) * flScale;
    } else {
        int nI = (pMat3Rows[0] < pMat3Rows[5]) ? 1 : 0;
        if (pMat3Rows[(nI * kMat3RowStride) + nI] < pMat3Rows[10]) {
            nI = 2;
        }
        const int nJ = kNextAxis[nI];
        const int nK = kNextAxis[nJ];

        float flRoot =
            sqrtf(((pMat3Rows[(nI * kMat3RowStride) + nI] - pMat3Rows[(nJ * kMat3RowStride) + nJ]) -
                   pMat3Rows[(nK * kMat3RowStride) + nK]) +
                  1.0f);
        aflQuat[nI] = flRoot * 0.5f;
        if (flRoot != 0.0f) {
            flRoot = 0.5f / flRoot;
        }

        aflQuat[3] =
            (pMat3Rows[(nJ * kMat3RowStride) + nK] - pMat3Rows[(nK * kMat3RowStride) + nJ]) *
            flRoot;
        aflQuat[nJ] =
            (pMat3Rows[(nI * kMat3RowStride) + nJ] + pMat3Rows[(nJ * kMat3RowStride) + nI]) *
            flRoot;
        aflQuat[nK] =
            (pMat3Rows[(nI * kMat3RowStride) + nK] + pMat3Rows[(nK * kMat3RowStride) + nI]) *
            flRoot;
    }

    Quat out;
    out.x = aflQuat[0];
    out.y = aflQuat[1];
    out.z = aflQuat[2];
    out.w = aflQuat[3];
    return out;
}

// 0x004f06a0
void QuatMultiply(const Quat &a, const Quat &b, Quat &out) {
    const float flX = (((a.w * b.x) + (a.x * b.w)) + (a.y * b.z)) - (a.z * b.y);
    const float flY = (((a.w * b.y) + (a.y * b.w)) + (a.z * b.x)) - (a.x * b.z);
    const float flZ = (((a.w * b.z) + (a.z * b.w)) + (a.x * b.y)) - (a.y * b.x);
    const float flW = (((a.w * b.w) - (a.x * b.x)) - (a.y * b.y)) - (a.z * b.z);

    out.x = flX;
    out.y = flY;
    out.z = flZ;
    out.w = flW;
}

// 0x004f0178
Quat QuatRotateByVector(const Quat &quat, const float *pRotVec) {
    const float flAngle =
        sqrtf((pRotVec[0] * pRotVec[0]) + (pRotVec[1] * pRotVec[1]) + (pRotVec[2] * pRotVec[2]));

    Vector3 axis;
    axis.w = 1.0f;
    Vec3Scale(pRotVec, 1.0f / flAngle, &axis.x);

    const Quat delta = AxisAngleToQuat(&axis.x, flAngle);
    Quat out;
    QuatMultiply(quat, delta, out);
    return out;
}

// 0x004eec20
void QuatSlerp(const Quat &from, const Quat &to, Quat &out, float flT) {
    if (flT == 0.0f) {
        out = from;
        return;
    }
    if (flT == 1.0f) {
        out = to;
        return;
    }

    double dDot = (((from.x * to.x) + (from.y * to.y)) + (from.z * to.z)) + (from.w * to.w);

    Quat target = to;
    if (dDot < 0.0) {
        target.x = -to.x;
        target.y = -to.y;
        target.z = -to.z;
        target.w = -to.w;
        dDot = 0.0 - dDot; // Yes, the binary subtracts from zero rather than negating.
    }

    double dFromScale;
    double dToScale;
    if ((1.0 - dDot) > kSlerpLinearEpsilon) {
        const double dTheta = acosf(dDot);
        const double dInvSinTheta = 1.0f / sinf(dTheta);
        dFromScale = sinf((1.0f - flT) * dTheta) * dInvSinTheta;
        dToScale = sinf(flT * dTheta) * dInvSinTheta;
    } else {
        dFromScale = 1.0f - flT;
        dToScale = flT;
    }

    out.x = (dFromScale * from.x) + (dToScale * target.x);
    out.y = (dFromScale * from.y) + (dToScale * target.y);
    out.z = (dFromScale * from.z) + (dToScale * target.z);
    out.w = (dFromScale * from.w) + (dToScale * target.w);
}

// 0x004f0600
void QuatToMat33(const Quat &quat, float *pMat3Rows) {
    const float flX2 = quat.x + quat.x;
    const float flY2 = quat.y + quat.y;
    const float flZ2 = quat.z + quat.z;

    const float flXx = flX2 * quat.x;
    const float flYy = flY2 * quat.y;
    const float flZz = flZ2 * quat.z;
    const float flXy = flX2 * quat.y;
    const float flXz = flX2 * quat.z;
    const float flYz = flY2 * quat.z;
    const float flWx = flX2 * quat.w;
    const float flWy = flY2 * quat.w;
    const float flWz = flZ2 * quat.w;

    pMat3Rows[0] = 1.0f - flYy - flZz;
    pMat3Rows[1] = flXy + flWz;
    pMat3Rows[2] = flXz - flWy;

    pMat3Rows[4] = flXy - flWz;
    pMat3Rows[5] = 1.0f - flZz - flXx;
    pMat3Rows[6] = flYz + flWx;

    pMat3Rows[8] = flXz + flWy;
    pMat3Rows[9] = flYz - flWx;
    pMat3Rows[10] = 1.0f - flXx - flYy;
}

// 0x004efe08
void Mat33ToEulerAngles(const float *pMat3Rows, float *pAngles) {
    const float flYRowZ = MatAt(pMat3Rows, kRowY, kZ);
    if (fabsf(flYRowZ) > kGimbalLockLimit) {
        pAngles[kX] = (flYRowZ > 0.0f) ? kQuarterTurn : -kQuarterTurn;
        pAngles[kZ] = atan2f(MatAt(pMat3Rows, kRowX, kY), MatAt(pMat3Rows, kRowX, kX));
        pAngles[kY] = 0.0f;
        return;
    }

    pAngles[kZ] = atan2f(-MatAt(pMat3Rows, kRowY, kX), MatAt(pMat3Rows, kRowY, kY));
    pAngles[kX] = asinf(flYRowZ);
    pAngles[kY] = atan2f(-MatAt(pMat3Rows, kRowX, kZ), MatAt(pMat3Rows, kRowZ, kZ));
}

// 0x004efed8
void Mat33ExtractScale(const float *pMat3Rows, float *pScale) {
    const float flLenZ = RowLength(pMat3Rows, kRowZ);
    const float flLenX = RowLength(pMat3Rows, kRowX);
    const float flLenY = RowLength(pMat3Rows, kRowY);

    // The cross product is an inline vopmula and vopmsub pair rather than a call.
    const float flCrossX = (MatAt(pMat3Rows, kRowX, kY) * MatAt(pMat3Rows, kRowY, kZ)) -
                           (MatAt(pMat3Rows, kRowX, kZ) * MatAt(pMat3Rows, kRowY, kY));
    const float flCrossY = (MatAt(pMat3Rows, kRowX, kZ) * MatAt(pMat3Rows, kRowY, kX)) -
                           (MatAt(pMat3Rows, kRowX, kX) * MatAt(pMat3Rows, kRowY, kZ));
    const float flCrossZ = (MatAt(pMat3Rows, kRowX, kX) * MatAt(pMat3Rows, kRowY, kY)) -
                           (MatAt(pMat3Rows, kRowX, kY) * MatAt(pMat3Rows, kRowY, kX));
    const float flHandedness = (flCrossX * MatAt(pMat3Rows, kRowZ, kX)) +
                               (flCrossY * MatAt(pMat3Rows, kRowZ, kY)) +
                               (flCrossZ * MatAt(pMat3Rows, kRowZ, kZ));

    pScale[kZ] = (flHandedness > 0.0f) ? flLenZ : -flLenZ;
    pScale[kX] = flLenX;
    pScale[kY] = flLenY;
}

// 0x004effe0
void LerpEulerAngles(const float *pFrom, const float *pTo, float *pOut, float flT) {
    // The image reads every input before it writes the first output.
    float aflOut[kComponentCount];
    for (int i = 0; i < kComponentCount; ++i) {
        float flDelta = fmodf((pTo[i] - pFrom[i]) + kHalfTurn, kFullTurn);
        if (flDelta < 0.0f) {
            flDelta += kFullTurn;
        }
        aflOut[i] = ((flDelta - kHalfTurn) * flT) + pFrom[i];
    }
    for (int i = 0; i < kComponentCount; ++i) {
        pOut[i] = aflOut[i];
    }
}
