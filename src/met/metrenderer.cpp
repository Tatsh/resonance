#include "met/metrenderer.h"

#include <algorithm>
#include <list>

#include "app/application.h"
#include "app/playsound.h"
#include "app/watchdog.h"
#include "game/freqappearance.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "game/gamestats.h"
#include "game/globalsettings.h"
#include "gfx/gfxdevice.h"
#include "math/color.h"
#include "met/metcommandmap.h"
#include "met/metcommandrepeater.h"
#include "met/metfade.h"
#include "met/metfreqmakerassetmanager.h"
#include "met/metfrontendstate.h"
#include "met/metlogoscreen.h"
#include "met/metpersonadata.h"
#include "met/metremixmanager.h"
#include "met/metscreen.h"
#include "met/metsonglists.h"
#include "msg/gameconnectionlostmsg.h"
#include "msg/isrecordingmsg.h"
#include "msg/lobbyconnectionlostmsg.h"
#include "msg/metcontrollerreading.h"
#include "msg/metfreqendedmsg.h"
#include "msg/metstartnetlaunchmsg.h"
#include "msg/metstartpausemsg.h"
#include "msg/metunlockstagesmsg.h"
#include "msg/rawcontrollermsg.h"
#include "os/async.h"
#include "os/hxstr.h"
#include "os/r250.h"
#include "os/zone.h"
#include "profile/profiler.h"
#include "rnd/animatable.h"
#include "rnd/asyncloader.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/transformable.h"
#include "script/configquery.h"
#include "script/scripthost.h"
#include "synth/midi_main.h"
#include "synth/ps2hardsynth.h"

namespace {

constexpr long long kNanosecondsPerMillisecond = 1000000;

// Half a millisecond, added before the division that turns a nanosecond interval into
// milliseconds so that the quotient rounds rather than truncates.
constexpr long long kHalfMillisecondNs = 500000;

constexpr float kMillisecondsPerSecond = 1000.0f;

// Frame position the front end rewinds to when it starts running.
constexpr float kFirstFrame = 1.0f;

// Configuration code the start-up value is read under.
constexpr int kStartUpConfigCode = 0x193;

// The zone every front-end container loader allocates from.
constexpr char kLoaderZone[] = "rndglobal";

// The metagame, fonts, and shared-texture loaders whose progress PollCommonLoaders() averages.
constexpr float kCommonLoaderCount = 3.0f;

// The tag of a joystick reading, `joy `.
constexpr int kReadingTagJoystick = 0x6a6f7920;

// The translator's codes past kMetScreenCommandBack that OnRawController() also arms for repeat.
// Their meaning is not recovered.
constexpr int kRepeatCommandFirst = 0x10;
constexpr int kRepeatCommandLast = 0x13;

// Whether a scene list holds the matching subobject of a view. The list's element type selects
// the subobject the comparison converts the view to. The out-of-line copies are at 0x00370ab8 for
// the drawable list, at 0x00370b08 for the transformable list, and at 0x00370b58 for the
// animatable list.
template <class T>
inline bool ContainsRef(const std::list<T *> &list, Rnd::View *const &pView) {
    return std::find(list.begin(), list.end(), pView) != list.end();
}

// Reading of the frame clock in nanoseconds, measured from the origin the watchdog's clock
// recorded when the run started. MainLoop has its own copy of the same inline.
inline long long FrameClockNs(Watchdog *pWatchdog) {
    return (ProfileClockMilliseconds() - pWatchdog->mClock.mOriginMs) * kNanosecondsPerMillisecond;
}

// Milliseconds between two frame-clock readings, rounded rather than truncated.
inline int FrameIntervalMs(long long nNowNs, long long nThenNs) {
    return static_cast<int>((nNowNs - nThenNs + kHalfMillisecondNs) / kNanosecondsPerMillisecond);
}

// 0x006c359c
// Set to send the next finished game back to the logo screen. OnFreqEnded() is the one reader and
// clears it, and no routine in the image sets it. The name is inferred.
int g_nReturnToLogo;

// The front-end state phase in which a pause shows the plain pause screen. The meaning of the
// phase is not recovered.
constexpr int kPlainPausePhase = 5;

// The frame rate the constructor starts mUnknown64 at, in frames per second.
constexpr float kFrameRate = 500.0f;

// The highest pad index HandleMessage() accepts, which the constructor records in mUnknownd4.
constexpr int kHighestPadIndex = 4;

// Configuration codes of the two debug overlays the constructor reads.
constexpr int kTimingGraphConfigCode = 0x397;
constexpr int kRenderStatsConfigCode = 0x3a2;

// The three scene views ResolveSceneViews() resolves, and the view OnUnknownSlot7() shows while
// the disc cannot be read.
static const char *const kTopView = "met top view";
static const char *const kBackgroundView = "meta bg view";
static const char *const kScreensView = "metscreens.view";
static const char *const kDiscProblemView = "met_disc_prob.view";

// The first screen OnUnknownSlot7() activates once the boot containers have loaded.
static const char *const kStartupScreen = "MetMemDetectStartup";

// The front-end state phase in which OnUnknownSlot5() leaves the music playing. The meaning of
// the phase is not recovered.
constexpr int kNoMusicStopPhase = 2;

// The music OnUnknownSlot5() stops.
static const char *const kFrontEndMusic = "SND_MET_MUSIC1";

// The full scale OnUnknownSlot8() and OnUnknownSlot10() give the subsystem timing graph.
constexpr int kTimingGraphFullScaleMs = 50;

// The arena view ResolveArenaView() attaches.
static const char *const kArenaView = "Metagame_arena.view";

// The fade OnFreqEnded() starts, in frames.
constexpr float kFreqEndedFadeFrames = 360.0f;

// The two levels that return to the main screen and complete the tutorial.
static const char *const kTutorialLevel = "tutorial";
static const char *const kTutorialRemixLevel = "tutorialrmx";

// The script template OnFreqEnded() runs on leaving a jam, and its one argument.
constexpr int kJukeboxTemplate = 0x267;
static const char *const kJukeboxStopArgument = "0";

// The components of the two clear colours OnFreqEnded() sets.
constexpr float kClearBlue = 0.15f;
constexpr float kOpaque = 1.0f;

} // namespace

