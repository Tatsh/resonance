#include "rnd/cam.h"

#include <algorithm>
#include <math.h>

#include "math/frustum.h"
#include "math/plane.h"
#include "math/transformops.h"
#include "math/vector3.h"
#include "os/dbg.h"
#include "os/hxstr.h"
#include "os/mem.h"
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

// SetFrustum() keeps the near distance at no less than the far distance divided by this.
constexpr float kFarToMinNearRatio = 1000.0f;

// An empty HxStr stores a null buffer, and the binary substitutes the program-wide empty-string
// pointer at 0x006fbd10 rather than passing null to the stream.
const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

constexpr char kCamTag[] = "Rnd::Cam";

constexpr char kFloatFormat[] = "%.2f";

// Dump level from which DumpText() adds the vertical ratio, the matrices, and the frustums.
constexpr int kCamMatrixDumpLevel = 2;

// Defaults the constructor gives the projection. The field of view is one ulp short of pi / 2.
constexpr float kDefaultFarPlane = 1000.0f;
constexpr float kDefaultFov = 1.57079625f;
constexpr float kDefaultYRatio = 0.75f;

// Depths SetTargetTex() resizes a render target to: anything deeper than the first becomes the
// second.
constexpr int kTargetMinDepth = 16;
constexpr int kTargetMaxDepth = 32;

// The padding word of every row of a matrix member, which the construction of each quadword row
// sets to 1.0.
inline void SetRowPadding(Vector3 (&rows)[kXfmRowCount]) {
    for (Vector3 &row : rows) {
        row.w = 1.0f;
    }
}

// One matrix as DumpText() writes it, a tab-indented line per row.
void PrintMatrix(Dbg &sink, const Vector3 (&rows)[kXfmRowCount]) {
    for (const Vector3 &row : rows) {
        sink.Print("\n\t");
        sink.Print("(x:");
        sink.Format(kFloatFormat, row.x);
        sink.Print(" y:");
        sink.Format(kFloatFormat, row.y);
        sink.Print(" z:");
        sink.Format(kFloatFormat, row.z);
        sink.Print(")");
    }
}

// Row of a transform that holds the translation.
constexpr int kXfmTranslationRow = 3;

// Row of the world transform a camera looks along.
constexpr int kXfmForwardRow = 1;

// A point of the unit square mapped onto -1..1 at a depth of one, which is the far side of the
// projection.
inline Vector3 UnitToFarNdc(float flX, float flY) {
    return Vector3{flX + flX - 1.0f, flY + flY - 1.0f, 1.0f, 1.0f};
}

} // namespace

Cam *Cam::sCurrent;

// NTSC-U/C: 0x006f958c, PAL: 0x0073cfdc
Cam *(*Cam::sNew)(const HxStr &name) = Cam::NewCam;

void *Cam::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kCamTag);
}

void Cam::operator delete(void *pBlock) {
    OperatorDeleteOverride(pBlock, kCamTag);
}

Cam *NewCamThroughHook(const HxStr &name) {
    try {
        return Cam::sNew(name);
    } catch (...) {
        return nullptr; // The binary's handler returns null.
    }
}

Object *CreateRegisteredCam(const HxStr &name) {
    try {
        return Cam::sNew(name);
    } catch (...) {
        return nullptr;
    }
}

Cam::Cam(const HxStr &name)
    : Object(name), mNearPlane(1.0f), mFarPlane(kDefaultFarPlane), mFov(kDefaultFov),
      mYRatio(kDefaultYRatio), mZRange{0.0f, 1.0f}, mScreenRect{0.0f, 0.0f, 1.0f, 1.0f},
      mpTargetTex(nullptr) {
    SetRowPadding(mWorldToCam);
    SetRowPadding(mLocalProject);
    SetRowPadding(mInvLocalProject);
    SetRowPadding(mWorldProject);
    SetRowPadding(mInvWorldProject);
    AcquireTargetTex();
}

Cam::~Cam() {
    if (Cam::sCurrent == this) {
        Cam::sCurrent = nullptr;
    }
    ReleaseTargetTex();
    ReleaseAllRefs();
}

void Cam::SetTargetTex(Tex *pTex) {
    if (mpTargetTex != nullptr) {
        mpTargetTex->RemoveRef(this);
    }
    mpTargetTex = pTex;
    if (pTex != nullptr) {
        pTex->AddRef(this);
        // Yes, the binary tests the new target a second time.
        if (pTex != nullptr) {
            const int nDepth =
                pTex->mBitsPerPixel > kTargetMinDepth ? kTargetMaxDepth : kTargetMinDepth;
            pTex->SetBitmapConfig(
                pTex->mWidth, pTex->mHeight, nDepth, HxStr(""), pTex->mMipSelect, 0);
            pTex->ReloadBitmaps();
        }
    }
    UpdateTargetAspect();
}

