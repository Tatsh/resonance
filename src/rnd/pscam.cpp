#include "rnd/pscam.h"

#include "gfx/gfxdevice.h"
#include "math/vector2.h"
#include "os/hxstr.h"
#include "rnd/cam.h"
#include "rnd/tex.h"
#include "rndartt/apalette.h"

namespace Rnd {

namespace {

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

// 0x00582558
PsCam::PsCam(const HxStr &name) : Object(name), Cam(name) {
}

// 0x005826e0
PsCam::~PsCam() {
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
