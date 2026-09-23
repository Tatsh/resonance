#include "rnd/pscam.h"

#include <math.h>
#include <string.h>

#include "gfx/gfxdevice.h"
#include "math/frustum.h"
#include "math/plane.h"
#include "math/transform.h"
#include "math/transformops.h"
#include "math/vector2.h"
#include "math/vector3.h"
#include "os/hxstr.h"
#include "rnd/cam.h"
#include "rnd/drawverts.h"
#include "rnd/pstex.h"
#include "rnd/tex.h"
#include "rndartt/apalette.h"

namespace Rnd {

namespace {

// Centre of the GS primitive coordinate space, as a float for the guard band and as an integer for
// the scissor conversion, and the four fractional bits of a primitive coordinate.
constexpr float kGsCoordinateCentre = 2048.0f;
constexpr int kGsCoordinateCentreInt = 0x800;
constexpr int kGsSubpixelShift = 4;

// Fraction of the screen rectangle each guard band axis is measured against, a 2 per cent margin
// over half the extent.
constexpr float kGuardBandHalfExtent = 0.51f;

// Depth range scaling and the bias the third viewport transform adds.
constexpr float kDepthRangeScale = 0.95f;
constexpr float kDepthBias = 0.0005f;
constexpr int kBitsPerByte = 8;

// SCISSOR_1 and the positions of its four fields.
constexpr int kGsRegScissor1 = 0x40;
constexpr unsigned long long kGsRegAllBits = ~0ULL;
constexpr int kScissorX1Shift = 16;
constexpr int kScissorY0Shift = 32;
constexpr int kScissorY1Shift = 48;

// Index of the first side plane in a frustum, after the front and back planes.
constexpr int kFirstSidePlane = 2;

// Store one plane into the flat plane array the draw path tests against.
inline void StoreDrawFrustumPlane(int nPlane, const Plane &plane) {
    memcpy(&g_afDrawFrustumPlanes[nPlane * kFrustumPlaneFloatCount], &plane, sizeof(plane));
}

// Read one plane back out of the same array.
inline Plane LoadDrawFrustumPlane(int nPlane) {
    Plane plane;
    memcpy(&plane, &g_afDrawFrustumPlanes[nPlane * kFrustumPlaneFloatCount], sizeof(plane));
    return plane;
}

// Widen one side plane of the local frustum by a guard band factor applied to the y component of
// its normal, retaining the point of the plane nearest the origin.
inline Plane WidenSidePlane(const Plane &plane, float flScale) {
    const float flDistance = -plane.d;
    Vector3 point;
    point.x = plane.a * flDistance;
    point.y = plane.b * flDistance;
    point.z = plane.c * flDistance;

    Vector3 normal;
    normal.x = plane.a;
    normal.y = plane.b * flScale;
    normal.z = plane.c;
    normal.w = 1.0f;
    Vec3Normalize(&normal.x, &normal.x);

    Plane widened;
    widened.a = normal.x;
    widened.b = normal.y;
    widened.c = normal.z;
    widened.d = -(normal.x * point.x + normal.y * point.y + normal.z * point.z);
    return widened;
}

// Clamp a coordinate of the screen rectangle to the unit range.
inline float ClampToUnit(float flValue) {
    if (flValue > 1.0f) {
        return 1.0f;
    }
    if (flValue < 0.0f) {
        return 0.0f;
    }
    return flValue;
}

// Vertical ratio of a four by three display, which is what a camera with no render target
// projects into.
constexpr float kDisplayYRatio = 0.75f;

// Where Init() places the default camera, 150 units back along the viewing axis.
constexpr float kDefaultCamDistance = -150.0f;

// Row of a transform that stores the translation.
constexpr int kXfmRowTranslation = 3;

} // namespace

// 0x00768410
PsCam *g_pDefaultCam;

// 0x00768420
float g_afDrawFrustumPlanes[kFrustumPlaneCount * kFrustumPlaneFloatCount];

// 0x008e4020
Transform g_viewProjectXfm;

// 0x008e4060
Transform g_viewProjectUnscaledXfm;

// 0x008e40a0
Transform g_viewportXfm;

// 0x008e40e0
Transform g_viewportUnscaledXfm;

// 0x008e4120
Transform g_viewportBiasedXfm;

// 0x008e4160
Vector3 g_particleScreenScale;

// 0x008e4170
Vector3 g_particleProjectScale;

// 0x008e4180
Vector3 g_guardBandScale;

// 0x008e4190
Vector3 g_invGuardBandScale;

// 0x008e41a0
float g_flCamNear;

// 0x008e41a4
int g_nScissorX0;

// 0x008e41a8
int g_nScissorX1;

// 0x008e41ac
int g_nScissorY0;

// 0x008e41b0
int g_nScissorY1;

// 0x00582558
PsCam::PsCam(const HxStr &name) : Object(name), Cam(name) {
}

// 0x005826e0
PsCam::~PsCam() {
}

// 0x00582830
int PsCam::DrawSelf() {
    int nTargetWidth;
    int nTargetHeight;
    if (mpTargetTex != nullptr) {
        static_cast<PsTex *>(mpTargetTex)->BindAsRenderTarget();
        nTargetWidth = mpTargetTex->mWidth;
        nTargetHeight = mpTargetTex->mHeight;
    } else {
        if (g_pCurrentCam != nullptr && g_pCurrentCam->mpTargetTex != nullptr) {
            g_gfxDevice.RestoreFrameBufferTarget();
        }
        nTargetWidth = g_gfxDevice.mnDisplayWidth;
        nTargetHeight = g_gfxDevice.mnDisplayHeight;
    }

    // The binary subtracts the half through the Vector2 routine at 0x004bec40.
    Vector2 offset;
    offset.x = mScreenRect.x + mScreenRect.w * 0.5f - 0.5f;
    offset.y = mScreenRect.y + mScreenRect.h * 0.5f - 0.5f;
    g_flCamNear = mNearPlane;

    const float flWidth = static_cast<float>(nTargetWidth);
    const float flHeight = static_cast<float>(nTargetHeight);
    g_guardBandScale.z = 1.0f;
    g_guardBandScale.x = (kGsCoordinateCentre - fabsf(offset.x * flWidth)) /
                         (mScreenRect.w * flWidth * kGuardBandHalfExtent);
    g_guardBandScale.y = (kGsCoordinateCentre - fabsf(offset.y * flHeight)) /
                         (mScreenRect.h * flHeight * kGuardBandHalfExtent);
    if (g_guardBandScale.x != 0.0f && g_guardBandScale.y != 0.0f) {
        g_invGuardBandScale.z = 1.0f;
        g_invGuardBandScale.x = 1.0f / g_guardBandScale.x;
        g_invGuardBandScale.y = 1.0f / g_guardBandScale.y;
    }

    StoreDrawFrustumPlane(0, mLocalFrustum.mFront);
    StoreDrawFrustumPlane(1, mLocalFrustum.mBack);
    StoreDrawFrustumPlane(kFirstSidePlane, WidenSidePlane(mLocalFrustum.mLeft, g_guardBandScale.x));
    StoreDrawFrustumPlane(kFirstSidePlane + 1,
                          WidenSidePlane(mLocalFrustum.mRight, g_guardBandScale.x));
    StoreDrawFrustumPlane(kFirstSidePlane + 2,
                          WidenSidePlane(mLocalFrustum.mTop, g_guardBandScale.y));
    StoreDrawFrustumPlane(kFirstSidePlane + 3,
                          WidenSidePlane(mLocalFrustum.mBottom, g_guardBandScale.y));

    // Every plane is transformed before any is stored back.
    Plane aWorldPlanes[kFrustumPlaneCount];
    for (int nPlane = 0; nPlane < kFrustumPlaneCount; ++nPlane) {
        aWorldPlanes[nPlane] =
            TransformPlaneToWorld(LoadDrawFrustumPlane(nPlane), &mWorldXfm[0][0]);
    }
    for (int nPlane = 0; nPlane < kFrustumPlaneCount; ++nPlane) {
        StoreDrawFrustumPlane(nPlane, aWorldPlanes[nPlane]);
    }

    // The three basis rows are set to the identity without touching their padding words.
    g_viewportXfm.mBasisX.x = 1.0f;
    g_viewportXfm.mBasisX.y = 0.0f;
    g_viewportXfm.mBasisX.z = 0.0f;
    g_viewportXfm.mBasisY.x = 0.0f;
    g_viewportXfm.mBasisY.y = 1.0f;
    g_viewportXfm.mBasisY.z = 0.0f;
    g_viewportXfm.mBasisZ.x = 0.0f;
    g_viewportXfm.mBasisZ.y = 0.0f;
    g_viewportXfm.mBasisZ.z = 1.0f;
    g_viewportXfm.mTranslation.x = 0.0f;
    g_viewportXfm.mTranslation.y = 0.0f;
    g_viewportXfm.mTranslation.z = 0.0f;
    g_viewportXfm.mTranslation.w = 1.0f;

    const float flHalfWidth = flWidth * mScreenRect.w * 0.5f;
    const float flHalfHeight = flHeight * mScreenRect.h * 0.5f;
    const int nMaxDepth = (1 << (g_gfxDevice.mnDepthBytes * kBitsPerByte)) - 1;
    const float flHalfDepth = static_cast<float>(nMaxDepth) * 0.5f;
    g_viewportXfm.mBasisX.x = flHalfWidth;
    g_viewportXfm.mBasisY.y = flHalfHeight;
    g_viewportXfm.mTranslation.x = offset.x * flWidth + kGsCoordinateCentre;
    g_viewportXfm.mTranslation.y = offset.y * flHeight + kGsCoordinateCentre;
    g_viewportXfm.mBasisZ.z = flHalfDepth * (mZRange.x - mZRange.y) * kDepthRangeScale;
    g_viewportXfm.mTranslation.z = flHalfDepth * (2.0f - mZRange.x - mZRange.y);
    g_viewportUnscaledXfm = g_viewportXfm;

    g_viewportXfm.mBasisY.y = flHalfHeight * g_guardBandScale.y;
    g_viewportXfm.mBasisX.x = flHalfWidth * g_guardBandScale.x;
    g_viewportBiasedXfm = g_viewportXfm;
    g_viewportBiasedXfm.mTranslation.z += flHalfDepth * (mZRange.y - mZRange.x) * kDepthBias;

    g_viewProjectXfm.mBasisX = mLocalProject[0];
    g_viewProjectXfm.mBasisY = mLocalProject[1];
    g_viewProjectXfm.mBasisZ = mLocalProject[2];
    g_viewProjectXfm.mTranslation = mLocalProject[3];
    g_viewProjectXfm.mBasisZ.w = 0.0f;
    g_viewProjectXfm.mBasisY.w = 0.0f;
    g_viewProjectXfm.mBasisX.w = 0.0f;
    g_viewProjectXfm.mTranslation.w = 1.0f;
    if (mFov == 0.0f) {
        g_viewProjectXfm.mBasisY.z = 2.0f / (mFarPlane - mNearPlane);
        g_viewProjectXfm.mTranslation.z = 1.0f - mFarPlane * g_viewProjectXfm.mBasisY.z;
    } else {
        g_viewProjectXfm.mTranslation.w = 0.0f;
        g_viewProjectXfm.mBasisY.w = g_viewProjectXfm.mBasisY.z;
        g_viewProjectXfm.mBasisY.z = (mFarPlane + mNearPlane) / (mFarPlane - mNearPlane);
        g_viewProjectXfm.mTranslation.z = mFarPlane - mFarPlane * g_viewProjectXfm.mBasisY.z;
    }

    const float flProjectX = g_viewProjectXfm.mBasisX.x / g_guardBandScale.x;
    const float flProjectY = g_viewProjectXfm.mBasisZ.y / g_guardBandScale.y;
    g_viewProjectUnscaledXfm = g_viewProjectXfm;
    g_particleScreenScale.z = 0.0f;
    g_particleProjectScale.z = 0.0f;
    g_viewProjectXfm.mBasisZ.y = flProjectY;
    g_particleProjectScale.x = flProjectX;
    g_viewProjectXfm.mBasisX.x = flProjectX;
    g_particleProjectScale.y = -flProjectY;
    g_particleScreenScale.x = flProjectX * g_viewportXfm.mBasisX.x;
    g_particleScreenScale.y = -flProjectY * g_viewportXfm.mBasisY.y;
    g_particleScreenScale.w = 0.0f;
    g_particleProjectScale.w = 0.0f;
    Mat44Concat(&g_viewProjectXfm.mBasisX.x, &g_viewProjectXfm.mBasisX.x, &mWorldToCam[0].x);
    Mat44Concat(&g_viewProjectUnscaledXfm.mBasisX.x,
                &g_viewProjectUnscaledXfm.mBasisX.x,
                &mWorldToCam[0].x);

    const float flLeft = ClampToUnit(mScreenRect.x);
    const float flTop = ClampToUnit(mScreenRect.y);
    const float flRight = ClampToUnit(mScreenRect.x + mScreenRect.w);
    const float flBottom = ClampToUnit(mScreenRect.y + mScreenRect.h);
    g_nScissorX1 = static_cast<int>(flRight * flWidth) - 1;
    g_nScissorY1 = static_cast<int>(flBottom * flHeight) - 1;
    g_nScissorX0 = static_cast<int>(flLeft * flWidth);
    g_nScissorY0 = static_cast<int>(flTop * flHeight);
    g_gfxDevice.SetGsReg(kGsRegScissor1,
                         static_cast<unsigned long long>(g_nScissorX0) |
                             (static_cast<unsigned long long>(g_nScissorX1) << kScissorX1Shift) |
                             (static_cast<unsigned long long>(g_nScissorY0) << kScissorY0Shift) |
                             (static_cast<unsigned long long>(g_nScissorY1) << kScissorY1Shift),
                         kGsRegAllBits);

    g_pCurrentCam = this;
    const int nOriginY =
        kGsCoordinateCentreInt - static_cast<int>(g_gfxDevice.mnDisplayHeight * 0.5f);
    const int nOriginX =
        kGsCoordinateCentreInt - static_cast<int>(g_gfxDevice.mnDisplayWidth * 0.5f);
    g_nScissorY1 = (nOriginY + g_nScissorY1) << kGsSubpixelShift;
    g_nScissorY0 = (nOriginY + g_nScissorY0) << kGsSubpixelShift;
    g_nScissorX1 = (nOriginX + g_nScissorX1) << kGsSubpixelShift;
    g_nScissorX0 = (nOriginX + g_nScissorX0) << kGsSubpixelShift;
    return 1;
}

// 0x005885b0
Vector2 PsCam::ScreenToPixels(const Vector2 &ptScreen) {
    Vector2 ptPixels;
    ptPixels.x = mScreenRect.x + ptScreen.x * mScreenRect.w;
    ptPixels.y = mScreenRect.y + ptScreen.y * mScreenRect.h;
    if (mpTargetTex != nullptr) {
        ptPixels.x *= static_cast<float>(mpTargetTex->mWidth);
        ptPixels.y *= static_cast<float>(mpTargetTex->mHeight);
    } else {
        ptPixels.y *= static_cast<float>(g_gfxDevice.mnDisplayHeight);
        ptPixels.x *= static_cast<float>(g_gfxDevice.mnDisplayWidth);
    }
    return ptPixels;
}

// 0x00588578
void PsCam::UpdateTargetAspect() {
    if (mpTargetTex == nullptr) {
        mYRatio = kDisplayYRatio;
    }
    Cam::UpdateTargetAspect();
}

// 0x00588498
void PsCam::SetTargetTex(Tex *pTex) {
    Cam::SetTargetTex(pTex);
    if (pTex == nullptr) {
        mYRatio = kDisplayYRatio;
    }
}

// 0x00588500
Cam *PsCam::NewCam(const HxStr &name) {
    // The binary bills the allocation to the tag "Rnd::Cam" and the object is 0x330 bytes.
    return new PsCam(name);
}

// 0x00582430
void PsCam::Init() {
    g_pfnNewCam = NewCam;
    g_pDefaultCam = new PsCam(HxStr("[default cam]"));
    g_pDefaultCam->mInternal = 1;

    // The binary assembles the row in a temporary and stores it as one quadword.
    float *pTranslation = g_pDefaultCam->mLocalXfm[kXfmRowTranslation];
    pTranslation[0] = 0.0f;
    pTranslation[1] = kDefaultCamDistance;
    pTranslation[2] = 0.0f;
    pTranslation[3] = 1.0f;
    g_pDefaultCam->mDirty = 1;
}

} // namespace Rnd
