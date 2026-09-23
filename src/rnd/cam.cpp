#include "rnd/cam.h"

#include <math.h>

#include "math/frustum.h"
#include "math/plane.h"
#include "math/transformops.h"
#include "math/vector3.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/collideable.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/tex.h"
#include "rnd/transformable.h"
#include "rndartt/apalette.h"

namespace Rnd {

namespace {

// Row of a projection matrix with only its fourth component filled, which is the quadword
// UpdateProjection() clears each row to before writing the few terms the projection needs.
const Vector3 kEmptyProjectRow = {0.0f, 0.0f, 0.0f, 1.0f};

// The only revision this build writes, and the highest Load() accepts.
constexpr int kCamRevision = 8;

// Revision from which the depth range is present.
constexpr int kCamZRangeRevision = 4;

// Revision from which the render target name is present.
constexpr int kCamTargetTexRevision = 5;

// Revision from which the Rnd::Collideable form is present.
constexpr int kCamCollideRevision = 8;

// An empty HxStr stores a null buffer, and the binary substitutes the program-wide empty-string
// pointer at 0x006fbd10 rather than passing null to the stream.
const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

} // namespace

Cam *g_pCurrentCam;

// 0x006f958c
Cam *(*g_pfnNewCam)(const HxStr &name) = Cam::NewCam;

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

void Cam::ReleaseTargetTex() {
    if (mpTargetTex != nullptr) {
        mpTargetTex->RemoveRef(this);
    }
}

int Cam::UpdateWorldXfm(Transformable *pParent, int nForce) {
    if (Transformable::UpdateWorldXfm(pParent, nForce) == 0) {
        return 0;
    }
    UpdateWorldProject();
    return 1;
}

void Cam::Save(Stream &stream) {
    const int nRevision = kCamRevision;
    stream.Write(&nRevision, sizeof(nRevision));

    Transformable::Save(stream);
    Drawable::Save(stream);
    Collideable::Save(stream);

    stream.Write(&mNearPlane, sizeof(mNearPlane));
    stream.Write(&mFarPlane, sizeof(mFarPlane));
    stream.Write(&mFov, sizeof(mFov));

    stream.Write(&mScreenRect.x, sizeof(mScreenRect.x));
    stream.Write(&mScreenRect.y, sizeof(mScreenRect.y));
    stream.Write(&mScreenRect.w, sizeof(mScreenRect.w));
    stream.Write(&mScreenRect.h, sizeof(mScreenRect.h));

    stream.Write(&mZRange.x, sizeof(mZRange.x));
    stream.Write(&mZRange.y, sizeof(mZRange.y));

    const Object *pTargetObject = mpTargetTex;
    if (pTargetObject != nullptr) {
        stream.WriteBytes(NameText(pTargetObject), pTargetObject->mName.mLen + 1);
    } else {
        const char cEmpty = 0;
        stream.WriteBytes(&cEmpty, sizeof(cEmpty));
    }
}

void Cam::Load(Stream &stream) {
    int nRevision = 0;
    stream.Read(&nRevision, sizeof(nRevision));
    if (nRevision > kCamRevision) {
        g_failSink.Report("Can't load new Cam\n");
        return;
    }

    Transformable::Load(stream);
    Drawable::Load(stream);
    if (nRevision >= kCamCollideRevision) {
        Collideable::Load(stream);
    }

    ReleaseTargetTex();

    stream.Read(&mNearPlane, sizeof(mNearPlane));
    stream.Read(&mFarPlane, sizeof(mFarPlane));
    stream.Read(&mFov, sizeof(mFov));

    // Three dropped fields the reader still steps over. Each value is discarded.
    float flDropped = 0.0f;
    if (nRevision < 2) {
        stream.Read(&flDropped, sizeof(flDropped));
    }

    stream.Read(&mScreenRect.x, sizeof(mScreenRect.x));
    stream.Read(&mScreenRect.y, sizeof(mScreenRect.y));
    stream.Read(&mScreenRect.w, sizeof(mScreenRect.w));
    stream.Read(&mScreenRect.h, sizeof(mScreenRect.h));

    if (nRevision == 1 || nRevision == 2) {
        stream.Read(&flDropped, sizeof(flDropped));
    }

    if (nRevision >= kCamZRangeRevision) {
        stream.Read(&mZRange.x, sizeof(mZRange.x));
        stream.Read(&mZRange.y, sizeof(mZRange.y));
    }

    if (nRevision >= kCamTargetTexRevision) {
        HxStr targetName(nullptr);
        stream.ReadString(targetName);
        mpTargetTex = dynamic_cast<Tex *>(g_manager.Find(targetName));
    }

    if (nRevision == 6) {
        stream.Read(&flDropped, sizeof(flDropped));
    }

    AcquireTargetTex();
}

void Cam::Copy(const Object *pSource, unsigned nFlags) {
    const Cam *pSourceCam = dynamic_cast<const Cam *>(pSource);

    Transformable::Copy(pSource, nFlags);
    Drawable::Copy(pSource, nFlags);
    Collideable::Copy(pSource, nFlags);

    ReleaseTargetTex();

    mNearPlane = pSourceCam->mNearPlane;
    mFarPlane = pSourceCam->mFarPlane;
    mFov = pSourceCam->mFov;
    mScreenRect = pSourceCam->mScreenRect;
    mZRange = pSourceCam->mZRange;
    mpTargetTex = pSourceCam->mpTargetTex;

    AcquireTargetTex();
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