void Cam::FindCollisions(const Vector2 &point, std::list<Collision> &collisions) {
    if (mShowing != 0 && mScreenRect.x < point.x && point.x < mScreenRect.x + mScreenRect.w &&
        mScreenRect.y < point.y && point.y < mScreenRect.y + mScreenRect.h) {
        const Collision collision{this, 0.0f};
        collisions.push_back(collision);
    }
    Collideable::FindCollisions(point, collisions);
}

void Cam::DumpText(Dbg &sink) {
    Object::DumpText(sink);
    Transformable::DumpText(sink);
    Drawable::DumpText(sink);
    Collideable::DumpText(sink);

    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[Cam]\n");
    sink.Print("nearPlane:");
    sink.Format(kFloatFormat, mNearPlane);
    sink.Print(" farPlane:");
    sink.Format(kFloatFormat, mFarPlane);
    sink.Print(" fov:");
    sink.Format(kFloatFormat, mFov);
    sink.Print("\n");
    sink.Print("screenRect:");
    sink.Print("(x:");
    sink.Format(kFloatFormat, mScreenRect.x);
    sink.Print(" y:");
    sink.Format(kFloatFormat, mScreenRect.y);
    sink.Print(" w:");
    sink.Format(kFloatFormat, mScreenRect.w);
    sink.Print(" h:");
    sink.Format(kFloatFormat, mScreenRect.h);
    sink.Print(")");
    sink.Print("\n");
    sink.Print("zRange:");
    sink.Print("(x:");
    sink.Format(kFloatFormat, mZRange.x);
    sink.Print(" y:");
    sink.Format(kFloatFormat, mZRange.y);
    sink.Print(")");
    sink.Print(" targetTex:");
    if (mpTargetTex != nullptr) {
        sink.Format("\"%s\"", NameText(mpTargetTex));
    } else {
        sink.Print("no object");
    }
    sink.Print("\n");

    if (sink.mDumpLevel < kCamMatrixDumpLevel) {
        return;
    }

    sink.Print("yRatio:");
    sink.Format(kFloatFormat, mYRatio);
    sink.Print(" localProject:");
    PrintMatrix(sink, mLocalProject);
    sink.Print("\n");
    sink.Print("worldProject:");
    PrintMatrix(sink, mWorldProject);
    sink.Print("\n");
    sink.Print("localFrustrum:");
    sink << mLocalFrustum;
    sink.Print("\n");
    sink.Print("worldFrustrum:");
    sink << mWorldFrustum;
    sink.Print("\n");
    sink.Print("invWorldProject:");
    PrintMatrix(sink, mInvWorldProject);
    sink.Print("\n");
}

Vector2 Cam::ScreenToPixels([[maybe_unused]] const Vector2 &ptScreen) {
    Vector2 ptPixels; // Yes, the binary returns this unset.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
    return ptPixels;
#pragma GCC diagnostic pop
}

void Cam::UpdateTargetAspect() {
    if (mpTargetTex != nullptr) {
        mYRatio =
            static_cast<float>(mpTargetTex->mHeight) / static_cast<float>(mpTargetTex->mWidth);
    }
    UpdateProjection();
}

const HxStr &Cam::ClassName() const {
    return Cam::sClassName;
}

Cam *Cam::NewCam(const HxStr &name) {
    return new Cam(name);
}

void Cam::Replace(Object *pFrom, Object *pTo) {
    Transformable::Replace(pFrom, pTo);
    Drawable::Replace(pFrom, pTo);
    Collideable::Replace(pFrom, pTo);

    if (mpTargetTex != pFrom) {
        return;
    }
    if (mpTargetTex != nullptr) {
        mpTargetTex->RemoveRef(this);
    }
    if (mpTargetTex != nullptr) {
        mpTargetTex = pTo != nullptr ? dynamic_cast<Tex *>(pTo) : nullptr;
    }
    if (mpTargetTex != nullptr) {
        mpTargetTex->AddRef(this);
    }
}

int Cam::DrawShowing() {
    Cam::sCurrent = this;
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
    stream.WriteLE(&nRevision, sizeof(nRevision));

    Transformable::Save(stream);
    Drawable::Save(stream);
    Collideable::Save(stream);

    stream.WriteLE(&mNearPlane, sizeof(mNearPlane));
    stream.WriteLE(&mFarPlane, sizeof(mFarPlane));
    stream.WriteLE(&mFov, sizeof(mFov));

    stream.WriteLE(&mScreenRect.x, sizeof(mScreenRect.x));
    stream.WriteLE(&mScreenRect.y, sizeof(mScreenRect.y));
    stream.WriteLE(&mScreenRect.w, sizeof(mScreenRect.w));
    stream.WriteLE(&mScreenRect.h, sizeof(mScreenRect.h));

    stream.WriteLE(&mZRange.x, sizeof(mZRange.x));
    stream.WriteLE(&mZRange.y, sizeof(mZRange.y));

    const Object *pTargetObject = mpTargetTex;
    if (pTargetObject != nullptr) {
        stream.Write(NameText(pTargetObject), pTargetObject->mName.mLen + 1);
    } else {
        const char cEmpty = 0;
        stream.Write(&cEmpty, sizeof(cEmpty));
    }
}

