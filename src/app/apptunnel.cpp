#include "app/apptunnel.h"

#include <algorithm>

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
#include "app/tnlpanelfxdelay.h"
#include "app/tnlplayer.h"
#include "app/tnlsnake.h"
#include "app/tnlutil.h"
#include "app/tunnelcache.h"
#include "game/forcefeedbackmgr.h"
#include "game/gamemanagerimpl.h"
#include "game/grooveworld.h"
#include "game/leveldata.h"
#include "game/nullplayer.h"
#include "game/player.h"
#include "game/playmap.h"
#include "math/color.h"
#include "math/transform.h"
#include "math/vector3.h"
#include "mid/mbt.h"
#include "msg/advancesectiontogglemsg.h"
#include "msg/axebuttonmsg.h"
#include "msg/catchmsg.h"
#include "msg/choosepowerupmsg.h"
#include "msg/cleargemmsg.h"
#include "msg/cleargemsmsg.h"
#include "msg/cripplepacket.h"
#include "msg/deployedpowerupmsg.h"
#include "msg/displaypointermsg.h"
#include "msg/durgemmsg.h"
#include "msg/freestylefxmsg.h"
#include "msg/gemmsg.h"
#include "msg/juiceamountmsg.h"
#include "msg/multiplierstatemsg.h"
#include "msg/nowbarmsg.h"
#include "msg/phrasemuffedmsg.h"
#include "msg/pitchmsg.h"
#include "msg/playbacktogglemsg.h"
#include "msg/playerstrackneutralizedmsg.h"
#include "msg/powerupfailedmsg.h"
#include "msg/sectioncapturedmsg.h"
#include "msg/seekermsg.h"
#include "msg/showeraseeffectmsg.h"
#include "msg/stdmidimsg.h"
#include "msg/susgemmsg.h"
#include "msg/toggleghostmsg.h"
#include "msg/trackselectmsg.h"
#include "msg/winmsg.h"
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

// Script templates the track selection and the axe button run in display mode.
constexpr int kTrackSelectScriptTemplate = 0x3ef;
constexpr int kAxeButtonScriptTemplate = 0x3f7;

// Scaled frames a freestyler arrow stays up after a failed freestyler.
constexpr float kArrowShowFrames = 2000.0f;

// Juice fraction below which a solo player's activator blinks.
constexpr float kLowJuiceFraction = 0.2f;

// A gem's lane blend is its gem index times the scale plus the base.
constexpr float kGemLaneScale = 0.315f;
constexpr float kGemLaneBase = 0.185f;

// A real player's gem further ahead than this in a game draws as the track's effect gem.
constexpr float kFarGemFrames = 100.0f;

// Frame from which a miss gem is drawn, before the song starts.
constexpr float kMissGemAppearFrame = -1000.0f;

// Script templates the catch, the miss, the pitch, and the erase run in display mode.
constexpr int kCatchScriptTemplate = 0x3ea;
constexpr int kMissScriptTemplate = 0x3eb;
constexpr int kPitchScriptTemplate = 0x3fc;
constexpr int kEraseScriptTemplate = 0x3f4;

// An erase panel starts a sixth of the way to its bar, unless the bar is this far behind.
constexpr float kErasePanelDivisor = 6.0f;
constexpr float kErasePanelCutoff = -1900.0f;

// A section capture's fire starts a quarter bar earlier in a local game.
constexpr float kLocalCaptureLead = 480.0f;

// Frames ahead of the song tick a crippler is launched.
constexpr float kCrippleLaunchLead = 120.0f;

// A freestyler's snakes run this long, and its fire starts this much earlier.
constexpr float kFreestyleFrames = 11520.0f;
constexpr float kFreestyleFireLead = 1200.0f;
constexpr float kFreestyleShade = 0.3f;
constexpr float kFreestyleSecondPhase = 3.1415925f;

// A bumper's path offset for the attacker, and the base and per-level step for the target.
constexpr float kBumperAttackerOffset = 50.0f;
constexpr int kBumperTargetOffset = -50;
constexpr int kBumperTargetLevelStep = 75;

// A multiplier's effect runs from this long before the song tick to this long after it.
constexpr float kMultiplierLead = 500.0f;
constexpr float kMultiplierRun = 15360.0f;

} // namespace

AppTunnel *g_pAppTunnel;

