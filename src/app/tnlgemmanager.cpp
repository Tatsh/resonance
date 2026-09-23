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

// 0x006df360
int g_nTnlMeshNameCounter;

// 0x006df364
float g_flTnlGemLastFrame;

// 0x004122c0
TnlGemManager::TnlGemManager(AppTunnel *pTunnel, float flCostBudget)
    : mTunnel(pTunnel), mCostBudget(flCostBudget), mLateWindow(kLateWindowFrames),
      mMaxPlaced(kInitialMaxPlaced) {
}

// 0x00412490
TnlGemManager::~TnlGemManager() {
    for (auto *pKind : mMeshKinds) {
        delete pKind;
    }
    for (auto *pKind : mEffectKinds) {
        delete pKind;
    }
}

// 0x00412628
char TnlGemManager::AddMeshKind(const char *pszName, float flLodOffset, float flCostScale) {
    char nKind = mMeshKinds.size();
    TnlGemMeshKind *pPrevious = nullptr;
    for (int i = 0;; ++i) {
        auto *pMesh = dynamic_cast<Rnd::MultiMesh *>(
            Rnd::g_manager.Find(HxStr(FormatString("%s%d.mm", pszName, i))));
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

// 0x00412878
char TnlGemManager::AddEffectKind(const char *pszName) {
    int nCount = mEffectKinds.size();
    mEffectKinds.push_back(new TnlGemEffectKind(pszName));
    return nCount + kEffectKindBase;
}

// 0x00412968
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

// 0x00412b18
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

// 0x00412c50
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

// 0x00412db0
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

// 0x00415a50
TnlGemMeshKind *TnlGemManager::GetMeshKind(char nKind) {
    return mMeshKinds[nKind];
}

// 0x00415a68
TnlGemEffectKind *TnlGemManager::GetEffectKind(char nKind) {
    return mEffectKinds[nKind - kEffectKindBase];
}

// 0x00415a88
void TnlGemManager::AddKindDraws(char nKind, Rnd::Drawable *pParent) {
    if (!(nKind & kEffectKindBase)) {
        mMeshKinds[nKind]->AddDrawTo(pParent);
    }
}

// 0x00415af8
void TnlGemManager::SetKindShowing(char nKind, int nShowing) {
    if (!(nKind & kEffectKindBase)) {
        mMeshKinds[nKind]->SetShowing(nShowing);
    }
}

// 0x00415e78
std::list<TnlGem>::iterator TnlGemManager::FindFirstAt(std::list<TnlGem> &gems, float flFrame) {
    for (auto it = gems.begin(); it != gems.end(); ++it) {
        if (flFrame <= it->mFrame) {
            return it;
        }
    }
    return gems.end();
}

// 0x00415668
HxStr NextTnlMeshName() {
    return HxStr(FormatString("<tnlmesh%04d>", ++g_nTnlMeshNameCounter));
}
