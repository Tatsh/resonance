#include "app/tnlgemmanager.h"

#include "app/tnlgemeffectkind.h"
#include "app/tnlgemmeshkind.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/multimesh.h"

namespace {

// Frames behind the playhead a gem is kept before Update() drops it.
constexpr float kLateWindowFrames = 240.0f;

// Placement limit Update() starts from.
constexpr int kInitialMaxPlaced = 1000;

// A pending flash starts once the playhead is this many frames short of its gem.
constexpr float kFlashLeadFrames = -20.0f;

} // namespace

// NTSC-U/C: 0x006df360, PAL: 0x00722b88
int g_nTnlMeshNameCounter = 1;

// NTSC-U/C: 0x006df364, PAL: 0x00722b8c
float g_flTnlGemLastFrame;

// NTSC-U/C: 0x004122c0, PAL: 0x0044bdc0
TnlGemManager::TnlGemManager(AppTunnel *pTunnel, float flCostBudget)
    : mTunnel(pTunnel), mCostBudget(flCostBudget), mLateWindow(kLateWindowFrames),
      mMaxPlaced(kInitialMaxPlaced) {
}

// NTSC-U/C: 0x00412490, PAL: 0x0044bf90
TnlGemManager::~TnlGemManager() {
    for (auto *pKind : mMeshKinds) {
        delete pKind;
    }
    for (auto *pKind : mEffectKinds) {
        delete pKind;
    }
}

// NTSC-U/C: 0x00412628, PAL: 0x0044c128
char TnlGemManager::AddMeshKind(const char *pszName, float flLodOffset, float flCostScale) {
    char nKind = mMeshKinds.size();
    TnlGemMeshKind *pPrevious = nullptr;
    for (int i = 0;; ++i) {
        auto *pMesh = dynamic_cast<Rnd::MultiMesh *>(
            Rnd::TheManager.Find(HxStr(Rnd::MakeString("%s%d.mm", pszName, i))));
        if (!pMesh) {
            break;
        }
        auto *pLevel = new TnlGemMeshKind(pMesh, flLodOffset, flCostScale);
        mMeshKinds.push_back(pLevel);
        if (pPrevious) {
            pPrevious->mNext = pLevel;
        }
        pPrevious = pLevel;
    }
    return nKind;
}

// NTSC-U/C: 0x00412878, PAL: 0x0044c3a0
char TnlGemManager::AddEffectKind(const char *pszName) {
    int nCount = mEffectKinds.size();
    mEffectKinds.push_back(new TnlGemEffectKind(pszName));
    return nCount + kEffectKindBase;
}

// NTSC-U/C: 0x00412968, PAL: 0x0044c490
void TnlGemManager::Add(const TnlGem &gem) {
    auto it = FindFirstAt(mGems, gem.mFrame);
    while ((it != mGems.end()) && (it->mFrame <= gem.mFrame)) {
        if ((it->mFrame == gem.mFrame) && (it->mTrack == gem.mTrack) &&
            (it->mBlend == gem.mBlend)) {
            it->mExpireFrame = gem.mAppearFrame;
        }
        ++it;
    }
    mGems.insert(it, gem);
}

// NTSC-U/C: 0x00412b18, PAL: 0x0044c640
void TnlGemManager::RemoveRange(char nTrack, float flStart, float flEnd) {
    auto it = FindFirstAt(mGems, flStart);
    while ((it != mGems.end()) && (it->mFrame < flEnd)) {
        if (it->mTrack == nTrack) {
            it->Release();
            it = mGems.erase(it);
        } else {
            ++it;
        }
    }
}

// NTSC-U/C: 0x00412c50, PAL: 0x0044c778
void TnlGemManager::Remove(char nTrack, float flFrame, float flBlend) {
    auto it = FindFirstAt(mGems, flFrame);
    while ((it != mGems.end()) && (it->mFrame == flFrame)) {
        if ((it->mTrack == nTrack) && (it->mBlend == flBlend)) {
            it->Release();
            it = mGems.erase(it);
        } else {
            ++it;
        }
    }
}

// NTSC-U/C: 0x00412db0, PAL: 0x0044c8d8
void TnlGemManager::Update(float flFrame) {
    (void)mGems.size(); // Yes, the binary walks the whole list and discards the count.
    auto it = mGems.begin();
    while ((it != mGems.end()) && ((flFrame - it->mFrame) > mLateWindow)) {
        it->Release();
        it = mGems.erase(it);
    }
    int nPlaced = 0;
    float flCost = 0.0f;
    while (it != mGems.end()) {
        if (it->mExpireFrame < flFrame) {
            it->Release();
            it = mGems.erase(it);
            continue;
        }
        if ((it->mState == TnlGem::kStateFlashPending) &&
            ((flFrame - it->mFrame) > kFlashLeadFrames)) {
            it->Flash(mTunnel);
            it->mState = TnlGem::kStateFlashed;
        }
        if ((flCost < mCostBudget) && (nPlaced < mMaxPlaced)) {
            if (it->mAppearFrame < flFrame) {
                flCost += it->Place(this, flFrame);
                ++nPlaced;
            }
        } else {
            it->Release();
        }
        ++it;
    }
    if (nPlaced < mMaxPlaced) {
        mMaxPlaced = nPlaced;
    } else {
        ++mMaxPlaced;
    }
    if (flFrame != g_flTnlGemLastFrame) {
        g_flTnlGemLastFrame = flFrame;
    }
}

// NTSC-U/C: 0x00415a50, PAL: 0x0044f5a0
TnlGemMeshKind *TnlGemManager::GetMeshKind(char nKind) {
    return mMeshKinds[nKind];
}

// NTSC-U/C: 0x00415a68, PAL: 0x0044f5b8
TnlGemEffectKind *TnlGemManager::GetEffectKind(char nKind) {
    // The binary narrows the difference back to a signed char before indexing.
    return mEffectKinds[static_cast<signed char>(nKind - kEffectKindBase)];
}

// NTSC-U/C: 0x00415a88, PAL: 0x0044f5d8
void TnlGemManager::AddKindDraws(char nKind, Rnd::Drawable *pParent) {
    if (!(nKind & kEffectKindBase)) {
        mMeshKinds[nKind]->AddDrawTo(pParent);
    }
}

// NTSC-U/C: 0x00415af8, PAL: 0x0044f648
void TnlGemManager::SetKindShowing(char nKind, int nShowing) {
    if (!(nKind & kEffectKindBase)) {
        mMeshKinds[nKind]->SetShowing(nShowing);
    }
}

// NTSC-U/C: 0x00415e78, PAL: 0x0044f9e0
std::list<TnlGem>::iterator TnlGemManager::FindFirstAt(std::list<TnlGem> &gems, float flFrame) {
    for (auto it = gems.begin(); it != gems.end(); ++it) {
        if (flFrame <= it->mFrame) {
            return it;
        }
    }
    return gems.end();
}

// NTSC-U/C: 0x00415668, PAL: 0x0044f1b8
HxStr NextTnlMeshName() {
    return HxStr(Rnd::MakeString("<tnlmesh%04d>", ++g_nTnlMeshNameCounter));
}