// 0x006c3598
MetRenderer *MetRenderer::sInstance;

// 0x006c3600
RndAsyncLoader *MetRenderer::sMetagameLoader;

// 0x006c3608
RndAsyncLoader *MetRenderer::sFontsLoader;

// 0x006c360c
RndAsyncLoader *MetRenderer::sSharedTexLoader;

// 0x006c3610
RndAsyncLoader *MetRenderer::sArenaLoader;

// 0x0036a900
void MetRenderer::OnUnknownSlot4() {
    if (mUnknown94 != nullptr) {
        mUnknown94->Reset();
    }

    mUnknowna8 = 1;
    mUnknown68 = kFirstFrame;
    mUnknown70 = FrameClockNs(Application::shared()->GetWatchdog());
    QueryConfigValue(kStartUpConfigCode); // Yes, the binary discards the result.
}

// 0x00369fb0
MetRenderer::MetRenderer()
    : mUnknown68(0.0f), mUnknown80(1), mUnknown60(0), mUnknown64(kFrameRate), mUnknown70(0),
      mUnknown78(0), mUnknown7c(nullptr), mUnknown90(nullptr), mUnknown94(nullptr), mUnknown98(0),
      mUnknown9c(nullptr), mUnknowna8(0), mUnknownac(0), mUnknownb0(0), mUnknownb4(1),
      mUnknownb8(0), mUnknownbc(0), mUnknownc4(nullptr), mUnknownc8(0), mUnknowncc(nullptr),
      mUnknownd0(0), mUnknownd4(kHighestPadIndex) {
    sInstance = this;
    MetFreqMakerAssetManager::Create();
    MetFreqMakerAssetManager::shared()->StartAssetLoad();
    MetFrontEndState::Create();
    GlobalSettings::Create();
    mUnknown94 = new MetCommandRepeater;
    mUnknown90 = new MetCommandMap;
    SeedR250(FrameIntervalMs(FrameClockNs(Application::shared()->GetWatchdog()), 0));
    RebuildStageLists();
    RebuildArenaLists();
    MetPersonaData::ClearSavedList();
    MetPersonaData::ClearLoadList();
    CreateCommonLoaders();
    mUnknownb8 = 1;
    EnqueueCommonLoaders();
    mUnknownac = QueryConfigFlag(kTimingGraphConfigCode);
    mUnknownb0 = QueryConfigFlag(kRenderStatsConfigCode);
    const Color black{0.0f, 0.0f, 0.0f, kOpaque};
    g_gfxDevice.SetClearColor(black);
    SetDoWinSequence(0);
}

