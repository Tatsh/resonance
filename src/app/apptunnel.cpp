#include "app/apptunnel.h"

#include "app/application.h"
#include "app/durgemtrails.h"
#include "app/hudutil.h"
#include "app/renderer.h"
#include "app/tnlarms.h"
#include "app/tnlarrow.h"
#include "app/tnlboundary.h"
#include "app/tnlbumpfx.h"
#include "app/tnlcamerarig.h"
#include "app/tnlcripplefx.h"
#include "app/tnlfirefx.h"
#include "app/tnlgemmanager.h"
#include "app/tnllattice.h"
#include "app/tnlmultfx.h"
#include "app/tnlnowring.h"
#include "app/tnlpanel.h"
#include "app/tnlpanelfx.h"
#include "app/tnlplayer.h"
#include "app/tnlsnake.h"
#include "app/tnlutil.h"
#include "app/tunnelcache.h"
#include "game/gamemanagerimpl.h"
#include "game/grooveworld.h"
#include "game/leveldata.h"
#include "game/player.h"
#include "game/playmap.h"
#include "math/color.h"
#include "math/transform.h"
#include "math/vector3.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/cam.h"
#include "rnd/drawable.h"
#include "rnd/environ.h"
#include "rnd/light.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/object.h"
#include "rnd/particle.h"
#include "rnd/particlesys.h"
#include "rnd/tunnel.h"
#include "rnd/view.h"
#include "script/configquery.h"
#include "script/scripthost.h"

namespace {

// Configuration code whose flag becomes g_nAppTunnelDisplayMode.
constexpr int kDisplayModeConfigCode = 0x3a1;

// Globals::GetTempo() divided by this is the rate "realtime.view" runs at.
constexpr float kTempoToRate = 480000.0f;

// Tunnel frames a seeker takes to change lane, before the tempo rate divides it.
constexpr float kLaneChangeFrames = 400.0f;

// Bounds that take in every scheduled tunnel event.
constexpr float kEarliestEventFrame = -9999999.0f;
constexpr float kLatestEventFrame = 9999999.0f;

// Local players a game can have, each with a "tnl local%d.view" and an "outer cam%d".
constexpr int kMaxLocalPlayers = 4;

// Each local view takes a quarter of the screen, two to a row.
constexpr int kLocalViewsPerRow = 2;
constexpr float kLocalViewSpan = 0.5f;

// Gem trail points, and the cost budget and level of detail offsets of the gem kinds, for one
// local player.
constexpr int kSinglePlayerTrailPoints = 500;
constexpr float kSinglePlayerGemCost = 3000.0f;

// The same for two local players.
constexpr int kTwoPlayerTrailPoints = 400;
constexpr float kTwoPlayerGemCost = 2200.0f;
constexpr float kTwoPlayerHexGemLod = -750.0f;

// The same for three or four local players.
constexpr int kThreePlayerTrailPoints = 400;
constexpr int kFourPlayerTrailPoints = 300;
constexpr float kSplitScreenGemCost = 1250.0f;
constexpr float kSplitScreenGemLod = -1250.0f;

// Local player counts with their own settings.
constexpr int kTwoLocalPlayers = 2;
constexpr int kThreeLocalPlayers = 3;
constexpr int kFourLocalPlayers = 4;

// Screen size threshold of the first tunnel level of detail, alone and split.
constexpr float kSingleScreenFirstLod = 2000.0f;
constexpr float kSplitScreenFirstLod = -5000.0f;

// Far slices the tunnel skips drawing, by local player count.
constexpr int kSingleScreenCulledFarSlices = 2;
constexpr int kTwoPlayerCulledFarSlices = 3;
constexpr int kSplitScreenCulledFarSlices = 4;

// Lane floor brightness alone and split, and the floor colour before the brightness applies.
constexpr float kSingleScreenBrightness = 1.0f;
constexpr float kSplitScreenBrightness = 2.0f;
constexpr float kLaneFloorBlue = 0.4f;

// Hex gem kinds, one per player colour.
constexpr int kHexGemCount = 5;

// Ribbon widths of the vocal and the axe or scratch gem trails.
constexpr float kVoxTrailWidth = 0.1f;
constexpr float kStringTrailWidth = 0.08f;

// Size and colour of a string flare particle before PlaceStringFlare() places it.
constexpr float kStringFlareSize = 0.5f;

// Gem flash records.
constexpr int kGemFlashCount = 30;

// Panel effects, fires of each kind, cripplers, bumpers, snakes, and arrows.
constexpr int kPanelFXCount = 8;
constexpr int kFireCount = 2;
constexpr int kFireFsCount = 2;
constexpr int kFireMultCount = 4;
constexpr int kCrippleFXCount = 2;
constexpr int kBumpFXCount = 3;
constexpr int kSnakeCount = 4;
constexpr int kArrowCount = 8;

// TnlFireFX indices of the plain and the full-screen fires.
constexpr int kFireIndex = -1;
constexpr int kFireFsIndex = -2;

// Effect gem kinds, indexed by TrackData::mInstrument.
enum InstrumentKind {
    kInstrumentDrums = 0,
    kInstrumentBass = 1,
    kInstrumentSynth = 2,
    kInstrumentGuitar = 3,
    kInstrumentVocal = 4,
    kInstrumentFX = 5,
    kInstrumentCount = 6,
};

// Look an object up by name and cast it to its class, null when either step fails.
template <class T>
inline T *FindObject(const char *pszName) {
    return dynamic_cast<T *>(Rnd::g_manager.Find(HxStr(pszName)));
}

// Rate the ghost material's alpha moves at while a ghost shows, negated while it hides.
constexpr float kGhostFadeRate = 0.15f;

// Start frame of a TnlSnake that is not running.
constexpr float kIdleSnakeFrame = 1e9f;

// Push of a string flare placed on a ring, as the tangent scale of the ring transform.
constexpr float kStringFlareRingScale = 0.97f;

// Alpha SetFrame() takes from each live string flare particle per call.
constexpr float kStringFlareFade = 0.15f;

// Size SetFrame() takes from each gem flash per call.
constexpr float kGemFlashShrink = 0.1f;

// Song frames per bar.
constexpr int kFramesPerBar = 1920;

// OnBarChanged() ignores a bar that ended more than this many frames, a quarter bar, ago.
constexpr float kBarChangeLateFrames = 480.0f;

// A new panel starts this fraction of the way from the song tick to the start of its bar.
constexpr float kPanelLeadFraction = 0.25f;

} // namespace