// 0x00456cd0
inline AppTunnel::GemFlash::~GemFlash() {
    if (mParticle != nullptr) {
        mSystem->FreeParticle(mParticle);
    }
}

// 0x00456d28
inline int AppTunnel::GemFlash::Start(const Vector3 &pos) {
    if (mParticle != nullptr) {
        return 0;
    }
    // The binary does not test the new particle for null.
    mParticle = mSystem->AllocParticle();
    mParticle->mCol = Color{1.0f, 1.0f, 1.0f, 1.0f};
    mParticle->mSize = 1.0f;
    mParticle->mPos = pos;
    return 1;
}

// 0x00456db0
inline void AppTunnel::GemFlash::Update() {
    if (mParticle == nullptr) {
        return;
    }
    if (mParticle->mSize <= 0.0f) {
        mSystem->FreeParticle(mParticle);
        mParticle = nullptr;
    } else {
        mParticle->mSize -= kGemFlashShrink;
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
        mNowRing->SetShowing(0);
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

// 0x00447268
void AppTunnel::AddPanel(TnlPanel *pPanel, float flStartFrame) {
    mPanels.push_back(pPanel);
    pPanel->SetStartFrame(flStartFrame);
}

// 0x00447638
void AppTunnel::OnGem(GemMsg *pMsg) {
    const float flFrame = static_cast<float>(pMsg->mPosition.mTick);
    const float flBlend = static_cast<float>(pMsg->mGem) * kGemLaneScale + kGemLaneBase;
    const int nTrack = pMsg->mTrack;
    Player *pPlayer = pMsg->mPlayer;
    const float flTick = mRenderer->mSongTick;
    const int nPowerup =
        mRenderer->GetCell(nTrack, static_cast<int>(flFrame / static_cast<float>(kFramesPerBar)))
            ->mPowerup;
    if (kBarChangeLateFrames < flTick - flFrame) {
        return;
    }
    float flAppearFrame = flTick;
    bool bFlash = !pPlayer->IsNull();
    const int nColor = TnlColorIndexFromName(pPlayer->mColorName);
    char nKind;
    if (pMsg->mGhost) {
        nKind = mGhostGemKinds[nTrack];
    } else if ((mTrackModes[nTrack] == kTrackModeScratch) ||
               ((mPlayMode == kPlayModeGame) && !pPlayer->IsNull() &&
                (kFarGemFrames < flFrame - flTick)) ||
               mJukebox) {
        nKind = mTrackEffectKinds[nTrack];
        bFlash = true;
        if (mUnknowncc) {
            flAppearFrame = flTick + ((flFrame - flTick) * kPanelLeadFraction);
        }
    } else if (nPowerup != -1) {
        nKind = mPowerupGemKinds[nPowerup];
    } else {
        nKind = mShowCrates ? mCrateGemKind : mHexGemKinds[nColor];
    }
    mGemManager->Add(TnlGem(nKind, nTrack, nColor, bFlash, flFrame, flBlend, flAppearFrame));
}

// 0x00447938
void AppTunnel::OnCatch(CatchMsg *pMsg) {
    TnlPlayer *pPlayer = FindTnlPlayer(pMsg->mPlayer);
    const int nGem = pMsg->mGem;
    const float flFrame = static_cast<float>(pMsg->mTick);
    const float flBlend = static_cast<float>(nGem) * kGemLaneScale + kGemLaneBase;
    const int nTrack = pMsg->mTrack;
    pPlayer->mActivator.mCatcher.Hit(nGem);
    if (pMsg->mHit) {
        Transform xfm;
        PadTransformRows(xfm);
        GetCachedTunnelObject()->GetRingXfm(nTrack, &xfm, flFrame, flBlend);
        StartGemFlash(xfm.mTranslation);
        pPlayer->mSabreTrail.Pulse(flFrame, pMsg->mCaught, pMsg->mTotal);
        if (g_nAppTunnelDisplayMode) {
            CallScriptTemplate(kCatchScriptTemplate);
        }
    } else {
        mGemManager->Add(
            TnlGem(mMissGemKind, nTrack, 0, false, flFrame, flBlend, kMissGemAppearFrame));
        if (g_nAppTunnelDisplayMode) {
            CallScriptTemplate(kMissScriptTemplate);
        }
    }
}

// 0x00447ba8
void AppTunnel::OnPhraseMuffed(PhraseMuffedMsg *pMsg) {
    if (!pMsg->mTried) {
        return;
    }
    const int nTrack = pMsg->mTrack;
    const int nBar = pMsg->mPosition.mTick / Mid::MBT(kFramesPerBar).mTick;
    if (mRenderer->GetCell(nTrack, nBar)->mPowerup == -1) {
        return;
    }
    const int nStep = mPlayMap->FindStepIndex(mPlayMap->Slot5(nBar));
    TnlPanel panel(nTrack, nBar, &g_nullPlayer, -1, TnlPanel::kKindLane, 1, nStep);
    panel.Apply();
}

// 0x00447cc0
void AppTunnel::OnPitch(PitchMsg *pMsg) {
    TnlPlayer *pPlayer = FindTnlPlayer(pMsg->mUnknown10);
    const int nGem = pMsg->mUnknown0c;
    const float flBlend = static_cast<float>(nGem) * kGemLaneScale + kGemLaneBase;
    Transform xfm;
    PadTransformRows(xfm);
    GetCachedTunnelObject()->GetRingXfm(pMsg->mUnknown08, &xfm, mRenderer->mSongTick, flBlend);
    StartGemFlash(xfm.mTranslation);
    pPlayer->mActivator.mCatcher.Hit(nGem);
    if (g_nAppTunnelDisplayMode) {
        CallScriptTemplate(kPitchScriptTemplate);
    }
}

// 0x00447eb0
void AppTunnel::OnSeeker(SeekerMsg *pMsg) {
    TnlPlayer *pPlayer = FindTnlPlayer(pMsg->mPlayer);
    // Yes, the binary converts both bar words to float and back.
    const float flFirstBar = static_cast<float>(pMsg->mFirstBar);
    const float flBarCount = static_cast<float>(pMsg->mBarCount);
    const int nTrack = pMsg->mTrack;
    if (!pMsg->mEnabled) {
        pPlayer->mSeekerFade.ClearRange();
        pPlayer->mSabreTrail.Clear();
        return;
    }
    pPlayer->mSeekerFade.SetRange(
        static_cast<int>(flFirstBar), static_cast<int>(flBarCount), nTrack);
    if ((mPlayMode == kPlayModeGame) && (mTrackModes[nTrack] == kTrackModeCatch)) {
        pPlayer->mSabreTrail.Build(
            nTrack, static_cast<int>(flFirstBar), static_cast<int>(flBarCount));
    }
}

// 0x004481d0
void AppTunnel::OnShowEraseEffect(ShowEraseEffectMsg *pMsg) {
    const float flTick = mRenderer->mSongTick;
    for (int nBar = pMsg->mFirstBar; nBar < pMsg->mEndBar; ++nBar) {
        const float flAhead = static_cast<float>(nBar * kFramesPerBar) - flTick;
        if (kErasePanelCutoff < flAhead) {
            AddPendingTrigger(new TnlPanelFXDelay(this, pMsg->mTrack, nBar, 1),
                              flTick + flAhead / kErasePanelDivisor);
        }
    }
    if (g_nAppTunnelDisplayMode) {
        CallScriptTemplate(kEraseScriptTemplate);
    }
}

// 0x00448530
void AppTunnel::OnSectionCaptured(SectionCapturedMsg *pMsg) {
    if ((mGameMode != kGameModeSolo) && pMsg->mAutoCatch) {
        return;
    }
    const float flPathEnd = static_cast<float>(pMsg->mEndBar * kFramesPerBar);
    float flPathStart = mRenderer->mSongTick;
    int nIndex = -1;
    if (mGameMode == kGameModeLocal) {
        nIndex = pMsg->mPlayer->Slot2();
        flPathStart -= kLocalCaptureLead;
    }
    // The binary copies the name before the lookup.
    const Color playerColor = TnlColorFromName(HxStr(pMsg->mPlayer->mColorName));
    const Color color = pMsg->mAutoCatch ? Color{0.0f, 0.0f, 1.0f, 1.0f} : playerColor;
    StartFireFX(flPathStart, nIndex, pMsg->mTrack, color, playerColor, flPathEnd);
}

// 0x004486f8
void AppTunnel::OnCripple(CripplePacket *pPacket) {
    std::vector<TnlPlayer *> targets;
    for (unsigned i = 0; i < pPacket->mTargets.size(); ++i) {
        targets.push_back(FindTnlPlayer(pPacket->mTargets[i]));
    }
    (void)StartCrippleFX(targets, mRenderer->mSongTick + kCrippleLaunchLead);
}

// 0x00448a08
void AppTunnel::OnFreestyleFX(FreestyleFXMsg *pMsg) {
    const float flTick = mRenderer->mSongTick;
    const float flEnd = flTick + kFreestyleFrames;
    (void)StartSnake(
        flTick, pMsg->mTrack, Color{kFreestyleShade, kFreestyleShade, 1.0f, 1.0f}, 0.0f, 1.0f);
    (void)StartSnake(flTick,
                     pMsg->mTrack,
                     Color{kFreestyleShade, 1.0f, kFreestyleShade, 1.0f},
                     kFreestyleSecondPhase,
                     -1.0f);
    const Color blue{0.0f, 0.0f, 1.0f, 1.0f};
    StartFireFX(flTick - kFreestyleFireLead,
                kFireFsIndex,
                pMsg->mTrack,
                blue,
                blue,
                flEnd - kFreestyleFireLead);
}

// 0x00448d58
void AppTunnel::OnDeployedPowerup(DeployedPowerupMsg *pMsg) {
    switch (pMsg->mKind) {
    case kHudItemNeutralizer: {
        const float flTick = mRenderer->mSongTick;
        for (int nBar = pMsg->mFirstBar; nBar < pMsg->mFirstBar + pMsg->mBarCount; ++nBar) {
            const float flFrame =
                flTick + ((static_cast<float>(nBar * kFramesPerBar) - flTick) * kPanelLeadFraction);
            AddPendingTrigger(new TnlPanelFXDelay(this, pMsg->mTrack, nBar, 1), flFrame);
        }
        break;
    }
    case kHudItemAutocatcher: {
        if (mGameMode == kGameModeSolo) {
            break;
        }
        const float flPathEnd =
            static_cast<float>((pMsg->mFirstBar + pMsg->mBarCount) * kFramesPerBar);
        const float flPathStart = mRenderer->mSongTick;
        const Color blue{0.0f, 0.0f, 1.0f, 1.0f};
        // The binary copies the name before the lookup.
        const Color playerColor = TnlColorFromName(HxStr(pMsg->mPlayer->mColorName));
        StartFireFX(flPathStart, kFireIndex, pMsg->mTrack, blue, playerColor, flPathEnd);
        if (Application::shared()->GetWorld() != nullptr) {
            Application::shared()->GetWorld()->mForceFeedback->PlayEffect0(pMsg->mPlayer);
        }
        break;
    }
    case kHudItemBumper: {
        int nMaxLevel = 0;
        for (auto it = mPlayers.begin(); it != mPlayers.end(); ++it) {
            if ((*it)->mActivator.mTrack == pMsg->mTrack) {
                nMaxLevel = std::max(nMaxLevel, (*it)->mActivator.mLevel);
            }
        }
        (void)StartBumpFX(pMsg->mTrack, HxStr(pMsg->mPlayer->mColorName), 1, kBumperAttackerOffset);
        const float flTargetOffset =
            static_cast<float>(kBumperTargetOffset - nMaxLevel * kBumperTargetLevelStep);
        (void)StartBumpFX(pMsg->mTrack, HxStr(pMsg->mTarget->mColorName), 0, flTargetOffset);
        if (Application::shared()->GetWorld() != nullptr) {
            Application::shared()->GetWorld()->mForceFeedback->PlayEffect1(pMsg->mTarget);
        }
        break;
    }
    case kHudItemMultiplier: {
        const float flTick = mRenderer->mSongTick;
        mMultFX->Start(flTick - kMultiplierLead, flTick + kMultiplierRun);
        break;
    }
    default:
        break;
    }
}

// 0x00457cf0
inline void AppTunnel::OnNowBar(NowBarMsg *pMsg) {
    TnlPlayer *pPlayer = FindTnlPlayer(pMsg->mPlayer);
    if (pPlayer != nullptr) {
        pPlayer->mActivator.mPointer.SetLane(pMsg->mLane);
    }
}

// 0x00457d88
inline void AppTunnel::OnClearGem(ClearGemMsg *pMsg) {
    mGemManager->Remove(pMsg->mTrack,
                        static_cast<float>(pMsg->mPosition.mTick),
                        static_cast<float>(pMsg->mGem) * kGemLaneScale + kGemLaneBase);
}

// 0x00457dd8
inline void AppTunnel::OnClearGems(ClearGemsMsg *pMsg) {
    const float flStart = static_cast<float>(pMsg->mBar) * static_cast<float>(kFramesPerBar);
    mGemManager->RemoveRange(pMsg->mTrack, flStart, flStart + static_cast<float>(kFramesPerBar));
    mGemTrails->EndTrail(pMsg->mTrack, pMsg->mBar);
}

// 0x00457e48
inline void AppTunnel::OnSusGem(SusGemMsg *pMsg) {
    const float flFrame = static_cast<float>(pMsg->mFrame);
    const float flBlend = pMsg->mBlend;
    // The binary copies the name before the lookup.
    const Color color = TnlColorFromName(HxStr(pMsg->mPlayer->mColorName));
    if (!pMsg->mStop) {
        mGemTrails->StartStrip(pMsg->mLane, color, pMsg->mStripId, flFrame, flBlend);
    } else {
        mGemTrails->StopStrip(pMsg->mStripId, flFrame);
    }
}

// 0x00457f38
inline void AppTunnel::OnDurGem(DurGemMsg *pMsg) {
    // The binary copies the name before the lookup.
    const Color color = TnlColorFromName(HxStr(pMsg->mPlayer->mColorName));
    mGemTrails->AddSegment(pMsg->mLane,
                           color,
                           static_cast<float>(pMsg->mStartFrame),
                           pMsg->mStartBlend,
                           static_cast<float>(pMsg->mEndFrame),
                           pMsg->mEndBlend);
}

void AppTunnel::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == static_cast<int>(g_dwTrackSelectMsgType)) {
        OnTrackSelect(static_cast<TrackSelectMsg *>(pMsg));
    } else if (nType == g_nNowBarMsgType) {
        OnNowBar(static_cast<NowBarMsg *>(pMsg));
    } else if (nType == g_nGemMsgType) {
        OnGem(static_cast<GemMsg *>(pMsg));
    } else if (nType == g_nClearGemMsgType) {
        OnClearGem(static_cast<ClearGemMsg *>(pMsg));
    } else if (nType == g_nDurGemMsgType) {
        OnDurGem(static_cast<DurGemMsg *>(pMsg));
    } else if (nType == g_nSusGemMsgType) {
        OnSusGem(static_cast<SusGemMsg *>(pMsg));
    } else if (nType == g_nClearGemsMsgType) {
        OnClearGems(static_cast<ClearGemsMsg *>(pMsg));
    } else if (nType == g_nCatchMsgType) {
        OnCatch(static_cast<CatchMsg *>(pMsg));
    } else if (nType == g_nPitchMsgType) {
        OnPitch(static_cast<PitchMsg *>(pMsg));
    } else if (nType == g_nAxeButtonMsgType) {
        OnAxeButton(static_cast<AxeButtonMsg *>(pMsg));
    } else if (nType == g_nSeekerMsgType) {
        OnSeeker(static_cast<SeekerMsg *>(pMsg));
    } else if ((nType == g_nDisplayPointerMsgType) || (nType == g_nChoosePowerupMsgType)) {
        // Both are ignored.
    } else if (nType == g_nAdvanceSectionToggleMsgType) {
        OnAdvanceSectionToggle();
    } else if (nType == g_nPlayersTrackNeutralizedMsgType) {
        OnPlayersTrackNeutralized(static_cast<PlayersTrackNeutralizedMsg *>(pMsg));
    } else if (nType == g_nShowEraseEffectMsgType) {
        OnShowEraseEffect(static_cast<ShowEraseEffectMsg *>(pMsg));
    } else if (nType == g_nPlaybackToggleMsgType) {
        OnPlaybackToggle(static_cast<PlaybackToggleMsg *>(pMsg));
    } else if (nType == g_nPhraseMuffedMsgType) {
        OnPhraseMuffed(static_cast<PhraseMuffedMsg *>(pMsg));
    } else if (nType == g_nSectionCapturedMsgType) {
        OnSectionCaptured(static_cast<SectionCapturedMsg *>(pMsg));
    } else if (nType == static_cast<int>(g_dwStdMidiMsgType)) {
        // Ignored.
    } else if (nType == g_nToggleGhostMsgType) {
        OnToggleGhost(static_cast<ToggleGhostMsg *>(pMsg));
    } else if (nType == g_nDeployedPowerupMsgType) {
        OnDeployedPowerup(static_cast<DeployedPowerupMsg *>(pMsg));
    } else if (nType == g_nCripplePacketType) {
        OnCripple(static_cast<CripplePacket *>(pMsg));
    } else if (nType == g_nFreestyleFXMsgType) {
        OnFreestyleFX(static_cast<FreestyleFXMsg *>(pMsg));
    } else if (nType == g_nWinMsgType) {
        OnWin(static_cast<WinMsg *>(pMsg));
    } else if (nType == g_nJuiceAmountMsgType) {
        OnJuiceAmount(static_cast<JuiceAmountMsg *>(pMsg));
    } else if (nType == g_nMultiplierStateMsgType) {
        OnMultiplierState(static_cast<MultiplierStateMsg *>(pMsg));
    } else if (nType == g_nPowerupFailedMsgType) {
        OnPowerupFailed(static_cast<PowerupFailedMsg *>(pMsg));
    }
}

