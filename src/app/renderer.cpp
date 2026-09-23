#include "app/renderer.h"

#include "app/application.h"
#include "app/apptunnel.h"
#include "app/mainloop.h"
#include "app/overlay.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "game/grooveworld.h"
#include "game/nullplayer.h"
#include "game/player.h"
#include "game/tnlarena.h"
#include "gfx/gfxdevice.h"
#include "gfx/vramtable.h"
#include "msg/barstatusmsg.h"
#include "msg/gamebeginmsg.h"
#include "msg/pointamountmsg.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "os/zone.h"
#include "rnd/asyncloader.h"
#include "rnd/environ.h"
#include "rnd/manager.h"
#include "rnd/tunnel.h"
#include "rnd/view.h"
#include "sch/tickclock.h"
#include "script/configquery.h"
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

// Blend factor and texture inset of the frame feedback g_nLsdMode draws.
constexpr float kLsdFeedbackAlpha = 0.9f;
constexpr int kLsdFeedbackInset = 100;

// Full-scale time of the subsystem timing graph, in milliseconds.
constexpr int kTimingGraphFullScaleMs = 50;

// Configuration codes of the arena and level names the constructor waits for.
constexpr int kArenaNameConfigCode = 0x27c;
constexpr int kLevelNameConfigCode = 0x278;

// Configuration codes of the two debug overlays.
constexpr int kTimingGraphConfigCode = 0x397;
constexpr int kRenderStatsConfigCode = 0x3a2;

// Bar and powerup a cell starts with. No BarStatusMsg reports either value.
constexpr int kNoBar = -1;
constexpr int kNoPowerup = -1;

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

Renderer::Renderer()
    : mSongClock(Application::shared()->GetSongClock()), mSongTick(0.0f), mDrawTimingGraph(0),
      mDrawRenderStats(0), mCellsPerRow(0), mRowCount(0), mTunnel(nullptr), mOverlay(nullptr),
      mArena(nullptr), mLeader(nullptr) {
    HxStr arena;
    HxStr level;
    float flProgress;

    g_nLsdMode = 0;
    MainLoop::PumpTimers();

    do {
        RndAsyncLoader::PollAsyncLoads();
    } while (PollCommon(&flProgress) == 0);
    PollCommon(&flProgress); // Yes, the binary polls once more and discards the result.

    QueryConfigString(&arena, kArenaNameConfigCode);
    QueryConfigString(&level, kLevelNameConfigCode);
    int nLoaded;
    do {
        RndAsyncLoader::PollAsyncLoads();
        nLoaded = IsLevelLoaded(arena, level);
        PollLevel(&flProgress); // Yes, the binary discards this result.
    } while (nLoaded == 0);
    IsLevelLoaded(arena, level); // Yes, the binary tests once more and discards the result.

    Rnd::Environ *pEnviron = dynamic_cast<Rnd::Environ *>(Rnd::g_manager.Find(HxStr("outer.env")));
    g_vramTable.Clear(1);
    g_gfxDevice.SetClearColor(pEnviron->mFogColor);

    mOuterView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr("outer.view")));
    mTunnelView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr("tnl.view")));
    mHudView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr("hud.view")));

    const int nLocalViews = Application::shared()->GetWorld()->mLocalPlayers.size();
    for (int i = 0; i < nLocalViews; ++i) {
        Rnd::View *pView = dynamic_cast<Rnd::View *>(
            Rnd::g_manager.Find(HxStr(FormatString("tnl local%d.view", i + 1))));
        mLocalViews.push_back(pView);
    }

    mDrawTimingGraph = QueryConfigFlag(kTimingGraphConfigCode);
    mDrawRenderStats = QueryConfigFlag(kRenderStatsConfigCode);

    mTunnel = new AppTunnel(this);
    mOverlay = new Overlay(this);
    mArena = new TnlArena(this);
    AddSink(mTunnel);
    AddSink(mOverlay);
    AddSink(mArena);

    Rnd::Tunnel *pTunnel = dynamic_cast<Rnd::Tunnel *>(Rnd::g_manager.Find(HxStr("tunnel")));
    mCellsPerRow = pTunnel->mSliceCount;
    mRowCount = pTunnel->mRingCount;

    Cell fill;
    fill.mEffects = 0; // Yes, the binary sets no other field of the fill value.
    mCells.resize(mCellsPerRow * mRowCount, fill);
    for (int i = 0; i < mCellsPerRow * mRowCount; ++i) {
        Cell &cell = mCells[i];
        cell.mBar = kNoBar;
        cell.mPlayer = &g_nullPlayer;
        cell.mEnabled = 0;
        cell.mPowerup = kNoPowerup;
        cell.mEffects = 0;
    }

    g_pRenderer = this;
}

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