// 0x0036a460
MetRenderer::~MetRenderer() {
    delete mUnknown94;
    mUnknown94 = nullptr;
    delete mUnknown90;
    mUnknown90 = nullptr;
    delete mUnknowncc;
    mUnknowncc = nullptr;
    OnUnknownSlot5();
    UnloadCommonLoaders();
    UnloadArenaLoader();
    sInstance = nullptr;
    MetFrontEndState::Destroy();
    GlobalSettings::Destroy();
    MetScreen::DestroyAllScreens();
    MetFreqMakerAssetManager::Destroy();
}

// 0x0036b0c0
void MetRenderer::OnUnknownSlot5() {
    mUnknowna8 = 0;
    if (mUnknown94 != nullptr) {
        mUnknown94->Reset();
    }
    while (mUnknown84.begin() != mUnknown84.end()) {
        mUnknown84.erase(mUnknown84.begin());
    }
    ClearScreenScene();
    ClearBackgroundScene();
    mUnknown7c = nullptr;
    if (MetFrontEndState::shared()->mUnknown18 != kNoMusicStopPhase) {
        StopSoundByName(kFrontEndMusic);
    }
}

// 0x0036b740
void MetRenderer::OnUnknownSlot9() {
    if (mUnknowna8 == 0) {
        return;
    }

    const long long nNowNs = FrameClockNs(Application::shared()->GetWatchdog());
    const int nIntervalMs = FrameIntervalMs(nNowNs, mUnknown70);

    mUnknown70 = nNowNs;
    mUnknown68 += mUnknown64 * static_cast<float>(nIntervalMs) / kMillisecondsPerSecond;

    for (std::vector<MetScreen *>::iterator it = mUnknown84.begin(); it != mUnknown84.end(); ++it) {
        (*it)->UpdateAnimationFrame(mUnknown68);

        if (mUnknowna8 == 0) {
            return;
        }

        // A screen that pushed or popped another one invalidated the iterator, so the walk is
        // abandoned rather than restarted. The screens past the change are not advanced this
        // frame.
        if (mUnknown98 != 0) {
            mUnknown98 = 0;
            break;
        }
    }

    mUnknown9c->SetFrame(mUnknown68);
    mUnknown9c->UpdateWorldXfm(nullptr, 0); // Yes, the binary discards the result.
}

// 0x003715e0
void MetRenderer::OnFadeOutDone() {
    mUnknownd0 = 0;

    if (mUnknownc8 != 0) {
        return;
    }

    MetScreen *const pPending = mUnknownc4;

    if (mUnknown94 != nullptr) {
        mUnknown94->Reset();
    }

    mUnknown7c = pPending;
    mUnknown80 = 1;
    AddScreen(mUnknown7c);
    mUnknown7c->PollContainerLoad(); // Yes, the binary discards the result.
    mUnknown7c->mUnknown4c = 1;
    mUnknown7c->mUnknown50 = 1;
}

// 0x003715d8
void MetRenderer::OnFadeInDone() {
}

// 0x003714c8
void MetRenderer::SetActivePanel(MetScreen *pScreen) {
    mUnknown7c = pScreen;

    if (mUnknown94 != nullptr) {
        mUnknown94->Reset();
    }
}

// 0x00390088
void MetRenderer::OnUnknown00390088() {
}

// 0x00390090
void MetRenderer::OnUnknown00390090() {
}

// 0x003719e0
void MetRenderer::AddScreen(MetScreen *pScreen) {
    if (std::find(mUnknown84.begin(), mUnknown84.end(), pScreen) != mUnknown84.end()) {
        return;
    }

    mUnknown84.push_back(pScreen);
    mUnknown98 = 1;
}