AppTunnel *g_pAppTunnel;

inline AppTunnel::GemFlash::~GemFlash() {
    if (mParticle != nullptr) {
        mSystem->FreeParticle(mParticle);
    }
}

AppTunnel::AppTunnel(Renderer *pRenderer)
    : mRenderer(pRenderer), mGameMode(Application::shared()->GetGameMode()),
      mPlayMode(Application::shared()->GetPlayMode()), mBoundary(nullptr), mCameraRig(nullptr),
      mJukebox(0), mUnknowncc(0),
      mUnknownd0(static_cast<float>(Application::shared()->GetTempo()) / kTempoToRate),
      mShowCrates(0), mPlayMap(nullptr), mTrackCount(kTrackCount), mUnknown140(0), mUnknown148(0) {
    g_pAppTunnel = this;
    g_nAppTunnelDisplayMode = QueryConfigFlag(kDisplayModeConfigCode);
    CacheTunnelObjectByName();
    Rnd::Tunnel *pTunnel = GetCachedTunnelObject();
    mPlayMap = Application::shared()->GetPlayMap();
    if (Application::shared()->GetPlayMode() == kPlayModeGame) {
        mUnknowncc = 1;
    }
    for (int i = 0; i < kTrackCount; ++i) {
        mTrackModes[i] =
            static_cast<TrackMode>(Application::shared()->GetLevel()->TrackAt(i)->mKind);
    }
    std::vector<Player *> &players = Application::shared()->GetWorld()->mPlayers;
    const int nPlayers = players.size();
    const int nLocalPlayers = Application::shared()->GetWorld()->mLocalPlayers.size();
    mUnknown144 = static_cast<float>(Application::shared()->GetTempo()) / kTempoToRate;

    FindObject<Rnd::Animatable>("realtime.view")->SetRate(mUnknown144);
    GetCachedTunnelObject()->SetLaneChangeFrames(kLaneChangeFrames / mUnknown144);
    // Yes, the binary discards the count of removed events.
    (void)pTunnel->RemoveEventsInRange(kEarliestEventFrame, kLatestEventFrame);

    Rnd::View *pTestDraw = FindObject<Rnd::View>("tnl test draw");
    if (pTestDraw != nullptr) {
        pTestDraw->SetShowing(0);
    }
    Rnd::View *pTunnelView = FindObject<Rnd::View>("tnl.view");
    pTunnelView->RemoveAnim(FindObject<Rnd::Animatable>("test anim"));
    Rnd::Cam *pTestCam = FindObject<Rnd::Cam>("test cam");
    if (pTestCam != nullptr) {
        pTunnelView->RemoveTrans(pTestCam);
        pTunnelView->RemoveDraw(pTestCam);
    }

    Rnd::Cam *pMainCam = FindObject<Rnd::Cam>(FormatString("tnl cam%d", nLocalPlayers));
    Rnd::Cam *pFirstOuterCam = FindObject<Rnd::Cam>("outer cam1");
    Rnd::View *pOuterView = FindObject<Rnd::View>("outer.view");
    int nView = 0;
    for (int i = 0; i < nLocalPlayers; ++i) {
        nView = i + 1;
        Rnd::Cam *pCam = FindObject<Rnd::Cam>(FormatString("tnl cam%d", nView));
        Rnd::Cam *pZoomCam = FindObject<Rnd::Cam>(FormatString("tnl cam%dz", nView));
        Rnd::View *pLocalView = FindObject<Rnd::View>(FormatString("tnl local%d.view", nView));
        Rnd::Cam *pSavedCam = Rnd::g_pfnNewCam(HxStr(FormatString("saved tnl cam%d", nView)));
        pSavedCam->Copy(pCam, Rnd::kCopyChildLists);
        mSavedCams.push_back(pSavedCam);
        if (pCam != pMainCam) {
            pCam->Copy(pMainCam, 0);
        }

        Rnd::Cam::Rect rect = pMainCam->mScreenRect;
        rect.x = static_cast<float>(i % kLocalViewsPerRow) * kLocalViewSpan;
        rect.y = static_cast<float>(i / kLocalViewsPerRow) * kLocalViewSpan;
        pCam->mScreenRect = rect;
        pCam->UpdateProjection();
        pZoomCam->mScreenRect = pCam->mScreenRect;
        pZoomCam->UpdateProjection();
        pZoomCam->SetFrustum(pCam->GetNearPlane(), pCam->GetFarPlane(), pCam->GetFov());
        pLocalView->SetShowing(1);

        Rnd::Cam *pOuterCam = FindObject<Rnd::Cam>(FormatString("outer cam%d", nView));
        pOuterCam->Copy(pFirstOuterCam, Rnd::kCopyChildLists);
        pOuterCam->SetShowing(1);
        pOuterCam->mScreenRect = rect;
        pOuterCam->UpdateProjection();
        pOuterCam->SetFrustum(
            pOuterCam->GetNearPlane(), pOuterCam->GetFarPlane(), pMainCam->GetFov());
        pOuterView->RemoveDraw(pOuterCam);
        std::list<Rnd::Drawable *> &draws = pOuterView->GetDraws();
        pOuterView->AddDraw(pOuterCam, draws.empty() ? nullptr : draws.front());
    }
    while (nView < kMaxLocalPlayers) {
        ++nView;
        FindObject<Rnd::View>(FormatString("tnl local%d.view", nView))->SetShowing(0);
        FindObject<Rnd::Cam>(FormatString("outer cam%d", nView))->SetShowing(0);
    }

    int nTrailPoints = kSinglePlayerTrailPoints;
    int nOuterShowing = 1;
    int nCulledFarSlices = kSingleScreenCulledFarSlices;
    int nSplitScreen = 0;
    float flGemCost = kSinglePlayerGemCost;
    float flHexGemLod = 0.0f;
    float flPowerupGemLod = 0.0f;
    std::vector<float> screenSizes(pTunnel->mLodScreenSizes);
    screenSizes[0] = kSingleScreenFirstLod;
    if (nLocalPlayers == kTwoLocalPlayers) {
        nOuterShowing = 1;
        nSplitScreen = 1;
        flGemCost = kTwoPlayerGemCost;
        nTrailPoints = kTwoPlayerTrailPoints;
        screenSizes[0] = kSplitScreenFirstLod;
        flHexGemLod = kTwoPlayerHexGemLod;
        flPowerupGemLod = 0.0f;
        nCulledFarSlices = kTwoPlayerCulledFarSlices;
    } else if (nLocalPlayers == kThreeLocalPlayers) {
        flHexGemLod = kSplitScreenGemLod;
        nOuterShowing = 0;
        nSplitScreen = 1;
        screenSizes[0] = kSplitScreenFirstLod;
        nTrailPoints = kThreePlayerTrailPoints;
        flGemCost = kSplitScreenGemCost;
        flPowerupGemLod = flHexGemLod;
        nCulledFarSlices = kSplitScreenCulledFarSlices;
    } else if (nLocalPlayers == kFourLocalPlayers) {
        nOuterShowing = 0;
        screenSizes[0] = kSplitScreenFirstLod;
        nSplitScreen = 1;
        flHexGemLod = kSplitScreenGemLod;
        nTrailPoints = kFourPlayerTrailPoints;
        nCulledFarSlices = kSplitScreenCulledFarSlices;
        flGemCost = kSplitScreenGemCost;
        flPowerupGemLod = flHexGemLod;
    }

    mLatLight = FindObject<Rnd::Light>("lat light1");
    FindObject<Rnd::View>("outer.view")->SetShowing(nOuterShowing);
    pTunnel->ApplyMeshLodScreenSizes(screenSizes);
    pTunnel->mCulledFarSlices = nCulledFarSlices;
    pTunnel->SetPath(pTunnel->mPath);
    FindObject<Rnd::Environ>("tunnel.env")->ClearLights();
    if (nSplitScreen) {
        g_flTunnelBrightness = kSplitScreenBrightness;
    } else {
        g_flTunnelBrightness = kSingleScreenBrightness;
        FindObject<Rnd::Environ>("tunnel.env")->AddLight(mLatLight);
    }
    Color laneFloor;
    ScaleColor(Color{0.0f, 0.0f, kLaneFloorBlue, 1.0f}, g_flTunnelBrightness, laneFloor);
    pTunnel->SetLaneFloorColor(laneFloor);

    mGemManager = new TnlGemManager(this, flGemCost);
    mHexGemKinds.resize(kHexGemCount);
    mHexGemKinds[0] = mGemManager->AddMeshKind("gem_hex_b", flHexGemLod, 1.0f);
    mHexGemKinds[1] = mGemManager->AddMeshKind("gem_hex_g", flHexGemLod, 1.0f);
    mHexGemKinds[2] = mGemManager->AddMeshKind("gem_hex_r", flHexGemLod, 1.0f);
    mHexGemKinds[3] = mGemManager->AddMeshKind("gem_hex_y", flHexGemLod, 1.0f);
    mHexGemKinds[4] = mGemManager->AddMeshKind("gem_hex_p", flHexGemLod, 1.0f);
    mCrateGemKind = mGemManager->AddMeshKind("gem_crate", flHexGemLod, 1.0f);
    mPowerupGemKinds.resize(kHudItemMultiplier + 1);
    mPowerupGemKinds[kHudItemBumper] = mGemManager->AddMeshKind("gem_bump", flPowerupGemLod, 1.0f);
    mPowerupGemKinds[kHudItemFreestyler] =
        mGemManager->AddMeshKind("gem_free", flPowerupGemLod, 1.0f);
    mPowerupGemKinds[kHudItemCrippler] =
        mGemManager->AddMeshKind("gem_crip", flPowerupGemLod, 1.0f);
    mPowerupGemKinds[kHudItemAutocatcher] =
        mGemManager->AddMeshKind("gem_auto", flPowerupGemLod, 1.0f);
    mPowerupGemKinds[kHudItemNeutralizer] =
        mGemManager->AddMeshKind("gem_neut", flPowerupGemLod, 1.0f);
    mPowerupGemKinds[kHudItemMultiplier] =
        mGemManager->AddMeshKind("gem_mult", flPowerupGemLod, 1.0f);
    mGhostGemKinds.resize(mTrackCount);
    for (int i = 0; i < mTrackCount; ++i) {
        mGhostGemKinds[i] = mGemManager->AddMeshKind(
            FormatString("gem_ghost%d", i), flHexGemLod, 1.0f / static_cast<float>(nLocalPlayers));
        mGemManager->SetKindShowing(mGhostGemKinds[i], 0);
    }
    mMissGemKind = mGemManager->AddMeshKind("gem_miss", 0.0f, 1.0f);

    char instrumentKinds[kInstrumentCount];
    instrumentKinds[kInstrumentDrums] = mGemManager->AddEffectKind("gem_drum");
    instrumentKinds[kInstrumentBass] = mGemManager->AddEffectKind("gem_bass");
    instrumentKinds[kInstrumentSynth] = mGemManager->AddEffectKind("gem_synth");
    instrumentKinds[kInstrumentGuitar] = mGemManager->AddEffectKind("gem_guitar");
    instrumentKinds[kInstrumentVocal] = mGemManager->AddEffectKind("gem_vox");
    instrumentKinds[kInstrumentFX] = mGemManager->AddEffectKind("gem_fx");
    const char scratchKind = mGemManager->AddEffectKind("gem_scratch");
    mTrackEffectKinds.resize(mTrackCount);
    for (int i = 0; i < mTrackCount; ++i) {
        TrackData *pTrack = Application::shared()->GetLevel()->TrackAt(i);
        if (mTrackModes[i] == kTrackModeScratch) {
            mTrackEffectKinds[i] = scratchKind;
        } else {
            mTrackEffectKinds[i] = instrumentKinds[pTrack->mInstrument];
        }
    }

    mGemTrails = new DurGemTrails(this, nTrailPoints);
    for (int i = 0; i < mTrackCount; ++i) {
        const TrackMode mode = mTrackModes[i];
        if (mode == kTrackModeVocal) {
            mGemTrails->CreateLane(kVoxTrailWidth, i, FindObject<Rnd::Mat>("voxstring.mat"));
        } else if ((mode == kTrackModeAxe) || (mode == kTrackModeScratch)) {
            mGemTrails->CreateLane(kStringTrailWidth, i, FindObject<Rnd::Mat>("string gem mat"));
        }
    }
    mStringView = FindObject<Rnd::View>("tnl strings");
    mStringGemMat = FindObject<Rnd::Mat>("string gem mat");
    mVoxStringMat = FindObject<Rnd::Mat>("voxstring.mat");
    mStringFlare = FindObject<Rnd::ParticleSys>("string flare.ps");
    Rnd::Particle *pParticle;
    while ((pParticle = mStringFlare->AllocParticle())) {
        pParticle->mSize = kStringFlareSize;
        pParticle->mCol = Color{1.0f, 1.0f, 1.0f, 0.0f};
    }

    mGhostMats.resize(mTrackCount);
    for (int i = 0; i < mTrackCount; ++i) {
        mGhostMats[i] = FindObject<Rnd::Mat>(FormatString("gem_ghost%d.mat", i));
    }
    mGhostFadeRates.resize(mTrackCount);
    for (unsigned i = 0; i < mGhostFadeRates.size(); ++i) {
        mGhostFadeRates[i] = 0.0f;
    }

    pTunnel->ResizeSeekers(nPlayers);
    for (int i = 0; i < nPlayers; ++i) {
        mPlayers.push_back(new TnlPlayer(players[i], i, this));
    }

    Rnd::ParticleSys *pFlashSystem = FindObject<Rnd::ParticleSys>("gem_flash.ps");
    mGemFlashes.resize(kGemFlashCount);
    for (unsigned i = 0; i < mGemFlashes.size(); ++i) {
        mGemFlashes[i] = new GemFlash{pFlashSystem, nullptr};
    }
    mPanelFX.resize(kPanelFXCount);
    for (unsigned i = 0; i < mPanelFX.size(); ++i) {
        mPanelFX[i] = new TnlPanelFX(i);
    }
    mBoundary = new TnlBoundary(mPlayMap);
    mNowRing = new TnlNowRing(mTrackCount, nPlayers);
    mArms = new TnlArms();
    mMultFX = new TnlMultFX();
    mLattice = new TnlLattice();
    for (int i = 0; i < kFireCount; ++i) {
        mFireFX.push_back(new TnlFireFX(HxStr(FormatString("fire%d", i)), kFireIndex));
    }
    for (int i = 0; i < kFireFsCount; ++i) {
        mFireFX.push_back(new TnlFireFX(HxStr(FormatString("firefs%d", i)), kFireFsIndex));
    }
    for (int i = 0; i < kFireMultCount; ++i) {
        mFireFX.push_back(new TnlFireFX(HxStr(FormatString("firemult%d", i)), i));
    }
    for (int i = 0; i < kCrippleFXCount; ++i) {
        mCrippleFX.push_back(new TnlCrippleFX(i, mUnknown144));
    }
    for (int i = 0; i < kBumpFXCount; ++i) {
        mBumpFX.push_back(new TnlBumpFX(i));
    }
    for (int i = 0; i < kSnakeCount; ++i) {
        mSnakes.push_back(new TnlSnake());
    }
    for (int i = 0; i < kArrowCount; ++i) {
        mArrows.push_back(new TnlArrow(i));
    }

    RunScript(HxStr("hx.nowring(1)"));
    RunScript(HxStr("hx.sections(1)"));
    RunScript(HxStr("hx.fade_activator(1)"));

    if ((mPlayMode == kPlayModeJam) && Application::shared()->IsJukeboxMode()) {
        mJukebox = 1;
    }
    mCameraRig = new TnlCameraRig(nPlayers, mJukebox);
    if (mJukebox) {
        for (auto it = mPlayers.begin(); it != mPlayers.end(); ++it) {
            (*it)->mActivator.SetSuppressed(1);
        }
        mNowRing->mView->SetShowing(0);
    }
}