void Renderer::OnUnknownSlot8() {
    mOuterView->Draw();

    if (g_nLsdMode != 0) {
        g_gfxDevice.mFeedbackAlpha = kLsdFeedbackAlpha;
        g_gfxDevice.mFeedbackInset = kLsdFeedbackInset;
        g_gfxDevice.mFeedbackRect = GfxDevice::Rect{0.0f, 0.0f, 1.0f, 1.0f};
        g_gfxDevice.SetupGsDrawContext();
    }

    int nView = 0;
    for (std::vector<Rnd::View *>::iterator it = mLocalViews.begin(); it != mLocalViews.end();
         ++it) {
        mTunnel->PrepareLocalView(nView++, mSongTick);
        (*it)->UpdateWorldXfm(nullptr, 0); // Yes, the binary discards the result.
        (*it)->Draw();
    }

    mHudView->Draw();
    mOverlay->Draw();

    if (mDrawTimingGraph != 0) {
        g_gfxDevice.DrawSubsystemTimingGraph(kTimingGraphFullScaleMs);
    }
    if (mDrawRenderStats != 0) {
        g_gfxDevice.DrawRenderStatsOverlay();
    }
}

Renderer::Cell *Renderer::GetCell(int nTrack, int nBar) {
    int nSlice = nBar % mCellsPerRow;
    if (nSlice < 0) {
        nSlice += mCellsPerRow;
    }
    return &mCells[nTrack * mCellsPerRow + nSlice];
}

void Renderer::LoadCommon() {
    if (g_pTunnelLoader != nullptr) {
        return;
    }

    const int nZone = FindZoneByName("rndCommon");
    ZoneResetZone(nZone);
    g_pTunnelLoader = new RndAsyncLoader(HxStr("tunnel/"), HxStr("tunnel_new.rnd"), nZone);
    g_pLaunchLoader = new RndAsyncLoader(HxStr("tunnel/"), HxStr("launch.rnd"), nZone);
    g_pHudLoader = new RndAsyncLoader(HxStr("hud/"), HxStr("hud.rnd"), nZone);
    g_pTunnelLoader->Enqueue();
    g_pLaunchLoader->Enqueue();
    g_pHudLoader->Enqueue();
}

void Renderer::UnloadCommon() {
    UnloadLevel();

    if (g_pHudLoader != nullptr) {
        g_pHudLoader->Unload();
        delete g_pHudLoader;
        g_pHudLoader = nullptr;
    }
    if (g_pLaunchLoader != nullptr) {
        g_pLaunchLoader->Unload();
        delete g_pLaunchLoader;
        g_pLaunchLoader = nullptr;
    }
    if (g_pTunnelLoader != nullptr) {
        g_pTunnelLoader->Unload();
        delete g_pTunnelLoader;
        g_pTunnelLoader = nullptr;
    }
}