// 0x00447328
void AppTunnel::OnTrackSelect(TrackSelectMsg *pMsg) {
    TnlPlayer *pPlayer = FindTnlPlayer(pMsg->mUnknown10);
    if (pPlayer == nullptr) {
        return;
    }
    const int nTrack = pMsg->mUnknown04;
    const int nLevel = pMsg->mUnknown08;
    GetCachedTunnelObject()->GetSeeker(pPlayer->mIndex)->SetTargetRing(nTrack);
    pPlayer->mActivator.MoveToTrack(nLevel, mTrackModes[nTrack], static_cast<float>(nTrack));
    pPlayer->mGridMarkers.SetTrack(nTrack);
    if (g_nAppTunnelDisplayMode) {
        CallScriptTemplate(kTrackSelectScriptTemplate, pMsg->mUnknown04);
    }
    mNowRing->SetPlayerMesh(pPlayer->mIndex, nTrack);
}

// 0x00448048
void AppTunnel::OnAdvanceSectionToggle() {
    mBoundary->UpdateText();
    mUnknown140 = mPlayMap->FollowingStepBar(
        static_cast<int>(mRenderer->mSongTick / static_cast<float>(kFramesPerBar)));
    for (auto it = mPlayers.begin(); it != mPlayers.end(); ++it) {
        (*it)->mSabreTrail.Rebuild();
    }
    const int nBar = static_cast<int>(mRenderer->mSongTick / static_cast<float>(kFramesPerBar));
    const int nEndBar = nBar + GetCachedTunnelObject()->mSliceCount;
    for (int nCellBar = (nBar > -1) ? nBar : 0; nCellBar < nEndBar; ++nCellBar) {
        for (int nTrack = 0; nTrack < mTrackCount; ++nTrack) {
            Renderer::Cell *pCell = mRenderer->GetCell(nTrack, nCellBar);
            OnBarChanged(nTrack, nCellBar, 1, pCell->mPlayer, pCell->mPowerup, pCell->mEnabled);
        }
    }
}