AppTunnel::~AppTunnel() {
    g_pAppTunnel = nullptr;
    delete mGemManager;
    delete mGemTrails;
    for (auto it = mGemFlashes.begin(); it != mGemFlashes.end(); ++it) {
        delete *it;
    }
    for (auto it = mPanelFX.begin(); it != mPanelFX.end(); ++it) {
        delete *it;
    }
    for (auto it = mFireFX.begin(); it != mFireFX.end(); ++it) {
        delete *it;
    }
    for (auto it = mCrippleFX.begin(); it != mCrippleFX.end(); ++it) {
        delete *it;
    }
    for (auto it = mBumpFX.begin(); it != mBumpFX.end(); ++it) {
        delete *it;
    }
    for (auto it = mSnakes.begin(); it != mSnakes.end(); ++it) {
        delete *it;
    }
    for (auto it = mPlayers.begin(); it != mPlayers.end(); ++it) {
        delete *it;
    }
    for (auto it = mPanels.begin(); it != mPanels.end(); ++it) {
        delete *it;
    }
    for (auto it = mArrows.begin(); it != mArrows.end(); ++it) {
        delete *it;
    }
    delete mBoundary;
    delete mNowRing;
    delete mCameraRig;
    delete mArms;
    delete mMultFX;
    delete mLattice;
    // The pending triggers are not deleted.
    for (unsigned i = 0; i < mSavedCams.size(); ++i) {
        Rnd::Cam *pCam = FindObject<Rnd::Cam>(FormatString("tnl cam%d", i + 1));
        pCam->Copy(mSavedCams[i], Rnd::kCopyChildLists);
        delete mSavedCams[i];
    }
    mSavedCams.clear();
}

