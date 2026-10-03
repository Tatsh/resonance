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

// mJuiceLock at construction, and the value LockLevel() stores.
constexpr int kNoJuiceLock = -1;
constexpr int kJuiceLocked = 1;

// Configuration code whose flag selects the plain ScreenAnim.
constexpr int kDisplayModeConfigCode = 929;

// The arena screens use `screen01.mat` through `screen04.mat`.
constexpr int kScreenCount = 4;

// A solo player's juice above kHighJuice selects kHighLevel, and below kLowJuice kLowLevel.
constexpr double kHighJuice = 0.85;
constexpr double kLowJuice = 0.2;
constexpr int kHighLevel = 2;
constexpr int kLowLevel = 0;

} // namespace

TnlArena *g_pTnlArena;

// NTSC-U/C: 0x00406010, PAL: 0x0043f910
void Rnd::Mat::GetMeshReferrers(std::vector<Mesh *> &meshes) {
    for (std::list<Object *>::iterator it = mRefs.begin(); it != mRefs.end(); ++it) {
        if ((*it)->ClassName() == "Mesh") {
            meshes.push_back(dynamic_cast<Mesh *>(*it));
        }
    }
}

// NTSC-U/C: 0x004067e8, PAL: 0x00440128
TnlArena::TnlArena(Renderer *) {
    mGameMode = Application::shared()->GetGameMode();
    mLevel = kNeutralLevel;
    mJuiceLock = kNoJuiceLock;
    g_pTnlArena = this;

    const int nPlain = QueryConfigFlag(kDisplayModeConfigCode);
    for (int i = 1; i <= kScreenCount; ++i) {
        Rnd::Mat *pMat =
            dynamic_cast<Rnd::Mat *>(Rnd::TheManager.Find(HxStr(FormatString("screen0%d.mat", i))));
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

// NTSC-U/C: 0x00406120, PAL: 0x0043fa20
TnlArena::PlayerMaterial::PlayerMaterial(Player *pPlayer) : mPlayer(pPlayer) {
    mMat = dynamic_cast<Rnd::Mat *>(
        Rnd::TheManager.Find(HxStr(FormatString("HUD freq%d.mat", pPlayer->mPlayerId))));
}

// NTSC-U/C: 0x0040c938, PAL: 0x00446360
void TnlArena::ScreenMesh::SetMaterial(Rnd::Mat *pMat) const {
    mMesh->SetMat(pMat);
}

// NTSC-U/C: 0x00406de0, PAL: 0x00440740
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

// NTSC-U/C: 0x00406ff0, PAL: 0x00440950
void TnlArena::DispatchPriv(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == g_nPointAmountMsgType) {
        OnPointAmount(static_cast<PointAmountMsg *>(pMsg));
    } else if (nType == g_nJuiceAmountMsgType) {
        OnJuiceAmount(static_cast<JuiceAmountMsg *>(pMsg));
    } else if (nType == g_nWinMsgType) {
        OnWin(static_cast<WinMsg *>(pMsg));
    }
}

// NTSC-U/C: 0x0040caf8, PAL: 0x00446520
inline void TnlArena::OnPointAmount([[maybe_unused]] PointAmountMsg *pMsg) {
    mScreenAnim->UpdateLeaders();
}

// NTSC-U/C: 0x0040ca00, PAL: 0x00446428
inline void TnlArena::OnJuiceAmount(JuiceAmountMsg *pMsg) {
    if (mJuiceLock != kNoJuiceLock) {
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

// NTSC-U/C: 0x0040cb28, PAL: 0x00446550
inline void TnlArena::OnWin(WinMsg *pMsg) {
    if (mGameMode == kGameModeSolo && pMsg->mWinners.size() != 0) {
        mScreenAnim->SetLevel(mLevel + 1);
    }
}

// NTSC-U/C: 0x0040c988, PAL: 0x004463b0
void TnlArena::LockLevel() {
    mJuiceLock = kJuiceLocked;
    mScreenAnim->SetLevel(kNeutralLevel);
}

// NTSC-U/C: 0x0040c958, PAL: 0x00446380
void TnlArena::SetFrame(float flFrame) {
    mScreenAnim->SetFrame(flFrame);
}