// 0x00448330
void AppTunnel::OnPlaybackToggle(PlaybackToggleMsg *pMsg) {
    if (pMsg->mOn) {
        mCameraRig->ZoomIn();
    } else {
        mCameraRig->ZoomOut();
    }
    mJukebox = pMsg->mOn;
    std::list<TnlGem> &gems = mGemManager->mGems;
    for (auto it = gems.begin(); it != gems.end(); ++it) {
        it->Release();
        const int nTrack = it->mTrack;
        if (mJukebox) {
            if (it->mKind != mGhostGemKinds[nTrack]) {
                it->mKind = mTrackEffectKinds[nTrack];
            }
        } else if ((it->mKind == mTrackEffectKinds[nTrack]) &&
                   (mTrackModes[nTrack] != kTrackModeScratch)) {
            it->mKind = mShowCrates ? mCrateGemKind : mHexGemKinds[it->mColor];
        }
    }
    for (auto it = mPlayers.begin(); it != mPlayers.end(); ++it) {
        (*it)->mActivator.SetSuppressed(mJukebox);
    }
    mNowRing->SetShowing(!mJukebox);
}

// 0x00449240
void AppTunnel::OnWin(WinMsg *pMsg) {
    if (pMsg->mWinners.size() == 0) {
        return;
    }
    for (auto it = pMsg->mWinners.begin(); it != pMsg->mWinners.end(); ++it) {
        mArms->AttachTo(FindTnlPlayer(*it)->mLocalView);
    }
    mArms->Start(mRenderer->mSongTick);
    if (mGameMode == kGameModeSolo) {
        FindTnlPlayer(pMsg->mWinners[0])->mActivator.mBlink = 0;
        mLattice->Start(mRenderer->mSongTick);
    }
}