inline TnlPlayer *AppTunnel::FindTnlPlayer(Player *pPlayer) {
    for (auto it = mPlayers.begin(); it != mPlayers.end(); ++it) {
        if ((*it)->mPlayer == pPlayer) {
            return *it;
        }
    }
    return nullptr;
}

void AppTunnel::OnBarChanged(
    int nTrack, int nBar, int nUnknown, Player *pPlayer, int nPowerup, int nEnabled) {
    int nNewPanel = 0;
    if (mUnknowncc) {
        nNewPanel = (nUnknown == 0);
    }
    const float flTick = mRenderer->mSongTick;
    if (flTick < 0.0f) {
        nNewPanel = 0;
    }
    if (kBarChangeLateFrames < flTick - static_cast<float>((nBar + 1) * kFramesPerBar)) {
        return;
    }
    const float flStartFrame =
        flTick + ((static_cast<float>(nBar * kFramesPerBar) - flTick) * kPanelLeadFraction);
    if (mJukebox) {
        nEnabled = 0;
    }
    TnlPanel::Kind kind;
    const TrackMode mode = mTrackModes[nTrack];
    if (mode == kTrackModeAxe) {
        kind = TnlPanel::kKindAxe;
    } else if (mode == kTrackModeScratch) {
        kind = TnlPanel::kKindScratch;
    } else {
        kind = (mode == kTrackModeVocal) ? TnlPanel::kKindVox : TnlPanel::kKindLane;
    }
    const int nStep = mPlayMap->FindStepIndex(mPlayMap->Slot5(nBar));
    if (nNewPanel) {
        AddPanel(new TnlPanel(nTrack, nBar, pPlayer, nPowerup, kind, nEnabled, nStep),
                 flStartFrame);
    } else {
        TnlPanel panel(nTrack, nBar, pPlayer, nPowerup, kind, nEnabled, nStep);
        panel.Apply();
    }
}