// 0x003717b0
void MetRenderer::AddScreenView(Rnd::View *pView) {
    if (!ContainsRef(mUnknowna0->GetDraws(), pView)) {
        mUnknowna0->AddDraw(pView, nullptr);
    }
    if (!ContainsRef(mUnknowna0->mTransList, pView)) {
        mUnknowna0->AddTrans(pView);
    }
    if (!ContainsRef(mUnknowna0->mAnims, pView)) {
        mUnknowna0->AddAnim(pView);
    }
}

// 0x003718b8
void MetRenderer::AddBackgroundView(Rnd::View *pView) {
    if (!ContainsRef(mUnknowna4->GetDraws(), pView)) {
        mUnknowna4->AddDraw(pView, nullptr);
    }
    if (!ContainsRef(mUnknowna4->mTransList, pView)) {
        mUnknowna4->AddTrans(pView);
    }
    if (!ContainsRef(mUnknowna4->mAnims, pView)) {
        mUnknowna4->AddAnim(pView);
    }
}

// 0x00371858
void MetRenderer::RemoveScreenView(Rnd::View *pView) {
    mUnknowna0->RemoveTrans(pView);
    mUnknowna0->RemoveDraw(pView);
    mUnknowna0->RemoveAnim(pView);
}

// 0x003719a0
void MetRenderer::ClearScreenScene() {
    mUnknowna0->ReleaseAnimsRefs();
    mUnknowna0->ClearDraws();
    mUnknowna0->ClearTransList();
}

// 0x00371960
void MetRenderer::ClearBackgroundScene() {
    mUnknowna4->ReleaseAnimsRefs();
    mUnknowna4->ClearDraws();
    mUnknowna4->ClearTransList();
}

// 0x003714f8
void MetRenderer::ActivatePanel(MetScreen *pScreen) {
    mUnknown7c = pScreen;
    if (mUnknown94 != nullptr) {
        mUnknown94->Reset();
    }
    mUnknown80 = 1;
    AddScreen(mUnknown7c);
    mUnknown7c->PollContainerLoad(); // Yes, the binary discards the result.
    mUnknown7c->mUnknown4c = 1;
    mUnknown7c->mUnknown50 = 1;
}

// 0x00371730
void MetRenderer::MoveScreenViewToFront(Rnd::View *pView) {
    if (!ContainsRef(mUnknowna0->GetDraws(), pView)) {
        AddScreenView(pView);
        return;
    }
    mUnknowna0->RemoveDraw(pView);
    mUnknowna0->AddDraw(pView, nullptr);
}

// 0x0036b8c8
void MetRenderer::UnlockAllStages() {
    if (mUnknown7c != nullptr) {
        MetUnlockStagesMsg msg;
        mUnknown7c->Handle(&msg);
    }
}

// 0x00371b58
int MetRenderer::IsLogoScreenActive() {
    return dynamic_cast<MetLogoScreen *>(mUnknown7c) != nullptr;
}

// 0x00371cb0
void MetRenderer::ForwardToPanel(Message *pMsg) {
    if (mUnknown7c != nullptr) {
        mUnknown7c->Handle(pMsg);
    }
}

// 0x00371cf0
void MetRenderer::ForwardToPanelUnchecked(Message *pMsg) {
    mUnknown7c->Handle(pMsg);
}

// 0x00371ba8
inline void MetRenderer::OnRawController(RawControllerMsg *pMsg) {
    if (mUnknowna8 == 0) {
        return;
    }
    MetControllerReading &reading = pMsg->mReading;
    if (mUnknownd4 < reading.mPadIndex) {
        return;
    }
    MetScreenCommand command;
    if (mUnknown90->Translate(&reading, &command) == 0) {
        return;
    }
    if (reading.mTag == kReadingTagJoystick) {
        switch (command.mCommand) {
        case kMetScreenCommandNone:
        case kMetScreenCommandPrevious:
        case kMetScreenCommandNext:
        case kMetScreenCommandLeft:
        case kMetScreenCommandRight:
        case kRepeatCommandFirst:
        case kRepeatCommandFirst + 1:
        case kRepeatCommandFirst + 2:
        case kRepeatCommandLast:
            mUnknown94->Arm(&command, reading.mButton, command.mPadIndex);
            break;
        default:
            break;
        }
    }
    if (mUnknown7c != nullptr && mUnknown80 != 0 && command.mCommand != kMetScreenCommandNone) {
        mUnknown7c->DeliverCommand(&command);
    }
}

