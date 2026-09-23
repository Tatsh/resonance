#include "game/tnlarena.h"

#include <list>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/grooveworld.h"
#include "game/multiscreenanim.h"
#include "game/player.h"
#include "game/screenanim.h"
#include "game/soloscreenanim.h"
#include "msg/juiceamountmsg.h"
#include "msg/message.h"
#include "msg/pointamountmsg.h"
#include "msg/winmsg.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "script/configquery.h"

namespace {

// The level the constructor starts at and the destructor passes before deleting the animation.
constexpr int kNeutralLevel = 1;

// mUnknown24 at construction, and the value LockLevel() stores.
constexpr int kNoJuiceLock = -1;
constexpr int kJuiceLocked = 1;

// Configuration code whose flag selects the plain ScreenAnim.
constexpr int kDisplayModeConfigCode = 0x3a1;

// The arena screens use `screen01.mat` through `screen04.mat`.
constexpr int kScreenCount = 4;

// A solo player's juice above kHighJuice selects kHighLevel, and below kLowJuice kLowLevel.
constexpr double kHighJuice = 0.85;
constexpr double kLowJuice = 0.2;
constexpr int kHighLevel = 2;
constexpr int kLowLevel = 0;

} // namespace

TnlArena *g_pTnlArena;

void Rnd::Mat::GetMeshReferrers(std::vector<Mesh *> &meshes) {
    for (std::list<Object *>::iterator it = mRefs.begin(); it != mRefs.end(); ++it) {
        if ((*it)->ClassName() == "Mesh") {
            meshes.push_back(dynamic_cast<Mesh *>(*it));
        }
    }
}

TnlArena::TnlArena(Renderer *) {
    mGameMode = Application::shared()->GetGameMode();
    mLevel = kNeutralLevel;
    mUnknown24 = kNoJuiceLock;
    g_pTnlArena = this;

    const int nPlain = QueryConfigFlag(kDisplayModeConfigCode);
    for (int i = 1; i <= kScreenCount; ++i) {
        Rnd::Mat *pMat =
            dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr(FormatString("screen0%d.mat", i))));
        std::vector<Rnd::Mesh *> meshes;
        pMat->GetMeshReferrers(meshes);
        for (std::vector<Rnd::Mesh *>::iterator it = meshes.begin(); it != meshes.end(); ++it) {
            // The temporary's destructor puts the material back on the mesh, as the binary does.
            ScreenMesh screen{*it, (*it)->mMat};
            mScreenMeshes.push_back(screen);
        }
    }

    std::vector<Player *> &players = Application::shared()->GetWorld()->mPlayers;
    const int nPlayers = players.size();
    for (int i = 0; i < nPlayers; ++i) {
        mPlayerMaterials.push_back(new PlayerMaterial(players[i]));
    }

    if (nPlain != 0) {
        mScreenAnim = new ScreenAnim;
    } else if (mGameMode == kGameModeSolo) {
        mScreenAnim = new SoloScreenAnim(&mScreenMeshes, mPlayerMaterials[0]->mMat);
    } else {
        mScreenAnim = new MultiScreenAnim(&mScreenMeshes, &mPlayerMaterials);
    }

    const int nPlayMode = Application::shared()->GetPlayMode();
    if (nPlayMode == kPlayModeJam) {
        mLevel = nPlayMode;
    }
    mScreenAnim->SetLevel(mLevel);
}

TnlArena::PlayerMaterial::PlayerMaterial(Player *pPlayer) : mPlayer(pPlayer) {
    mMat = dynamic_cast<Rnd::Mat *>(
        Rnd::g_manager.Find(HxStr(FormatString("HUD freq%d.mat", pPlayer->mId20))));
}

void TnlArena::ScreenMesh::SetMaterial(Rnd::Mat *pMat) const {
    mMesh->SetMaterial(pMat);
}

TnlArena::~TnlArena() {
    mScreenAnim->SetLevel(kNeutralLevel);
    g_pTnlArena = nullptr;
    delete mScreenAnim;
    for (std::vector<PlayerMaterial *>::iterator it = mPlayerMaterials.begin();
         it != mPlayerMaterials.end();
         ++it) {
        delete *it;
    }
    // Yes, the binary restores every material here and again as mScreenMeshes is destroyed.
    for (std::vector<ScreenMesh>::iterator it = mScreenMeshes.begin(); it != mScreenMeshes.end();
         ++it) {
        it->SetMaterial(it->mMat);
    }
}

void TnlArena::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == g_nPointAmountMsgType) {
        OnPointAmount(static_cast<PointAmountMsg *>(pMsg));
    } else if (nType == g_nJuiceAmountMsgType) {
        OnJuiceAmount(static_cast<JuiceAmountMsg *>(pMsg));
    } else if (nType == g_nWinMsgType) {
        OnWin(static_cast<WinMsg *>(pMsg));
    }
}

// 0x0040caf8
inline void TnlArena::OnPointAmount([[maybe_unused]] PointAmountMsg *pMsg) {
    mScreenAnim->UpdateLeaders();
}

// 0x0040ca00
inline void TnlArena::OnJuiceAmount(JuiceAmountMsg *pMsg) {
    if (mUnknown24 != kNoJuiceLock) {
        return;
    }
    if (mGameMode != kGameModeSolo || Application::shared()->GetPlayMode() != kPlayModeGame) {
        return;
    }

    const float flJuice = pMsg->GetJuiceFraction();
    mLevel = kNeutralLevel;
    if (flJuice > kHighJuice) {
        mLevel = kHighLevel;
    } else if (flJuice < kLowJuice) {
        mLevel = kLowLevel;
    }
    mScreenAnim->SetLevel(mLevel);
}

// 0x0040cb28
inline void TnlArena::OnWin(WinMsg *pMsg) {
    if (mGameMode == kGameModeSolo && pMsg->mWinners.size() != 0) {
        mScreenAnim->SetLevel(mLevel + 1);
    }
}

// 0x0040c988
void TnlArena::LockLevel() {
    mUnknown24 = kJuiceLocked;
    mScreenAnim->SetLevel(kNeutralLevel);
}

void TnlArena::SetFrame(float flFrame) {
    mScreenAnim->SetFrame(flFrame);
}