void AppTunnel::OnLeaderChanged(Player *pOldLeader, Player *pNewLeader) {
    if (mGameMode == kGameModeSolo) {
        return;
    }
    if (pOldLeader != nullptr) {
        FindTnlPlayer(pOldLeader)->mActivator.SetLeader(0);
    }
    if (pNewLeader != nullptr) {
        FindTnlPlayer(pNewLeader)->mActivator.SetLeader(1);
    }
}

void AppTunnel::AddPanel(TnlPanel *pPanel, float flStartFrame) {
    mPanels.push_back(pPanel);
    pPanel->SetStartFrame(flStartFrame);
}

void AppTunnel::UpdateGhostFades() {
    for (unsigned i = 0; i < mGhostFadeRates.size(); ++i) {
        Rnd::Mat *pMat = GetGhostMat(i);
        if (mGhostFadeRates[i] == 0.0f) {
            continue;
        }
        float flAlpha = pMat->mDiffuse.a + mGhostFadeRates[i];
        if (flAlpha < 0.0f) {
            flAlpha = 0.0f;
            mGhostFadeRates[i] = 0.0f;
            mGemManager->SetKindShowing(mGhostGemKinds[i], 0);
        } else if (1.0f < flAlpha) {
            mGhostFadeRates[i] = 0.0f;
            flAlpha = 1.0f;
        }
        pMat->SetAlpha(flAlpha);
    }
}