// 0x0036aae0
MetScreen *MetRenderer::SelectEndScreen() {
    const GameParams params(*Application::shared()->GetGameManager()->GetParams());
    MetScreen *pScreen;
    if (params.mUnknown1c == kPlayModeJam) {
        // Yes, the binary looks each discarded screen up only for the fatal check.
        MetScreen::FindEndScreen(this, HxStr("MetSaveRemixScreen"));
        if (Application::shared()->GetGameManager()->GetParams()->mUnknown28) {
            pScreen = MetScreen::FindEndScreen(this, HxStr("MetNetEndRemixScreen"));
        } else if (Application::shared()->GetGameManager()->GetGameMode() == kGameModeSolo) {
            pScreen = MetScreen::FindEndScreen(this, HxStr("MetSoloEndRemixScreen"));
        } else {
            MetScreen::FindEndScreen(this, HxStr("MetMultiEndRemixScreen"));
            pScreen = MetScreen::FindEndScreen(this, HxStr("MetMultiSaveRemixScreen"));
        }
    } else if (Application::shared()->GetGameManager()->GetParams()->mUnknown28) {
        MetScreen::FindEndScreen(this, HxStr("MetMultiStatsScreen"));
        pScreen = MetScreen::FindEndScreen(this, HxStr("MetNetEndScreen"));
    } else if (Application::shared()->GetGameManager()->GetGameMode() == kGameModeSolo) {
        GameStats *pStats = Application::shared()->GetGameManager()->GetStats();
        MetScreen::FindEndScreen(this, HxStr("MetSoloStatsScreen"));
        if (pStats->mCompleted != 0 && pStats->mUnknown08 == 0) {
            pScreen = MetScreen::FindEndScreen(this, HxStr("MetStageFinishScreen"));
        } else {
            pScreen = MetScreen::FindEndScreen(this, HxStr("MetSoloLoseScreen"));
        }
    } else {
        MetScreen::FindEndScreen(this, HxStr("MetMultiStatsScreen"));
        pScreen = MetScreen::FindEndScreen(this, HxStr("MetMultiEndScreen"));
    }
    return pScreen;
}

// 0x00369b08
void MetRenderer::CreateCommonLoaders() {
    const int nZone = FindZoneByName(kLoaderZone);
    sMetagameLoader = new RndAsyncLoader(HxStr("MetaGame/"), HxStr("metagame.rnd"), nZone);
    sFontsLoader = new RndAsyncLoader(HxStr("metagame/fonts/"), HxStr("fonts.rnd"), nZone);
    sSharedTexLoader =
        new RndAsyncLoader(HxStr("metagame/shared/"), HxStr("shared_tex.rnd"), nZone);
}

// 0x00369e50
void MetRenderer::CreateArenaLoader() {
    const int nZone = FindZoneByName(kLoaderZone);
    sArenaLoader = new RndAsyncLoader(HxStr("MetaGame/Arena/"), HxStr("meta_arena.rnd"), nZone);
    MetScreen::CreateMainMenuScreens(sInstance);
    StartArenaLoad();
}

// 0x003712d8
void MetRenderer::StartArenaLoad() {
    EnqueueArenaLoader();
}

// 0x00371270
void MetRenderer::EnqueueCommonLoaders() {
    if (sMetagameLoader->mPending != 0) {
        sMetagameLoader->Enqueue();
    }
    if (sFontsLoader->mPending != 0) {
        sFontsLoader->Enqueue();
    }
    if (sSharedTexLoader->mPending != 0) {
        sSharedTexLoader->Enqueue();
    }
}

// 0x00371438
void MetRenderer::EnqueueArenaLoader() {
    if (sArenaLoader->mPending != 0) {
        sArenaLoader->Enqueue();
    }
}

// 0x003712f8
void MetRenderer::UnloadCommonLoaders() {
    sMetagameLoader->Unload();
    sFontsLoader->Unload();
    sSharedTexLoader->Unload();
}

// 0x00371490
void MetRenderer::UnloadArenaLoader() {
    sArenaLoader->Unload();
}

