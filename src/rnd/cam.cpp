#include "rnd/cam.h"

#include <math.h>

#include "math/frustum.h"
#include "math/plane.h"
#include "math/transformops.h"
#include "math/vector3.h"
#include "rnd/transformable.h"

namespace Rnd {

namespace {

// Row of a projection matrix with only its fourth component filled, which is the quadword
// UpdateProjection() clears each row to before writing the few terms the projection needs.
const Vector3 kEmptyProjectRow = {0.0f, 0.0f, 0.0f, 1.0f};

} // namespace

Cam *g_pCurrentCam;

int Cam::DrawSelf() {
    g_pCurrentCam = this;
    return 1;
}

void Cam::AcquireTargetTex() {
    if (mpTargetTex != nullptr) {
        mpTargetTex->AddRef(this);
    }
    UpdateTargetAspect();
}

void Cam::UpdateProjection() {
    const float flAspect = (mYRatio * mScreenRect.h) / mScreenRect.w;
    BuildFrustum(mLocalFrustum, mNearPlane, mFarPlane, mFov, flAspect);
    for (int nRow = 0; nRow < kXfmRowCount; ++nRow) {
        mLocalProject[nRow] = kEmptyProjectRow;
    }
    for (int nRow = 0; nRow < kXfmRowCount; ++nRow) {
        mInvLocalProject[nRow] = kEmptyProjectRow;
    }
    if (mFov == 0.0f) {
        mLocalProject[0].x = 2.0f;
        mLocalProject[2].y = -2.0f / flAspect;
        mLocalProject[3].x = -1.0f;
        mLocalProject[3].y = -1.0f;
        mInvLocalProject[0].x = 0.5f;
        mInvLocalProject[1].z = flAspect * -0.5f;
        mInvLocalProject[3].x = 0.5f;
        mInvLocalProject[3].z = flAspect * -0.5f;
    } else {
        const float flScale = 1.0f / tanf(mFov * 0.5f);
        mLocalProject[0].x = flScale;
        mLocalProject[1].z = 1.0f;
        mLocalProject[2].y = -flScale / flAspect;
        mInvLocalProject[0].x = 1.0f / flScale;
        mInvLocalProject[1].z = -flAspect / flScale;
        mInvLocalProject[2].y = 1.0f;
    }
    UpdateWorldProject();
}

void Cam::UpdateWorldProject() {
    XfmInvertRigid(&mWorldToCam[0].x, &mWorldXfm[0][0]);

    Frustum worldFrustum;
    worldFrustum.mFront = TransformPlaneToWorld(mLocalFrustum.mFront, &mWorldXfm[0][0]);
    worldFrustum.mBack = TransformPlaneToWorld(mLocalFrustum.mBack, &mWorldXfm[0][0]);
    worldFrustum.mLeft = TransformPlaneToWorld(mLocalFrustum.mLeft, &mWorldXfm[0][0]);
    worldFrustum.mRight = TransformPlaneToWorld(mLocalFrustum.mRight, &mWorldXfm[0][0]);
    worldFrustum.mTop = TransformPlaneToWorld(mLocalFrustum.mTop, &mWorldXfm[0][0]);
    worldFrustum.mBottom = TransformPlaneToWorld(mLocalFrustum.mBottom, &mWorldXfm[0][0]);
    mWorldFrustum = worldFrustum;

    Vector3 aResult[kXfmRowCount];
    for (int nRow = 0; nRow < kXfmRowCount; ++nRow) {
        aResult[nRow].w = 1.0f;
    }
    XfmConcat(&mWorldToCam[0].x, &mLocalProject[0].x, &aResult[0].x);
    for (int nRow = 0; nRow < kXfmRowCount; ++nRow) {
        mWorldProject[nRow] = aResult[nRow];
    }
    XfmConcat(&mInvLocalProject[0].x, &mWorldXfm[0][0], &aResult[0].x);
    for (int nRow = 0; nRow < kXfmRowCount; ++nRow) {
        mInvWorldProject[nRow] = aResult[nRow];
    }
}

} // namespace Rnd