void AppTunnel::SetFrame(float flFrame) {
    const float flScaledFrame = flFrame * mUnknown144;
    for (Rnd::Particle *pParticle = mStringFlare->GetLiveParticles(); pParticle != nullptr;
         pParticle = pParticle->mNext) {
        pParticle->mCol.a -= kStringFlareFade;
        if (pParticle->mCol.a < 0.0f) {
            pParticle->mCol.a = 0.0f;
        }
    }
    mGemManager->Update(flFrame);
    mGemTrails->Update(flFrame);
    for (auto it = mPanels.begin(); it != mPanels.end();) {
        if ((*it)->Update(flFrame, this)) {
            ++it;
        } else {
            delete *it;
            it = mPanels.erase(it);
        }
    }
    for (auto it = mPendingTriggers.begin(); it != mPendingTriggers.end();) {
        if (it->Update(flFrame)) {
            ++it;
        } else {
            it = mPendingTriggers.erase(it);
        }
    }
    for (auto it = mGemFlashes.begin(); it != mGemFlashes.end(); ++it) {
        GemFlash *pFlash = *it;
        if (pFlash->mParticle == nullptr) {
            continue;
        }
        if (pFlash->mParticle->mSize <= 0.0f) {
            pFlash->mSystem->FreeParticle(pFlash->mParticle);
            pFlash->mParticle = nullptr;
        } else {
            pFlash->mParticle->mSize -= kGemFlashShrink;
        }
    }
    for (auto it = mPanelFX.begin(); it != mPanelFX.end(); ++it) {
        (*it)->Update(flFrame);
    }
    for (auto it = mFireFX.begin(); it != mFireFX.end(); ++it) {
        (*it)->SetFrame(flFrame, flScaledFrame);
    }
    for (auto it = mCrippleFX.begin(); it != mCrippleFX.end(); ++it) {
        (*it)->SetFrame(flFrame);
    }
    for (auto it = mBumpFX.begin(); it != mBumpFX.end(); ++it) {
        (*it)->SetFrame(flFrame);
    }
    for (auto it = mSnakes.begin(); it != mSnakes.end(); ++it) {
        (*it)->Update(flFrame);
    }
    for (auto it = mArrows.begin(); it != mArrows.end(); ++it) {
        (*it)->SetFrame(flScaledFrame);
    }
    mBoundary->SetFrame(flFrame);
    mNowRing->SetFrame(flFrame);
    mArms->SetFrame(flFrame);
    mMultFX->SetFrame(flFrame);
    mLattice->SetFrame(flFrame);
    mCameraRig->SetFrame(flFrame);
    for (auto it = mPlayers.begin(); it != mPlayers.end(); ++it) {
        (*it)->Update(flFrame, flScaledFrame);
    }
    if (static_cast<float>(mUnknown140 * kFramesPerBar) < flFrame) {
        mUnknown140 = mPlayMap->FollowingStepBar(mUnknown140);
    }
    UpdateGhostFades();
}