// 0x00371338
int MetRenderer::PollCommonLoaders(float *pfProgress) {
    *pfProgress = 0.0f;
    float flProgress;
    int bDone = sMetagameLoader->Poll(&flProgress);
    *pfProgress += flProgress;
    const int bFonts = sFontsLoader->Poll(&flProgress);
    *pfProgress += flProgress;
    bDone = bDone && bFonts;
    const int bSharedTex = sSharedTexLoader->Poll(&flProgress);
    *pfProgress = (*pfProgress + flProgress) / kCommonLoaderCount;
    return bDone && bSharedTex;
}

// 0x003713f0
int MetRenderer::PollArenaLoader(float *pfProgress) {
    *pfProgress = 0.0f;
    float flProgress;
    const int bDone = sArenaLoader->Poll(&flProgress);
    *pfProgress += flProgress;
    return bDone;
}

// 0x0036a9e0
void MetRenderer::ResolveArenaView(int nSkipResolve) {
    Rnd::View *pView = nullptr;
    if (nSkipResolve == 0) {
        float flProgress;
        sArenaLoader->Poll(&flProgress); // Yes, the binary discards the result.
        pView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kArenaView)));
    }
    if (pView != nullptr) {
        AddBackgroundView(pView);
    }
}

// 0x00371670
void MetRenderer::OnUnknownSlot8() {
    if (mUnknowna8 == 0) {
        return;
    }
    FreqAppearance::RenderBurnTextures();
    mUnknown9c->Rnd::Drawable::Draw();
    for (std::vector<MetScreen *>::iterator it = mUnknown84.begin(); it != mUnknown84.end(); ++it) {
        (*it)->OnDrawPass();
    }
    if (mUnknownac != 0) {
        g_gfxDevice.DrawSubsystemTimingGraph(kTimingGraphFullScaleMs);
    }
    if (mUnknownb0 != 0) {
        g_gfxDevice.DrawRenderStatsOverlay();
    }
}

// 0x00371570
void MetRenderer::OnUnknownSlot10() {
    if (mUnknowna8 == 0) {
        return;
    }
    mUnknown9c->Rnd::Drawable::Draw();
    if (mUnknownac != 0) {
        g_gfxDevice.DrawSubsystemTimingGraph(kTimingGraphFullScaleMs);
    }
    if (mUnknownb0 != 0) {
        g_gfxDevice.DrawRenderStatsOverlay();
    }
}

// 0x0036c5d8
void MetRenderer::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == g_nRawControllerMsgType) {
        OnRawController(static_cast<RawControllerMsg *>(pMsg));
    } else if (nType == g_nMetStartNetLaunchMsgType) {
        ForwardToPanel(pMsg);
    } else if (nType == g_nMetStartPauseMsgType) {
        OnStartPause(pMsg);
    } else if (nType == g_nGameConnectionLostMsgType) {
        // Yes, the binary drops this message rather than forwarding it.
    } else if (nType == g_nLobbyConnectionLostMsgType) {
        ForwardToPanelUnchecked(pMsg);
    } else if (nType == g_nIsRecordingMsgType) {
        mUnknown78 = static_cast<IsRecordingMsg *>(pMsg)->mIsRecording;
    } else if (nType == g_nMetFreqEndedMsgType) {
        OnFreqEnded(pMsg);
    } else {
        ForwardToPanel(pMsg);
    }
}

// 0x0036b938
void MetRenderer::OnStartPause([[maybe_unused]] Message *pMsg) {
    OnUnknownSlot4();
    if (MetFrontEndState::shared()->mUnknown18 == kPlainPausePhase) {
        ActivatePanel(MetScreen::FindScreenByName(HxStr("MetPauseGameScreen")));
    } else if (Application::shared()->GetGameMode() == kGameModeLocal) {
        if (Application::shared()->GetPlayMode() == kPlayModeGame) {
            ActivatePanel(MetScreen::FindScreenByName(HxStr("MetPauseGameScreen")));
        } else {
            ActivatePanel(MetScreen::FindScreenByName(HxStr("MetPauseMultiRemixScreen")));
        }
    } else if (Application::shared()->GetGameManager()->GetGameMode() == kGameModeSolo) {
        if (Application::shared()->GetPlayMode() == kPlayModeGame) {
            ActivatePanel(MetScreen::FindScreenByName(HxStr("MetPauseSoloGameScreen")));
        } else {
            ActivatePanel(MetScreen::FindScreenByName(HxStr("MetPauseSoloRemixScreen")));
        }
    }
}