void Renderer::LoadLevel(const GameParams &params) {
    HxStr level(params.mLevelName);
    HxStr arena(params.mArenaName);

    if (level != g_levelName) {
        delete g_pLevelLoader;
        g_pLevelLoader = nullptr;
        delete g_pArenaLoader;
        g_pArenaLoader = nullptr;
    }
    if (arena != g_arenaName) {
        delete g_pArenaLoader;
        g_pArenaLoader = nullptr;
    }

    if (g_pLevelLoader == nullptr) {
        const int nZone = FindZoneByName("rndTnlLevel");
        ZoneResetZone(nZone);
        g_pLevelLoader =
            new RndAsyncLoader(HxStr("levels/") + level + "/images/", HxStr("images.rnd"), nZone);
        g_pLevelLoader->Enqueue();
        g_levelName = level;
    }

    if (g_pArenaLoader == nullptr) {
        const int nZone = FindZoneByName("rndTnlArena");
        ZoneResetZone(nZone);
        g_pArenaLoader = new RndAsyncLoader(HxStr("arenas/") + arena + "/", arena + ".rnd", nZone);
        g_pArenaLoader->Enqueue();
        g_arenaName = arena;
    }
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

void Renderer::OnBarStatus(BarStatusMsg *pMsg) {
    bool bOverlayChanged = false;
    bool bTunnelChanged = false;
    const int nBar = pMsg->mBar;
    const int nTrack = pMsg->mTrack;
    Cell *pCell = GetCell(nTrack, nBar);

    if (pCell->mBar != nBar) {
        pCell->mBar = nBar;
        bOverlayChanged = true;
        bTunnelChanged = true;
    }

    if ((pMsg->mFlags & BarStatusMsg::kFieldEffects) != 0 &&
        pCell->mEffects != pMsg->GetEffects()) {
        bOverlayChanged = true;
        pCell->mEffects = pMsg->GetEffects();
    }

    if ((pMsg->mFlags & BarStatusMsg::kFieldPlayer) != 0 && pCell->mPlayer != pMsg->GetPlayer()) {
        bTunnelChanged = true;
        pCell->mPlayer = pMsg->GetPlayer();
    }

    if ((pMsg->mFlags & BarStatusMsg::kFieldPowerup) != 0 &&
        pCell->mPowerup != pMsg->GetPowerup()) {
        bTunnelChanged = true;
        pCell->mPowerup = pMsg->GetPowerup();
    }

    if ((pMsg->mFlags & BarStatusMsg::kFieldEnabled) != 0 &&
        pCell->mEnabled != pMsg->GetEnabled()) {
        bTunnelChanged = true;
        pCell->mEnabled = pMsg->GetEnabled();
    }

    if (bOverlayChanged) {
        mOverlay->OnBarChanged(nTrack, nBar, pCell->mEffects);
    }
    if (bTunnelChanged) {
        mTunnel->OnBarChanged(
            nTrack, nBar, pMsg->mUnknown14, pCell->mPlayer, pCell->mPowerup, pCell->mEnabled);
    }
}

void Renderer::OnPointAmount(Message *pMsg) {
    Send(pMsg);

    if (Application::shared()->GetGameMode() == kGameModeSolo) {
        return;
    }
    if (Application::shared()->GetPlayMode() == kPlayModeJam) {
        return;
    }

    int nTopScore = 0;
    Player *pLeader = nullptr;
    std::vector<Player *> &players = Application::shared()->GetWorld()->mPlayers;
    for (std::vector<Player *>::iterator it = players.begin(); it != players.end(); ++it) {
        if (nTopScore < (*it)->GetScore()) {
            pLeader = *it;
            nTopScore = (*it)->GetScore();
        } else if ((*it)->GetScore() == nTopScore) {
            pLeader = nullptr;
        }
    }

    if (pLeader == mLeader) {
        return;
    }
    mTunnel->OnLeaderChanged(mLeader, pLeader);
    mOverlay->OnLeaderChanged(mLeader, pLeader);
    mLeader = pLeader;
}

void Renderer::OnGameBegin() {
    CallScriptTemplate(kGameBeginScriptTemplate);
}
