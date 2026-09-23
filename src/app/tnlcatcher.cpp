#include "app/tnlcatcher.h"

#include "os/formatstring.h"
#include "rnd/animatable.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/tex.h"
#include "rnd/view.h"

namespace {

// Starting value of both counts, far past the reset at kResetHitCount.
constexpr int kIdleCount = 100000000;

constexpr int kResetHitCount = 10;

// One literal in the image, split after the colour letter.
constexpr char kTargetMeshFormat[] = "act_%c"
                                     "tar%d.mesh";

// View frames per target, and the length of the hit animation within one target.
constexpr int kFramesPerTarget = 100;

// View frames per update since the hit.
constexpr int kHitFramesPerUpdate = 2;

template <typename T>
T *FindObject(const HxStr &name) {
    return dynamic_cast<T *>(Rnd::g_manager.Find(name));
}

} // namespace

TnlCatcher::TnlCatcher(const HxStr &colorName)
    : mView(nullptr), mTarget(0), mMultiplied(0), mUnknown30(0), mUpdateCount(kIdleCount),
      mHitCount(kIdleCount) {
    const HxStr letter(1, colorName[0]);
    mView = FindObject<Rnd::View>(HxStr("catcher_") + letter);
    for (int i = 0; i < kTargetCount; ++i) {
        mTargets[i] =
            FindObject<Rnd::Mesh>(HxStr(FormatString(kTargetMeshFormat, colorName[0], i)));
    }
    mUpMat = FindObject<Rnd::Mat>(HxStr("act_") + letter + " tar up.mat");
    mDownMat = FindObject<Rnd::Mat>(HxStr("act_") + letter + " tar dn.mat");
    mMat = FindObject<Rnd::Mat>(HxStr("act_") + letter + ".mat");
    mTex = FindObject<Rnd::Tex>(HxStr("act_") + letter + ".bmp");
    mMultTex = FindObject<Rnd::Tex>(HxStr("act_mult_") + letter + ".tex");
    mMultMovie = FindObject<Rnd::Animatable>(HxStr("act_mult_") + letter + ".mov");
    SetMultiplied(0);
    ResetTargets();
}

void TnlCatcher::SetMultiplied(int nMultiplied) {
    mMultiplied = nMultiplied;
    if (nMultiplied) {
        mMat->mStages.front().SetTex(mMultTex);
    } else {
        mMat->mStages.front().SetTex(mTex);
    }
    mMat->SyncMat(0);
}

void TnlCatcher::ResetTargets() {
    for (int i = 0; i < kTargetCount; ++i) {
        mTargets[i]->SetMaterial(mUpMat);
    }
}

void TnlCatcher::Hit(int nTarget) {
    if (!mView->GetShowing()) {
        return;
    }
    ResetTargets();
    mTargets[nTarget]->SetMaterial(mDownMat);
    mTarget = nTarget;
    mHitCount = 0;
}

void TnlCatcher::SetAlpha(float flAlpha) {
    mMat->SetAlpha(flAlpha);
}

void TnlCatcher::AttachTo(Rnd::View *pParent) {
    pParent->AddTrans(mView);
    pParent->AddDraw(mView, nullptr);
}

void TnlCatcher::Update(float flFrame) {
    if (mMultiplied) {
        mMultMovie->SetFrame(flFrame);
    }
    ++mUpdateCount;
    if (++mHitCount == kResetHitCount) {
        ResetTargets();
    }
    if (0.0f <= flFrame) {
        int nHitFrame = mHitCount * kHitFramesPerUpdate;
        if (kFramesPerTarget <= nHitFrame) {
            nHitFrame = kFramesPerTarget;
        }
        mView->SetFrame(static_cast<float>(mTarget * kFramesPerTarget) +
                        static_cast<float>(nHitFrame));
    }
}