// 0x0036bcb8
void MetRenderer::OnFreqEnded(Message *pMsg) {
    MetFreqEndedMsg *pEnded = static_cast<MetFreqEndedMsg *>(pMsg);
    const GameParams params(*Application::shared()->GetGameManager()->GetParams());
    SetDoWinSequence(0);

    if (params.mJukeboxMode == 1 && pEnded->mUnknownb8Clear == 0) {
        const Color black{0.0f, 0.0f, 0.0f, kOpaque};
        g_gfxDevice.SetClearColor(black);
        OnUnknownSlot4();
        AddScreen(MetScreen::FindScreenByName(HxStr("MetRemixManager")));
        MetRemixManager::shared()->PlayCurrentTrack();
        return;
    }

    const Color blue{0.0f, 0.0f, kClearBlue, kOpaque};
    g_gfxDevice.SetClearColor(blue);
    mUnknownc8 = 1;
    mUnknownc4 = nullptr;

    if (mUnknown78 != 0 || g_nReturnToLogo != 0) {
        mUnknown78 = 0;
        g_nReturnToLogo = 0;
        OnUnknown00390088();
        mUnknownc4 = MetScreen::FindScreenByName(HxStr("MetLogoScreen"));
    } else if (params.mLevelName == kTutorialLevel || params.mLevelName == kTutorialRemixLevel) {
        OnUnknown00390088();
        if (GlobalSettings::shared()->mTutorialComplete == 0) {
            GlobalSettings::shared()->mTutorialComplete = 1;
            if (MetFrontEndState::shared()->mUnknown0c != 0) {
                MetFrontEndState::shared()->mUnknown10 = 1;
            }
        }
        mUnknownc4 = MetScreen::FindScreenByName(HxStr("MetMainScreen"));
    } else if (params.mUnknown1c == kPlayModeJam) {
        GameStats *pStats = Application::shared()->GetGameManager()->GetStats();
        if (params.mJukeboxMode != 0) {
            GameParams cleared(*Application::shared()->GetGameManager()->GetParams());
            cleared.mLoadingGame = 0;
            Application::shared()->GetGameManager()->SetParams(cleared);
            CallScriptTemplate(kJukeboxTemplate, kJukeboxStopArgument);
            OnUnknown00390088();
            OnUnknown00390090();
            MetRemixManager::shared()->LeaveJukeboxMode();
            mUnknownc4 = MetScreen::FindScreenByName(HxStr("MetJukeboxTopButtonsScreen"));
        } else if (MetFrontEndState::shared()->mUnknown0c != 0 && pStats->mUnknown14 != 0) {
            mUnknownc4 = SelectEndScreen();
        } else {
            OnUnknown00390088();
            OnUnknown00390090();
            mUnknownc4 = MetScreen::FindScreenByName(HxStr("MetRemixTypeScreen"));
            GameParams cleared(*Application::shared()->GetGameManager()->GetParams());
            cleared.mLoadingGame = 0;
            Application::shared()->GetGameManager()->SetParams(cleared);
            CallScriptTemplate(kJukeboxTemplate, kJukeboxStopArgument);
        }
    } else if (pEnded->mUnknownb8Clear != 0) {
        OnUnknown00390088();
        if (params.mUnknown28 != 1) {
            OnUnknown00390090();
            mUnknownc4 = MetScreen::FindScreenByName(HxStr("MetSoloStagesScreen"));
            GameParams cleared(*Application::shared()->GetGameManager()->GetParams());
            cleared.mLoadingGame = 0;
            Application::shared()->GetGameManager()->SetParams(cleared);
            CallScriptTemplate(kJukeboxTemplate, kJukeboxStopArgument);
        }
    } else {
        mUnknownc4 = SelectEndScreen();
    }

    mUnknownd0 = 1;
    OnUnknownSlot4();
    mUnknowncc->FadeOut(kFreqEndedFadeFrames, mUnknown68, this, 0);
    ResolveArenaView(0);
}

