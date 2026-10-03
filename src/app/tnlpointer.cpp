#include "app/tnlpointer.h"

#include <cstring>

#include "math/vector3.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/transformable.h"
#include "rnd/view.h"

namespace {

enum MeshPairIndex { kPairAxe = 0, kPairScratch = 1, kPairVoice = 2, kPairCount = 3 };

// Instrument kinds SetKind() recognises. The meaning is inferred from the mesh names.
enum InstrumentKind { kKindAxe = 1, kKindScratch = 3, kKindVoice = 4 };

enum XfmRow { kTranslationRow = 3 };

constexpr float kUnsetTime = 1e9f;
constexpr float kRestartPending = -1e9f;
constexpr float kSpinDuration = 300.0f;

// The dip ramp maps raw 0 to the rest height and raw 1 to the dipped height over kDipDuration.
constexpr float kRestHeight = 0.1f;
constexpr float kDippedHeight = 0.0f;
constexpr float kDipDuration = 60.0f;

constexpr float kCentreOffset = 0.5f;
constexpr double kLaneCentre = 0.5;
constexpr double kLaneToOffset = -0.5;

} // namespace

// NTSC-U/C: 0x00455508, PAL: 0x00492a38
void TnlPointer::MeshPair::Init(const HxStr &iconName, const HxStr &baseName) {
    mIcon = dynamic_cast<Rnd::Mesh *>(Rnd::TheManager.Find(iconName));
    mBase = dynamic_cast<Rnd::Mesh *>(Rnd::TheManager.Find(baseName));
}

// NTSC-U/C: 0x004555e0, PAL: 0x00492b10
void TnlPointer::MeshPair::SetShowing(int nShowing) {
    mIcon->SetShowing(nShowing);
    mBase->SetShowing(nShowing);
}

// NTSC-U/C: 0x00455640, PAL: 0x00492b70
void TnlPointer::MeshPair::SetAlpha(float flAlpha) {
    mIcon->mMat->SetAlpha(flAlpha);
}

// NTSC-U/C: 0x0043a810, PAL: 0x00476b10
TnlPointer::TnlPointer(const HxStr &colorName)
    : mLastTime(0.0f), mSpinFrame(0.0f), mSpinning(0), mOffsetX(kCentreOffset),
      mCentreOffset(kCentreOffset), mSpinStart(kUnsetTime) {
    const HxStr prefix = HxStr("ptr_") + colorName[0];
    mView = dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(prefix));
    mSpinView = dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(prefix + "_spin.view"));
    mPairs.resize(kPairCount);
    mPairs[kPairAxe].Init(prefix + "_axe.mesh", prefix + "_plate.mesh");
    mPairs[kPairScratch].Init(prefix + "_scratch.mesh", prefix + "_plate.mesh");
    mPairs[kPairVoice].Init(prefix + "_vox.mesh", prefix + "_tgt.mesh");
    mDip.SetParams(kRestHeight, kDippedHeight, kDipDuration);
    Reset();
}

// NTSC-U/C: 0x0043b248, PAL: 0x00477768
void TnlPointer::SetKind(int nKind) {
    for (unsigned i = 0; i < mPairs.size(); ++i) {
        mPairs[i].SetShowing(0);
    }
    switch (nKind) {
    case kKindAxe:
        mPairs[kPairAxe].SetShowing(1);
        break;
    case kKindScratch:
        mPairs[kPairScratch].SetShowing(1);
        break;
    case kKindVoice:
        mPairs[kPairVoice].SetShowing(1);
        break;
    default:
        break;
    }
    Reset();
}

// NTSC-U/C: 0x004557d0, PAL: 0x00492d00
void TnlPointer::Reset() {
    mDip.SetTarget(0.0f);
    mSpinning = 0;
    mSpinStart = kUnsetTime;
}

// NTSC-U/C: 0x00455768, PAL: 0x00492c98
void TnlPointer::Spin(int nRestart) {
    if (!mView->GetShowing()) {
        return;
    }
    if (nRestart) {
        mSpinStart = kRestartPending;
    }
    mDip.Jump(1.0f);
    mSpinning = 1;
}

// NTSC-U/C: 0x00455810, PAL: 0x00492d40
void TnlPointer::SetLane(float flLane) {
    mOffsetX = static_cast<float>((flLane - kLaneCentre) * kLaneToOffset);
}

// NTSC-U/C: 0x004556c8, PAL: 0x00492bf8
void TnlPointer::SetAlpha(float flAlpha) {
    for (unsigned i = 0; i < mPairs.size(); ++i) {
        mPairs[i].SetAlpha(flAlpha);
    }
}

// NTSC-U/C: 0x00455670, PAL: 0x00492ba0
void TnlPointer::AttachTo(Rnd::View *pParent) {
    pParent->AddTrans(mView);
    pParent->AddDraw(mView, nullptr);
}

// NTSC-U/C: 0x00455860, PAL: 0x00492d90
void TnlPointer::Update(float flTime) {
    mDip.Execute(flTime);
    if (mSpinStart == kRestartPending) {
        mSpinStart = flTime;
    }
    if (kSpinDuration < flTime - mSpinStart) {
        Reset();
    }
    const Vector3 position{mOffsetX, 0.0f, mDip.Val(), 1.0f};
    Rnd::Transformable &trans = *mSpinView;
    std::memcpy(trans.mLocalXfm[kTranslationRow], &position, sizeof(position));
    trans.mDirty = 1;
    const float flElapsed = flTime - mLastTime;
    mLastTime = flTime;
    if (mSpinning) {
        mSpinFrame += flElapsed;
    }
    mView->SetFrame(mSpinFrame);
}