// 0x00449450
void AppTunnel::OnMultiplierState(MultiplierStateMsg *pMsg) {
    FindTnlPlayer(pMsg->mPlayer)->mActivator.mCatcher.SetMultiplied(pMsg->mBonus > 0);
}

// 0x00449500
void AppTunnel::OnPowerupFailed(PowerupFailedMsg *pMsg) {
    const float flScaledTick = mRenderer->mSongTick * mUnknown144;
    if (pMsg->mKind != kHudItemFreestyler) {
        return;
    }
    TnlPlayer *pPlayer = FindTnlPlayer(pMsg->mPlayer);
    for (int nTrack = 0; nTrack < kTrackCount; ++nTrack) {
        const TrackMode mode = mTrackModes[nTrack];
        if ((mode == kTrackModeAxe) || (mode == kTrackModeScratch) || (mode == kTrackModeVocal)) {
            mArrows[nTrack]->Show(pPlayer, flScaledTick + kArrowShowFrames);
        }
    }
}

// 0x00457ff0
inline void AppTunnel::OnAxeButton(AxeButtonMsg *pMsg) {
    TnlPointer &pointer = FindTnlPlayer(pMsg->mPlayer)->mActivator.mPointer;
    if (pMsg->mPressed) {
        pointer.Spin(pMsg->mUnknown08);
    } else {
        pointer.Reset();
    }
    if (g_nAppTunnelDisplayMode) {
        CallScriptTemplate(kAxeButtonScriptTemplate);
    }
}