// 0x0036a680
void MetRenderer::ResolveSceneViews() {
    // Yes, the binary polls the three boot loaders again and discards every result.
    float flProgress;
    sMetagameLoader->Poll(&flProgress);
    sFontsLoader->Poll(&flProgress);
    sSharedTexLoader->Poll(&flProgress);
    mUnknown9c = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kTopView)));
    mUnknowna4 = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kBackgroundView)));
    mUnknowna0 = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kScreensView)));
    MetScreen::CreateStartupScreens(this);
    mUnknowncc = new MetFade(this);
}

// 0x0036b190
void MetRenderer::OnUnknownSlot7() {
    if (mUnknownb8 != 0) {
        float flProgress;
        const int bMetagame = sMetagameLoader->Poll(&flProgress);
        const int bFonts = sFontsLoader->Poll(&flProgress);
        const int bSharedTex = sSharedTexLoader->Poll(&flProgress);
        if (bMetagame != 0 && bFonts != 0 && bSharedTex != 0) {
            mUnknownb8 = 0;
            ResolveSceneViews();
            OnUnknownSlot4();
            CreateArenaLoader();
            ActivatePanel(MetScreen::FindScreenByName(HxStr(kStartupScreen)));
            const Color black{0.0f, 0.0f, 0.0f, kOpaque};
            g_gfxDevice.SetClearColor(black);
        }
    }

    if (mUnknownc8 != 0) {
        if (IsBankXferBusy() != 0) {
            AsyncPumpCompletedRequests();
        } else {
            Application::shared()->GetSynth()->AllNotesOff();
            mUnknownc8 = 0;
            PlaySoundByName(kFrontEndMusic);
            const Color blue{0.0f, 0.0f, kClearBlue, kOpaque};
            g_gfxDevice.SetClearColor(blue);
            if (mUnknownd0 == 0) {
                ActivatePanel(mUnknownc4);
            }
        }
    }

    if (mUnknownb8 == 0) {
        if (IsMediaReady() == 0) {
            Rnd::Drawable *pProblem =
                dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kDiscProblemView)));
            pProblem->SetShowing(1);
            return;
        }
        Rnd::Drawable *pProblem =
            dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kDiscProblemView)));
        pProblem->SetShowing(0);
    }

    if (mUnknowna8 == 0) {
        return;
    }
    MetScreen::PollContainerLoads();

    const long long nNowNs = FrameClockNs(Application::shared()->GetWatchdog());
    const int nIntervalMs = FrameIntervalMs(nNowNs, mUnknown70);
    mUnknown70 = nNowNs;
    mUnknown68 += mUnknown64 * static_cast<float>(nIntervalMs) / kMillisecondsPerSecond;

    if (mUnknownd0 != 0) {
        mUnknowncc->Update(mUnknown68);
    } else {
        for (std::vector<MetScreen *>::iterator it = mUnknown84.begin(); it != mUnknown84.end();
             ++it) {
            (*it)->UpdateFrame(mUnknown68);
            if (mUnknowna8 == 0) {
                return;
            }
            // A screen that pushed or popped another one invalidated the iterator.
            if (mUnknown98 != 0) {
                mUnknown98 = 0;
                break;
            }
        }
        if (mUnknowna8 == 0) {
            return;
        }
        if (mUnknown80 != 0) {
            mUnknown94->Update(mUnknown7c, &nNowNs);
        }
    }
    mUnknown9c->SetFrame(mUnknown68);
    mUnknown9c->UpdateWorldXfm(nullptr, 0); // Yes, the binary discards the result.
}

// 0x00371a78
void MetRenderer::RemoveScreen(MetScreen *pScreen) {
    for (std::vector<MetScreen *>::iterator it = mUnknown84.begin(); it != mUnknown84.end(); ++it) {
        if (*it == pScreen) {
            Rnd::View *pView = pScreen->mUnknown14;
            mUnknowna0->RemoveTrans(pView);
            mUnknowna0->RemoveDraw(pView);
            mUnknowna0->RemoveAnim(pView);
            mUnknown84.erase(it);
            mUnknown98 = 1;
            return;
        }
    }
}