int AppTunnel::IsTrackBarLocked(int nTrack, int nBar) {
    // The binary compares the track mode against mPlayMode itself, which is kPlayModeJam here.
    if ((mPlayMode == kPlayModeJam) && (mTrackModes[nTrack] == kTrackModeRiff)) {
        return 0;
    }
    const TrackMode mode = mTrackModes[nTrack];
    if ((mode == kTrackModeAxe) || (mode == kTrackModeScratch) || (mode == kTrackModeVocal)) {
        return 1;
    }
    if (!mRenderer->GetCell(nTrack, nBar)->mPlayer->IsNull()) {
        return 1;
    }
    if (mRenderer->GetCell(nTrack, nBar)->mEnabled) {
        return 0;
    }
    return 1;
}

void AppTunnel::ShowTrackGhost(int nTrack, Rnd::Drawable *pGhost) {
    pGhost->ClearDraws();
    mGemManager->AddKindDraws(mGhostGemKinds[nTrack], pGhost);
    mGemManager->SetKindShowing(mGhostGemKinds[nTrack], 1);
    GetGhostMat(nTrack)->SetAlpha(0.0f);
    mGhostFadeRates[nTrack] = kGhostFadeRate;
}

void AppTunnel::HideTrackGhost(int nTrack) {
    GetGhostMat(nTrack)->SetAlpha(1.0f);
    mGhostFadeRates[nTrack] = -kGhostFadeRate;
}

