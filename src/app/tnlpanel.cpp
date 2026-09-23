#include "app/tnlpanel.h"

#include "app/application.h"
#include "app/apptunnel.h"
#include "app/tnlutil.h"
#include "app/tunnelcache.h"
#include "game/gamemanagerimpl.h"
#include "game/player.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/meshanim.h"
#include "rnd/tunnel.h"

namespace {

// Start frame of a panel no caller has scheduled, far beyond a song's length.
constexpr float kNoFrame = 1e9f;

// mPowerbar of a plain lane.
constexpr int kNoPowerbar = -1;

// Lane materials "tunnel mat0" through "tunnel mat3" and "powerbar mat0" through "powerbar mat3".
constexpr int kLaneMatCount = 4;

// Alpha of the non-lane panels.
constexpr float kPanelAlpha = 0.5f;

constexpr float kAnimFrames = 480.0f;
constexpr float kFxFrames = 200.0f;

inline Rnd::Mat *FindMat(const char *pszName) {
    return dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr(pszName)));
}

inline Rnd::MeshAnim *FindMeshAnim(const char *pszName) {
    return dynamic_cast<Rnd::MeshAnim *>(Rnd::g_manager.Find(HxStr(pszName)));
}

} // namespace

TnlPanel::TnlPanel(
    int nRing, int nSlice, Player *pPlayer, int nPowerbar, Kind kind, int nShowing, int nColorIndex)
    : mRing(nRing), mSlice(nSlice), mPlayer(pPlayer), mPowerbar(nPowerbar), mKind(kind),
      mShowing(nShowing), mColorIndex(nColorIndex), mMat(nullptr), mStartFrame(kNoFrame),
      mDuration(0.0f), mAnim(nullptr) {
    mSection = GetCachedTunnelObject()->GetRingSection(nRing, nSlice);
    Refresh();
}

void TnlPanel::Refresh() {
    const int nIndex = mColorIndex % kLaneMatCount;
    if (mPowerbar == kNoPowerbar) {
        mMat = FindMat(FormatString("tunnel mat%d", nIndex));
    } else {
        mMat = FindMat(FormatString("powerbar mat%d", nIndex));
    }
    mColor = Color{1.0f, 1.0f, 1.0f, 1.0f};

    switch (mKind) {
    case kKindAxe:
        mMat = FindMat("panel axe mat");
        mColor = Color{1.0f, 1.0f, 1.0f, kPanelAlpha};
        break;
    case kKindScratch:
        mMat = FindMat("panel scratch mat");
        mColor = Color{1.0f, 1.0f, 1.0f, kPanelAlpha};
        break;
    case kKindVox:
        mMat = FindMat("panel vox mat");
        mColor = Color{1.0f, 1.0f, 1.0f, kPanelAlpha};
        break;
    default:
        break;
    }
}

void TnlPanel::Apply() {
    int nShowing = mShowing;
    if (Application::shared()->GetPlayMode() == kPlayModeGame && mKind == kKindLane &&
        mPlayer->IsNull() == 0) {
        nShowing = 0;
    }
    mSection->SetShowing(nShowing);
    if (nShowing != 0) {
        mSection->SetMaterialChain(mMat);
        mSection->SetVertexColor(mColor);
    }
    GetCachedTunnelObject()->SetLaneDividerColor(
        mRing, mSlice, TnlLaneColorFromName(mPlayer->mColorName));
}

int TnlPanel::Update(float flFrame, AppTunnel *pTunnel) {
    if (mStartFrame <= flFrame && mDuration == 0.0f) {
        if (mKind != kKindLane) {
            const int nWasShowing = mSection->GetShowing();
            Apply();
            if (mShowing == 0 || nWasShowing != 0) {
                return 0;
            }
            mAnim = FindMeshAnim("panel show.msnm");
            mDuration = kAnimFrames;
        } else if (mPlayer->IsNull() != 0) {
            mAnim = nullptr;
            mDuration = kFxFrames;
            if (Application::shared()->GetGameMode() == kGameModeSolo) {
                pTunnel->StartPanelFX(mRing, mSlice, 0);
            }
        } else {
            mAnim = FindMeshAnim("panel erase.msnm");
            mDuration = kAnimFrames;
        }
    }

    if (mDuration == 0.0f) {
        return 1;
    }
    const float flElapsed = flFrame - mStartFrame;
    if (mAnim != nullptr) {
        mAnim->SetMesh(mSection);
        mAnim->SetFrame(flElapsed);
    }
    if (mDuration < flElapsed) {
        Apply();
        return 0;
    }
    return 1;
}