// 0x004580e8
inline void AppTunnel::OnPlayersTrackNeutralized(PlayersTrackNeutralizedMsg *pMsg) {
    Application::shared()->GetWorld()->mForceFeedback->PlayEffect3(pMsg->mPlayer);
}

// 0x00458120
inline void AppTunnel::OnToggleGhost(ToggleGhostMsg *pMsg) {
    FindTnlPlayer(pMsg->mUnknown04)->mActivator.SetGhost(pMsg->mOn);
}

// 0x004581b8
inline void AppTunnel::OnJuiceAmount(JuiceAmountMsg *pMsg) {
    if ((mGameMode != kGameModeSolo) || (mPlayMode != kPlayModeGame)) {
        return;
    }
    TnlPlayer *pPlayer = FindTnlPlayer(pMsg->mUnknown04);
    pPlayer->mActivator.mBlink = (pMsg->GetJuiceFraction() < kLowJuiceFraction);
}

// 0x00446460
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
        (*it)->Update();
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
        if ((*it)->Start(pos)) {
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

// 0x004576a0
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

// 0x00457758
int AppTunnel::StartCrippleFX(const std::vector<TnlPlayer *> &targets, float flFrame) {
    for (auto it = mCrippleFX.begin(); it != mCrippleFX.end(); ++it) {
        if ((*it)->mState == TnlCrippleFX::kStateIdle) {
            (*it)->Start(targets, flFrame);
            return 1;
        }
    }
    return 0;
}

// 0x00457828
int AppTunnel::StartBumpFX(int nStep, const HxStr &colorName, int nForward, float flPathOffset) {
    for (auto it = mBumpFX.begin(); it != mBumpFX.end(); ++it) {
        if ((*it)->IsIdle()) {
            (*it)->Start(nStep, colorName, nForward, flPathOffset);
            return 1;
        }
    }
    return 0;
}

// 0x00457880
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

// 0x004579e0
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

// 0x00457ae0
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