Rnd::Mat *AppTunnel::GetGhostMat(int nTrack) {
    return mGhostMats[nTrack];
}

void AppTunnel::StartGemFlash(const Vector3 &pos) {
    for (auto it = mGemFlashes.begin(); it != mGemFlashes.end(); ++it) {
        GemFlash *pFlash = *it;
        if (pFlash->mParticle == nullptr) {
            // The binary does not test the new particle for null.
            pFlash->mParticle = pFlash->mSystem->AllocParticle();
            pFlash->mParticle->mCol = Color{1.0f, 1.0f, 1.0f, 1.0f};
            pFlash->mParticle->mSize = 1.0f;
            pFlash->mParticle->mPos = pos;
            return;
        }
    }
}

int AppTunnel::StartPanelFX(int nRing, int nSlice, int nForward) {
    for (auto it = mPanelFX.begin(); it != mPanelFX.end(); ++it) {
        if ((*it)->IsIdle()) {
            (*it)->Start(nRing, nSlice, nForward);
            return 1;
        }
    }
    return 0;
}

void AppTunnel::StartFireFX(float flPathStart,
                            int nIndex,
                            int nSlot,
                            const Color &color,
                            const Color &altColor,
                            float flPathEnd) {
    for (auto it = mFireFX.begin(); it != mFireFX.end(); ++it) {
        if ((*it)->Start(flPathStart, nIndex, nSlot, color, altColor, flPathEnd)) {
            return;
        }
    }
}

int AppTunnel::StartCrippleFX(const std::vector<TnlPlayer *> &targets, float flFrame) {
    for (auto it = mCrippleFX.begin(); it != mCrippleFX.end(); ++it) {
        if ((*it)->mState == TnlCrippleFX::kStateIdle) {
            (*it)->Start(targets, flFrame);
            return 1;
        }
    }
    return 0;
}

int AppTunnel::StartBumpFX(int nStep, const HxStr &colorName, int nForward, float flPathOffset) {
    for (auto it = mBumpFX.begin(); it != mBumpFX.end(); ++it) {
        if ((*it)->IsIdle()) {
            (*it)->Start(nStep, colorName, nForward, flPathOffset);
            return 1;
        }
    }
    return 0;
}

int AppTunnel::StartSnake(
    float flFrame, int nRing, const Color &color, float flPhase, float flAmplitude) {
    for (auto it = mSnakes.begin(); it != mSnakes.end(); ++it) {
        if ((*it)->mStartFrame == kIdleSnakeFrame) {
            (*it)->Start(flFrame, nRing, color, flPhase, flAmplitude);
            return 1;
        }
    }
    return 0;
}

void AppTunnel::AddPendingTrigger(TnlTrigger *pTrigger, float flFrame) {
    mPendingTriggers.push_back(TnlPendingTrigger{pTrigger, flFrame});
}

void AppTunnel::PlaceStringFlare(const Vector3 &pos) {
    for (Rnd::Particle *pParticle = mStringFlare->GetLiveParticles(); pParticle != nullptr;
         pParticle = pParticle->mNext) {
        if (pParticle->mCol.a != 1.0f) {
            pParticle->mPos = pos;
            pParticle->mCol.a = 1.0f;
            return;
        }
    }
}

void AppTunnel::PlaceStringFlareOnRing(int nRing, float flBlend) {
    for (Rnd::Particle *pParticle = mStringFlare->GetLiveParticles(); pParticle != nullptr;
         pParticle = pParticle->mNext) {
        if (pParticle->mCol.a != 1.0f) {
            Transform xfm;
            PadTransformRows(xfm);
            GetCachedTunnelObject()->ProjectSectionToCameraSpace(
                nRing, &xfm, mRenderer->mSongTick, flBlend, kStringFlareRingScale);
            pParticle->mPos = xfm.mTranslation;
            pParticle->mCol.a = 1.0f;
            return;
        }
    }
}

void AppTunnel::PrepareLocalView(int nView, [[maybe_unused]] float flFrame) {
    mNowRing->SetRotation(nView);
}