void Cam::Load(Stream &stream) {
    int nRevision = 0;
    stream.ReadLE(&nRevision, sizeof(nRevision));
    if (nRevision > kCamRevision) {
        Rnd::TheDbg.Notify("Can't load new Cam\n");
        return;
    }

    Transformable::Load(stream);
    Drawable::Load(stream);
    if (nRevision >= kCamCollideRevision) {
        Collideable::Load(stream);
    }

    ReleaseTargetTex();

    stream.ReadLE(&mNearPlane, sizeof(mNearPlane));
    stream.ReadLE(&mFarPlane, sizeof(mFarPlane));
    stream.ReadLE(&mFov, sizeof(mFov));

    // Three dropped fields the reader still steps over. Each value is discarded.
    float flDropped = 0.0f;
    if (nRevision < 2) {
        stream.ReadLE(&flDropped, sizeof(flDropped));
    }

    stream.ReadLE(&mScreenRect.x, sizeof(mScreenRect.x));
    stream.ReadLE(&mScreenRect.y, sizeof(mScreenRect.y));
    stream.ReadLE(&mScreenRect.w, sizeof(mScreenRect.w));
    stream.ReadLE(&mScreenRect.h, sizeof(mScreenRect.h));

    if (nRevision == 1 || nRevision == 2) {
        stream.ReadLE(&flDropped, sizeof(flDropped));
    }

    if (nRevision >= kCamZRangeRevision) {
        stream.ReadLE(&mZRange.x, sizeof(mZRange.x));
        stream.ReadLE(&mZRange.y, sizeof(mZRange.y));
    }

    if (nRevision >= kCamTargetTexRevision) {
        HxStr targetName(nullptr);
        stream.ReadString(targetName);
        mpTargetTex = dynamic_cast<Tex *>(TheManager.Find(targetName));
    }

    if (nRevision == 6) {
        stream.ReadLE(&flDropped, sizeof(flDropped));
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
    mLocalFrustum.Set(mNearPlane, mFarPlane, mFov, flAspect);
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
    InversMatrix(&mWorldToCam[0].x, &mWorldXfm[0][0]);

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

Segment Cam::ScreenToRay(const Vector2 &ptScreen, float flLength) {
    Segment ray;
    ray.mStart[3] = 1.0f;
    ray.mEnd[3] = 1.0f;
    const Vector3 ptNdc = UnitToFarNdc((ptScreen.x - mScreenRect.x) / mScreenRect.w,
                                       (ptScreen.y - mScreenRect.y) / mScreenRect.h);
    Vector3 ptFar;
    XfmPoint(ptNdc, mInvWorldProject, ptFar);
    Vector3 extent;
    if (mFov != 0.0f) {
        std::copy(mWorldXfm[kXfmTranslationRow], mWorldXfm[kXfmTranslationRow] + 4, ray.mStart);
        Vector3 direction;
        Rnd::Subtract(&ptFar.x, ray.mStart, &direction.x);
        Vec3Normalize(&direction.x, &direction.x);
        Vec3Scale(&direction.x, flLength, &extent.x);
        Rnd::Add(&extent.x, ray.mStart, ray.mEnd);
    } else {
        std::copy(&ptFar.x, &ptFar.x + 4, ray.mStart);
        Vec3Scale(mWorldXfm[kXfmForwardRow], flLength, &extent.x);
        Rnd::Add(ray.mStart, &extent.x, ray.mEnd);
    }
    return ray;
}

Vector3 Cam::UnprojectFar(const Vector2 &ptUnit) {
    Vector3 pt;
    XfmPoint(UnitToFarNdc(ptUnit.x, ptUnit.y), mInvWorldProject, pt);
    return pt;
}

void Cam::SetScreenRect(const Rect &rect) {
    mScreenRect = rect;
    UpdateProjection();
}

void Cam::SetFrustum(float flNear, float flFar, float flFov) {
    const float flMinNear = flFar / kFarToMinNearRatio;
    mFov = flFov;
    mNearPlane = flNear;
    mFarPlane = flFar;
    mNearPlane = std::max(mNearPlane, flMinNear);
    UpdateProjection();
}

HxStr Cam::sClassName("Cam");

} // namespace Rnd
