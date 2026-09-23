#include "app/renderer.h"

#include "app/apptunnel.h"
#include "app/overlay.h"
#include "game/tnlarena.h"
#include "msg/barstatusmsg.h"
#include "msg/gamebeginmsg.h"
#include "msg/pointamountmsg.h"
#include "os/hxstr.h"
#include "rnd/asyncloader.h"
#include "rnd/view.h"
#include "sch/tickclock.h"
#include "script/scripthost.h"

namespace {

// Script template GameBeginMsg runs.
constexpr int kGameBeginScriptTemplate = 1000;

// Loads PollCommon() averages over.
constexpr int kCommonLoadCount = 3;

// Loads PollLevel() and IsLevelLoaded() poll.
constexpr int kLevelLoadCount = 2;

// Weight of each of the two loads PollLevel() averages over.
constexpr float kLevelLoadWeight = 0.5f;

} // namespace

Renderer *g_pRenderer;
int g_nLsdMode;
RndAsyncLoader *g_pTunnelLoader;
RndAsyncLoader *g_pLaunchLoader;
RndAsyncLoader *g_pHudLoader;
RndAsyncLoader *g_pArenaLoader;
RndAsyncLoader *g_pLevelLoader;
HxStr g_arenaName("");
HxStr g_levelName("");

Renderer::~Renderer() {
    g_pRenderer = nullptr;
    delete mArena;
    delete mOverlay;
    delete mTunnel;
}

void Renderer::HandleMessage(Message *pMsg) {
    int nType = pMsg->Type();
    if (nType == g_nGameBeginMsgType) {
        OnGameBegin();
    } else if (nType == g_nBarStatusMsgType) {
        OnBarStatus(static_cast<BarStatusMsg *>(pMsg));
    } else if (nType == g_nPointAmountMsgType) {
        OnPointAmount(pMsg);
    } else {
        Send(pMsg);
    }
}

void Renderer::OnUnknownSlot6() {
    mSongTick = static_cast<float>(mSongClock->SongTick());
    RendererBase::OnUnknownSlot6();
}

void Renderer::OnUnknownSlot7() {
    mTunnel->SetFrame(mSongTick);
    mOverlay->SetFrame(mSongTick);
    mArena->SetFrame(mSongTick);
    mOuterView->SetFrame(mSongTick);
    mTunnelView->SetFrame(mSongTick);
    mHudView->SetFrame(mSongTick);
    mOuterView->UpdateWorldXfm(nullptr, 0);  // Yes, the binary discards the result.
    mTunnelView->UpdateWorldXfm(nullptr, 0); // Yes, the binary discards the result.
    mHudView->UpdateWorldXfm(nullptr, 0);    // Yes, the binary discards the result.
}

Renderer::Cell *Renderer::GetCell(int nTrack, int nBar) {
    int nSlice = nBar % mCellsPerRow;
    if (nSlice < 0) {
        nSlice += mCellsPerRow;
    }
    return &mCells[nTrack * mCellsPerRow + nSlice];
}

void Renderer::UnloadLevel() {
    delete g_pLevelLoader;
    g_pLevelLoader = nullptr;
    g_levelName = "";

    delete g_pArenaLoader;
    g_pArenaLoader = nullptr;
    g_arenaName = "";
}

int Renderer::PollCommon(float *pflProgress) {
    if (g_pTunnelLoader == nullptr) {
        return 0;
    }

    float aflProgress[kCommonLoadCount];
    const int nTunnelDone = g_pTunnelLoader->Poll(&aflProgress[0]);
    const int nLaunchDone = g_pLaunchLoader->Poll(&aflProgress[1]);
    const int nHudDone = g_pHudLoader->Poll(&aflProgress[2]);
    *pflProgress =
        (aflProgress[0] + aflProgress[1] + aflProgress[2]) / static_cast<float>(kCommonLoadCount);
    return nTunnelDone != 0 && nLaunchDone != 0 && nHudDone != 0;
}

int Renderer::PollLevel(float *pflProgress) {
    if (g_pArenaLoader == nullptr) {
        return 0;
    }

    float aflProgress[kLevelLoadCount];
    const int nArenaDone = g_pArenaLoader->Poll(&aflProgress[0]);
    const int nLevelDone = g_pLevelLoader->Poll(&aflProgress[1]);
    *pflProgress = (aflProgress[0] + aflProgress[1]) * kLevelLoadWeight;
    return nArenaDone != 0 && nLevelDone != 0;
}

int Renderer::IsLevelLoaded(const HxStr &arena, const HxStr &level) {
    if (!(g_arenaName == arena) || !(g_levelName == level)) {
        return 0;
    }
    if (g_pArenaLoader == nullptr) {
        return 0;
    }

    float aflProgress[kLevelLoadCount];
    const int nArenaDone = g_pArenaLoader->Poll(&aflProgress[0]);
    const int nLevelDone = g_pLevelLoader->Poll(&aflProgress[1]);
    return nArenaDone != 0 && nLevelDone != 0;
}

void Renderer::OnGameBegin() {
    CallScriptTemplate(kGameBeginScriptTemplate);
}
