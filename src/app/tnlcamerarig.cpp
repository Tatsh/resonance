#include "app/tnlcamerarig.h"

#include <cstring>

#include "math/plane.h"
#include "math/transform.h"
#include "math/transformops.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/transanim.h"
#include "rnd/transformable.h"
#include "rnd/view.h"

namespace {

// Slots in each of the two vectors, one per possible player.
constexpr int kSlotCount = 4;

// Frames the ramp takes to cross from one pose to the other.
constexpr float kRampFrames = 480.0f;

constexpr float kPlayPitch = 0.488692194f; // 28 degrees.
constexpr float kPlayFov = 1.39626336f;    // 80 degrees.

// Row of a transform that stores the translation.
constexpr int kXfmRowTranslation = 3;

// Give one camera the blended field of view and screen rectangle and rebuild its projection.
inline void ApplyLens(Rnd::Cam *pCam, float flFov, const Rnd::Cam::Rect &rect) {
    pCam->SetFrustum(pCam->GetNearPlane(), pCam->GetFarPlane(), flFov);
    pCam->mScreenRect = rect;
    pCam->UpdateProjection();
}

// Show or hide the outer cameras and views of players 2 to 4.
inline void ShowOtherPlayers(const std::vector<Rnd::Cam *> &outerCams,
                             const std::vector<Rnd::View *> &localViews,
                             int nShowing) {
    for (int i = 1; i < kSlotCount; ++i) {
        if (outerCams[i] != nullptr) {
            outerCams[i]->SetShowing(nShowing);
        }
        if (localViews[i] != nullptr) {
            localViews[i]->SetShowing(nShowing);
        }
    }
}

} // namespace

TnlCameraRig::TnlCameraRig(int nPlayerCount, int nSkipIntro)
    : mPlayerCount(nPlayerCount), mState(kStateSettled) {
    mIntroPos.w = 1.0f;
    mPlayPos.w = 1.0f;
    mRamp.SetRange(0.0f, 1.0f, kRampFrames);

    mMainCams[kMainCamNormal] = dynamic_cast<Rnd::Cam *>(Rnd::g_manager.Find(HxStr("tnl cam1")));
    mMainCams[kMainCamZoom] = dynamic_cast<Rnd::Cam *>(Rnd::g_manager.Find(HxStr("tnl cam1z")));
    mOuterCams.resize(kSlotCount, nullptr);
    mLocalViews.resize(kSlotCount, nullptr);
    for (int i = 0; i < nPlayerCount; ++i) {
        mOuterCams[i] = dynamic_cast<Rnd::Cam *>(
            Rnd::g_manager.Find(HxStr(FormatString("outer cam%d", i + 1))));
        mLocalViews[i] = dynamic_cast<Rnd::View *>(
            Rnd::g_manager.Find(HxStr(FormatString("tnl local%d.view", i + 1))));
    }

    Rnd::TransAnim *pIntro = dynamic_cast<Rnd::TransAnim *>(
        Rnd::g_manager.Find(HxStr(FormatString("tnl cam intro%d.tnm", mPlayerCount))));
    Transform xfm;
    pIntro->EvalFrame(0.0f, &xfm.mBasisX.x, 1);
    Vector3 angles;
    Vector3 scale;
    Mat34DecomposeEulerScale(&xfm.mBasisX.x, &angles.x, &scale.x);

    mIntroPitch = angles.x;
    mIntroPos = xfm.mTranslation;
    mIntroRect = mMainCams[kMainCamNormal]->mScreenRect;
    mIntroFov = mMainCams[kMainCamNormal]->GetFov();
    mPlayPitch = kPlayPitch;
    mPlayPos = Vector3{0.0f, -0.5f, 0.0f, 1.0f};
    mPlayRect = Rnd::Cam::Rect{0.0f, 0.1f, 1.0f, 0.8f};
    mPlayFov = kPlayFov;

    if (nSkipIntro != 0) {
        mRamp.Jump(1.0f);
        ZoomIn();
    }
}

void TnlCameraRig::ZoomIn() {
    mRamp.SetTarget(1.0f);
    mState = kStateZoomedIn;
    ShowOtherPlayers(mOuterCams, mLocalViews, 0);
}

void TnlCameraRig::ZoomOut() {
    mRamp.SetTarget(0.0f);
    mState = kStateZoomingOut;
}

void TnlCameraRig::SetFrame(float flTime) {
    if (mRamp.Update(flTime) == 0) {
        if (mState == kStateSettled) {
            return;
        }
        if (mState == kStateZoomingOut) {
            ShowOtherPlayers(mOuterCams, mLocalViews, 1);
        }
        mState = kStateSettled;
        return;
    }

    const float flPitch = (mPlayPitch - mIntroPitch) * mRamp.Value() + mIntroPitch;
    const float flWeight = mRamp.Value();
    Vector3 pos;
    pos.x = mPlayPos.x * flWeight + mIntroPos.x * (1.0f - flWeight);
    pos.y = mPlayPos.y * flWeight + mIntroPos.y * (1.0f - flWeight);
    pos.z = mPlayPos.z * flWeight + mIntroPos.z * (1.0f - flWeight);
    pos.w = mPlayPos.w;
    Rnd::Cam::Rect rect;
    InterpolateFourFloats(&mIntroRect.x, &mPlayRect.x, &rect.x, mRamp.Value());
    const float flFov = (mPlayFov - mIntroFov) * mRamp.Value() + mIntroFov;

    const Vector3 pitchAngles{flPitch, 0.0f, 0.0f, 1.0f};
    Vector3 basis[3];
    for (Vector3 &row : basis) {
        row.w = 1.0f;
    }
    EulerAnglesToMatrix3x3(&pitchAngles.x, &basis[0].x);

    Rnd::Cam *pCam = mMainCams[kMainCamNormal];
    std::memcpy(pCam->mLocalXfm, basis, sizeof(basis));
    pCam->mDirty = 1;
    std::memcpy(pCam->mLocalXfm[kXfmRowTranslation], &pos, sizeof(pos));
    pCam->mDirty = 1;

    for (Rnd::Cam *pCam : mMainCams) {
        ApplyLens(pCam, flFov, rect);
    }
    ApplyLens(mOuterCams[0], flFov, rect);
}
