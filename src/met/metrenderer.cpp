#include "met/metrenderer.h"

#include <algorithm>
#include <list>

#include "app/application.h"
#include "app/playsound.h"
#include "app/scheduler.h"
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
#include "met/metstrings.h"
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
#include "os/cycles.h"
#include "os/formatstring.h"
#include "os/hostmode.h"
#include "os/hxstr.h"
#include "os/r250.h"
#include "os/zone.h"
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

constexpr float kMillisecondsPerSecondFloat = 1000.0f;

// Frame position the front end rewinds to when it starts running.
constexpr float kFirstFrame = 1.0f;

// Configuration code the start-up value is read under.
constexpr int kStartUpConfigCode = 0x193;

// The zone every front-end container loader allocates from.
constexpr char kLoaderZone[] = "rndglobal";

// The metagame, fonts, and shared-texture loaders whose progress ProgressMetGlobal() averages.
constexpr float kCommonLoaderCount = 3.0f;

// The tag of a joystick reading, `joy `.
constexpr int kReadingTagJoystick = 0x6a6f7920;

// The translator's codes past kMetScreenCommandBack that OnRawController() also arms for repeat.
// Their meaning is not recovered.
constexpr int kRepeatCommandFirst = 0x10;
constexpr int kRepeatCommandLast = 0x13;

// Whether a scene list includes the matching subobject of a view. The list's element type selects
// the subobject the comparison converts the view to.
template <class T>
inline bool ContainsRef(const std::list<T *> &list, Rnd::View *const &pView) {
    return std::find(list.begin(), list.end(), pView) != list.end();
}

// NTSC-U/C: 0x00370ab8, PAL: 0x0039f590
template bool ContainsRef<Rnd::Drawable>(const std::list<Rnd::Drawable *> &list,
                                         Rnd::View *const &pView);

// NTSC-U/C: 0x00370b08, PAL: 0x0039f5e0
template bool ContainsRef<Rnd::Transformable>(const std::list<Rnd::Transformable *> &list,
                                              Rnd::View *const &pView);

// NTSC-U/C: 0x00370b58, PAL: 0x0039f630
template bool ContainsRef<Rnd::Animatable>(const std::list<Rnd::Animatable *> &list,
                                           Rnd::View *const &pView);

// Reading of the frame clock in nanoseconds, measured from the origin the watchdog's clock
// recorded when the run started. MainLoop has a copy of the same inline.
inline long long FrameClockNs(Sch::Scheduler *pWatchdog) {
    return (GetElapsedMilliseconds() - pWatchdog->mClock.mOriginMs) * kNanosecondsPerMillisecond;
}

// Milliseconds between two frame-clock readings, rounded rather than truncated.
inline int FrameIntervalMs(long long nNowNs, long long nThenNs) {
    return static_cast<int>((nNowNs - nThenNs + kHalfMillisecondNs) / kNanosecondsPerMillisecond);
}

// NTSC-U/C: 0x006c359c, PAL: 0x0070660c
// Set to send the next finished game back to the logo screen. OnFreqEnded() is the one reader and
// clears it, and no routine in the image sets it. It starts set. Only the first finished game
// therefore returns to the logo screen. The name is inferred.
int g_nReturnToLogo = 1;

// The front-end state phase in which a pause shows the plain pause screen. The meaning of the
// phase is not recovered.
constexpr int kPlainPausePhase = 5;

// The frame rate the constructor starts mFrameRate at, in frames per second.
constexpr float kFrameRate = 500.0f;

// The highest pad index DispatchPriv() accepts. The constructor records it in mMaxPadIndex.
constexpr int kHighestPadIndex = 4;

// Configuration codes of the two debug overlays the constructor reads.
constexpr int kTimingGraphConfigCode = 0x397;
constexpr int kRenderStatsConfigCode = 0x3a2;

// The three scene views ResolveSceneViews() resolves, and the view Update() shows while
// the disc cannot be read.
static const char *const kTopView = "met top view";
static const char *const kBackgroundView = "meta bg view";
static const char *const kScreensView = "metscreens.view";
static const char *const kDiscProblemView = "met_disc_prob.view";

// The first screen Update() activates once the boot containers have loaded.
static const char *const kStartupScreen = "MetMemDetectStartup";

// The front-end state phase in which Stop() lets the music play on. The meaning of
// the phase is not recovered.
constexpr int kNoMusicStopPhase = 2;

// The music Stop() stops.
static const char *const kFrontEndMusic = "SND_MET_MUSIC1";

// The full scale Draw() and DrawSimple() give the subsystem timing graph.
constexpr int kTimingGraphFullScaleMs = 50;

// The arena view ResolveArenaView() attaches.
static const char *const kArenaView = "Metagame_arena.view";

// The fade OnFreqEnded() starts, in frames.
constexpr float kFreqEndedFadeFrames = 360.0f;

// The two levels that return to the main screen and complete the tutorial.
static const char *const kTutorialLevel = "tutorial";
static const char *const kTutorialRemixLevel = "tutorialrmx";

#ifdef VIDEO_STANDARD_PAL
// The European release gives the tutorial level names of each language a suffix.
enum TutorialLevelKind {
    kTutorialLevelGame = 1,
    kTutorialLevelRemix = 2,
};

// PAL: 0x00397ba0
// Build the tutorial level name of a kind for the current language.
inline HxStr TutorialLevelName(int nKind) {
    HxStr name;
    name = nKind == kTutorialLevelGame ? kTutorialLevel : kTutorialRemixLevel;
    HxStr suffix;
    suffix = LocalizedAssetSuffix();
    name += suffix;
    return name;
}
#endif

// The script template OnFreqEnded() runs on exiting a jam, and its one argument.
constexpr int kJukeboxTemplate = 0x267;
static const char *const kJukeboxStopArgument = "0";

// The components of the two clear colours OnFreqEnded() sets.
constexpr float kClearBlue = 0.15f;
constexpr float kOpaque = 1.0f;

// The metagame container, `MetaGame/metagame.rnd`.
// NTSC-U/C: 0x006c3600, PAL: 0x00706670
RndAsyncLoader *gTopViewLoader;

// NTSC-U/C: 0x006c3608, PAL: 0x00706678
RndAsyncLoader *gFontLoader;

// The shared texture container, `metagame/shared/shared_tex.rnd`.
// NTSC-U/C: 0x006c360c, PAL: 0x0070667c
RndAsyncLoader *gButtonLoader;

// NTSC-U/C: 0x006c3610, PAL: 0x00706680
RndAsyncLoader *gArenaLoader;

} // namespace

// NTSC-U/C: 0x006c3598, PAL: 0x00706608
MetRenderer *MetRenderer::sInstance;

void MetRenderer::Start() {
    if (mCommandRepeater != nullptr) {
        mCommandRepeater->Reset();
    }

    mRunning = 1;
    mAnimationFrame = kFirstFrame;
    mPreviousFrameNs = FrameClockNs(Application::shared()->GetWatchdog());
    QueryConfigValue(kStartUpConfigCode); // Yes, the binary discards the result.
}

MetRenderer::MetRenderer()
    : mAnimationFrame(0.0f), mPanelActive(1), mTitlePromptShowing(0), mFrameRate(kFrameRate),
      mPreviousFrameNs(0), mRecording(0), mActivePanel(nullptr), mCommandMap(nullptr),
      mCommandRepeater(nullptr), mScreensChanged(0), mTopView(nullptr), mRunning(0),
      mShowTimingGraph(0), mShowRenderStats(0), mUnreadFlag(1), mBootLoadPending(0),
      mUnreadValue(0), mPendingPanel(nullptr), mDiscProblemPending(0), mFade(nullptr), mFading(0),
      mMaxPadIndex(kHighestPadIndex) {
    sInstance = this;
#ifdef VIDEO_STANDARD_PAL
    LoadMetStrings();
#endif
    MetFreqMakerAssetManager::Create();
    MetFreqMakerAssetManager::shared()->StartAssetLoad();
    MetFrontEndState::Create();
    GlobalSettings::Create();
    mCommandRepeater = new MetCommandRepeater;
    mCommandMap = new MetCommandMap;
    SeedR250(FrameIntervalMs(FrameClockNs(Application::shared()->GetWatchdog()), 0));
    RebuildStageLists();
    RebuildArenaLists();
    MetPersonaData::ClearSavedList();
    MetPersonaData::ClearLoadList();
    CreateCommonLoaders();
    mBootLoadPending = 1;
    LoadMetGlobal();
    mShowTimingGraph = QueryConfigFlag(kTimingGraphConfigCode);
    mShowRenderStats = QueryConfigFlag(kRenderStatsConfigCode);
    const Color black{0.0f, 0.0f, 0.0f, kOpaque};
    Rnd::ThePs.SetClearColor(black);
    SetDoWinSequence(0);
}

MetRenderer::~MetRenderer() {
    delete mCommandRepeater;
    mCommandRepeater = nullptr;
    delete mCommandMap;
    mCommandMap = nullptr;
    delete mFade;
    mFade = nullptr;
    Stop();
    ReleaseMetGlobal();
    UnloadArenaLoader();
    sInstance = nullptr;
    MetFrontEndState::Destroy();
    GlobalSettings::Destroy();
    MetScreen::DestroyAllScreens();
    MetFreqMakerAssetManager::Destroy();
}

void MetRenderer::Stop() {
    mRunning = 0;
    if (mCommandRepeater != nullptr) {
        mCommandRepeater->Reset();
    }
    while (mScreens.begin() != mScreens.end()) {
        mScreens.erase(mScreens.begin());
    }
    ClearScreenScene();
    ClearBackgroundScene();
    mActivePanel = nullptr;
    if (MetFrontEndState::shared()->mPendingTransition != kNoMusicStopPhase) {
        StopSoundByName(kFrontEndMusic);
    }
}

void MetRenderer::UpdateSimple() {
    if (mRunning == 0) {
        return;
    }

    const long long nNowNs = FrameClockNs(Application::shared()->GetWatchdog());
    const int nIntervalMs = FrameIntervalMs(nNowNs, mPreviousFrameNs);

    mPreviousFrameNs = nNowNs;
    mAnimationFrame += mFrameRate * static_cast<float>(nIntervalMs) / kMillisecondsPerSecondFloat;

    for (std::vector<MetScreen *>::iterator it = mScreens.begin(); it != mScreens.end(); ++it) {
        (*it)->UpdateAnimationFrame(mAnimationFrame);

        if (mRunning == 0) {
            return;
        }

        // A screen that pushed or popped another one invalidated the iterator. The walk is
        // abandoned rather than restarted. The screens past the change are not advanced this
        // frame.
        if (mScreensChanged != 0) {
            mScreensChanged = 0;
            break;
        }
    }

    mTopView->SetFrame(mAnimationFrame);
    mTopView->UpdateWorldXfm(nullptr, 0); // Yes, the binary discards the result.
}

void MetRenderer::OnFadeOutDone() {
    mFading = 0;

    if (mDiscProblemPending != 0) {
        return;
    }

    MetScreen *const pPending = mPendingPanel;

    if (mCommandRepeater != nullptr) {
        mCommandRepeater->Reset();
    }

    mActivePanel = pPending;
    mPanelActive = 1;
    AddScreen(mActivePanel);
    mActivePanel->PollContainerLoad(); // Yes, the binary discards the result.
    mActivePanel->mEnterPending = 1;
    mActivePanel->mActivatePending = 1;
}

void MetRenderer::OnFadeInDone() {
}

void MetRenderer::SetActivePanel(MetScreen *pScreen) {
    mActivePanel = pScreen;

    if (mCommandRepeater != nullptr) {
        mCommandRepeater->Reset();
    }
}

void MetRenderer::OnReturnFromGame() {
}

void MetRenderer::OnReturnToMenus() {
}

void MetRenderer::AddScreen(MetScreen *pScreen) {
    if (std::find(mScreens.begin(), mScreens.end(), pScreen) != mScreens.end()) {
        return;
    }

    mScreens.push_back(pScreen);
    mScreensChanged = 1;
}

void MetRenderer::AddScreenView(Rnd::View *pView) {
    if (!ContainsRef(mScreenScene->GetDraws(), pView)) {
        mScreenScene->AddDraw(pView, nullptr);
    }
    if (!ContainsRef(mScreenScene->mTransList, pView)) {
        mScreenScene->AddTrans(pView);
    }
    if (!ContainsRef(mScreenScene->mAnims, pView)) {
        mScreenScene->AddAnim(pView);
    }
}

void MetRenderer::AddBackgroundView(Rnd::View *pView) {
    if (!ContainsRef(mBackgroundScene->GetDraws(), pView)) {
        mBackgroundScene->AddDraw(pView, nullptr);
    }
    if (!ContainsRef(mBackgroundScene->mTransList, pView)) {
        mBackgroundScene->AddTrans(pView);
    }
    if (!ContainsRef(mBackgroundScene->mAnims, pView)) {
        mBackgroundScene->AddAnim(pView);
    }
}

void MetRenderer::RemoveScreenView(Rnd::View *pView) {
    mScreenScene->RemoveTrans(pView);
    mScreenScene->RemoveDraw(pView);
    mScreenScene->RemoveAnim(pView);
}

void MetRenderer::ClearScreenScene() {
    mScreenScene->RemoveAllAnims();
    mScreenScene->RemoveAllDraws();
    mScreenScene->RemoveAllTranses();
}

void MetRenderer::ClearBackgroundScene() {
    mBackgroundScene->RemoveAllAnims();
    mBackgroundScene->RemoveAllDraws();
    mBackgroundScene->RemoveAllTranses();
}

void MetRenderer::ActivatePanel(MetScreen *pScreen) {
    mActivePanel = pScreen;
    if (mCommandRepeater != nullptr) {
        mCommandRepeater->Reset();
    }
    mPanelActive = 1;
    AddScreen(mActivePanel);
    mActivePanel->PollContainerLoad(); // Yes, the binary discards the result.
    mActivePanel->mEnterPending = 1;
    mActivePanel->mActivatePending = 1;
}

void MetRenderer::MoveScreenViewToFront(Rnd::View *pView) {
    if (!ContainsRef(mScreenScene->GetDraws(), pView)) {
        AddScreenView(pView);
        return;
    }
    mScreenScene->RemoveDraw(pView);
    mScreenScene->AddDraw(pView, nullptr);
}

void MetRenderer::UnlockAllStages() {
    if (mActivePanel != nullptr) {
        MetUnlockStagesMsg msg;
        mActivePanel->Dispatch(&msg);
    }
}

int MetRenderer::IsLogoScreenActive() {
    return dynamic_cast<MetLogoScreen *>(mActivePanel) != nullptr;
}

void MetRenderer::ForwardToPanel(Message *pMsg) {
    if (mActivePanel != nullptr) {
        mActivePanel->Dispatch(pMsg);
    }
}

void MetRenderer::ForwardToPanelUnchecked(Message *pMsg) {
    mActivePanel->Dispatch(pMsg);
}

inline void MetRenderer::OnRawController(RawControllerMsg *pMsg) {
    if (mRunning == 0) {
        return;
    }
    MetControllerReading &reading = pMsg->mReading;
    if (mMaxPadIndex < reading.mPadIndex) {
        return;
    }
    MetScreenCommand command;
    if (mCommandMap->Translate(&reading, &command) == 0) {
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
            mCommandRepeater->Arm(&command, reading.mButton, command.mPadIndex);
            break;
        default:
            break;
        }
    }
    if (mActivePanel != nullptr && mPanelActive != 0 && command.mCommand != kMetScreenCommandNone) {
        mActivePanel->DeliverCommand(&command);
    }
}

MetScreen *MetRenderer::SelectEndScreen() {
    const GameParams params(*Application::shared()->GetGameManager()->GetParams());
    MetScreen *pScreen;
    if (params.mPlayMode == kPlayModeJam) {
        // Yes, the binary looks each discarded screen up only for the fatal check.
        MetScreen::FindEndScreen(this, HxStr("MetSaveRemixScreen"));
        if (Application::shared()->GetGameManager()->GetParams()->mNetGame) {
            pScreen = MetScreen::FindEndScreen(this, HxStr("MetNetEndRemixScreen"));
        } else if (Application::shared()->GetGameManager()->GetGameMode() == kGameModeSolo) {
            pScreen = MetScreen::FindEndScreen(this, HxStr("MetSoloEndRemixScreen"));
        } else {
            MetScreen::FindEndScreen(this, HxStr("MetMultiEndRemixScreen"));
            pScreen = MetScreen::FindEndScreen(this, HxStr("MetMultiSaveRemixScreen"));
        }
    } else if (Application::shared()->GetGameManager()->GetParams()->mNetGame) {
        MetScreen::FindEndScreen(this, HxStr("MetMultiStatsScreen"));
        pScreen = MetScreen::FindEndScreen(this, HxStr("MetNetEndScreen"));
    } else if (Application::shared()->GetGameManager()->GetGameMode() == kGameModeSolo) {
        GameStats *pStats = Application::shared()->GetGameManager()->GetStats();
        MetScreen::FindEndScreen(this, HxStr("MetSoloStatsScreen"));
        if (pStats->mCompleted != 0 && pStats->mCheated == 0) {
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

void MetRenderer::CreateCommonLoaders() {
    const int nZone = FindZoneByName(kLoaderZone);
    gTopViewLoader = new RndAsyncLoader(HxStr("MetaGame/"), HxStr("metagame.rnd"), nZone);
#ifdef VIDEO_STANDARD_PAL
    gFontLoader = new RndAsyncLoader(HxStr("metagame/fonts/"),
                                     HxStr(Rnd::MakeString("fonts%s.rnd", GetFontLanguageSuffix())),
                                     nZone);
#else
    gFontLoader = new RndAsyncLoader(HxStr("metagame/fonts/"), HxStr("fonts.rnd"), nZone);
#endif
    gButtonLoader = new RndAsyncLoader(HxStr("metagame/shared/"), HxStr("shared_tex.rnd"), nZone);
}

void MetRenderer::CreateArenaLoader() {
    const int nZone = FindZoneByName(kLoaderZone);
    gArenaLoader = new RndAsyncLoader(HxStr("MetaGame/Arena/"), HxStr("meta_arena.rnd"), nZone);
    MetScreen::CreateMainMenuScreens(sInstance);
    StartArenaLoad();
}

void MetRenderer::StartArenaLoad() {
    EnqueueArenaLoader();
}

void LoadMetGlobal() {
    if (gTopViewLoader->mPending != 0) {
        gTopViewLoader->Enqueue();
    }
    if (gFontLoader->mPending != 0) {
        gFontLoader->Enqueue();
    }
    if (gButtonLoader->mPending != 0) {
        gButtonLoader->Enqueue();
    }
}

void MetRenderer::EnqueueArenaLoader() {
    if (gArenaLoader->mPending != 0) {
        gArenaLoader->Enqueue();
    }
}

void ReleaseMetGlobal() {
    gTopViewLoader->Unload();
    gFontLoader->Unload();
    gButtonLoader->Unload();
}

void MetRenderer::UnloadArenaLoader() {
    gArenaLoader->Unload();
}

int ProgressMetGlobal(float &flProgress) {
    flProgress = 0.0f;
    float flLoad;
    int bDone = gTopViewLoader->Poll(&flLoad);
    flProgress += flLoad;
    const int bFonts = gFontLoader->Poll(&flLoad);
    flProgress += flLoad;
    bDone = bDone && bFonts;
    const int bButtons = gButtonLoader->Poll(&flLoad);
    flProgress = (flProgress + flLoad) / kCommonLoaderCount;
    return bDone && bButtons;
}

int MetRenderer::PollArenaLoader(float *pfProgress) {
    *pfProgress = 0.0f;
    float flProgress;
    const int bDone = gArenaLoader->Poll(&flProgress);
    *pfProgress += flProgress;
    return bDone;
}

void MetRenderer::ResolveArenaView(int nSkipResolve) {
    Rnd::View *pView = nullptr;
    if (nSkipResolve == 0) {
        float flProgress;
        gArenaLoader->Poll(&flProgress); // Yes, the binary discards the result.
        pView = dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(HxStr(kArenaView)));
    }
    if (pView != nullptr) {
        AddBackgroundView(pView);
    }
}

void MetRenderer::Draw() {
    if (mRunning == 0) {
        return;
    }
    FreqAppearance::RenderBurnTextures();
    mTopView->Rnd::Drawable::Draw();
    for (std::vector<MetScreen *>::iterator it = mScreens.begin(); it != mScreens.end(); ++it) {
        (*it)->OnDrawPass();
    }
    if (mShowTimingGraph != 0) {
        Rnd::ThePs.DrawSubsystemTimingGraph(kTimingGraphFullScaleMs);
    }
    if (mShowRenderStats != 0) {
        Rnd::ThePs.DrawRenderStatsOverlay();
    }
}

void MetRenderer::DrawSimple() {
    if (mRunning == 0) {
        return;
    }
    mTopView->Rnd::Drawable::Draw();
    if (mShowTimingGraph != 0) {
        Rnd::ThePs.DrawSubsystemTimingGraph(kTimingGraphFullScaleMs);
    }
    if (mShowRenderStats != 0) {
        Rnd::ThePs.DrawRenderStatsOverlay();
    }
}

void MetRenderer::DispatchPriv(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == RawControllerMsg::sID) {
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
        mRecording = static_cast<IsRecordingMsg *>(pMsg)->mIsRecording;
    } else if (nType == g_nMetFreqEndedMsgType) {
        OnFreqEnded(pMsg);
    } else {
        ForwardToPanel(pMsg);
    }
}

void MetRenderer::OnStartPause([[maybe_unused]] Message *pMsg) {
    Start();
    if (MetFrontEndState::shared()->mPendingTransition == kPlainPausePhase) {
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

void MetRenderer::OnFreqEnded(Message *pMsg) {
    MetFreqEndedMsg *pEnded = static_cast<MetFreqEndedMsg *>(pMsg);
    const GameParams params(*Application::shared()->GetGameManager()->GetParams());
    SetDoWinSequence(0);

    if (params.mJukeboxMode == 1 && pEnded->mStopJukebox == 0) {
        const Color black{0.0f, 0.0f, 0.0f, kOpaque};
        Rnd::ThePs.SetClearColor(black);
        Start();
        AddScreen(MetScreen::FindScreenByName(HxStr("MetRemixManager")));
        MetRemixManager::shared()->PlayCurrentTrack();
        return;
    }

    const Color blue{0.0f, 0.0f, kClearBlue, kOpaque};
    Rnd::ThePs.SetClearColor(blue);
    mDiscProblemPending = 1;
    mPendingPanel = nullptr;

    if (mRecording != 0 || g_nReturnToLogo != 0) {
        mRecording = 0;
        g_nReturnToLogo = 0;
        OnReturnFromGame();
        mPendingPanel = MetScreen::FindScreenByName(HxStr("MetLogoScreen"));
#ifdef VIDEO_STANDARD_PAL
    } else if (params.mLevelName == TutorialLevelName(kTutorialLevelGame) ||
               params.mLevelName == TutorialLevelName(kTutorialLevelRemix)) {
#else
    } else if (params.mLevelName == kTutorialLevel || params.mLevelName == kTutorialRemixLevel) {
#endif
        OnReturnFromGame();
        if (GlobalSettings::shared()->mTutorialComplete == 0) {
            GlobalSettings::shared()->mTutorialComplete = 1;
            if (MetFrontEndState::shared()->mUsingMemcard != 0) {
                MetFrontEndState::shared()->mSettingsDirty = 1;
            }
        }
        mPendingPanel = MetScreen::FindScreenByName(HxStr("MetMainScreen"));
    } else if (params.mPlayMode == kPlayModeJam) {
        GameStats *pStats = Application::shared()->GetGameManager()->GetStats();
        if (params.mJukeboxMode != 0) {
            GameParams cleared(*Application::shared()->GetGameManager()->GetParams());
            cleared.mLoadingGame = 0;
            Application::shared()->GetGameManager()->SetParams(cleared);
            CallScriptTemplate(kJukeboxTemplate, kJukeboxStopArgument);
            OnReturnFromGame();
            OnReturnToMenus();
            MetRemixManager::shared()->LeaveJukeboxMode();
            mPendingPanel = MetScreen::FindScreenByName(HxStr("MetJukeboxTopButtonsScreen"));
#ifdef VIDEO_STANDARD_PAL
        } else if (pStats->mRemixEdited != 0) {
#else
        } else if (MetFrontEndState::shared()->mUsingMemcard != 0 && pStats->mRemixEdited != 0) {
#endif
            mPendingPanel = SelectEndScreen();
        } else {
            OnReturnFromGame();
            OnReturnToMenus();
            mPendingPanel = MetScreen::FindScreenByName(HxStr("MetRemixTypeScreen"));
            GameParams cleared(*Application::shared()->GetGameManager()->GetParams());
            cleared.mLoadingGame = 0;
            Application::shared()->GetGameManager()->SetParams(cleared);
            CallScriptTemplate(kJukeboxTemplate, kJukeboxStopArgument);
        }
    } else if (pEnded->mStopJukebox != 0) {
        OnReturnFromGame();
        if (params.mNetGame != 1) {
            OnReturnToMenus();
            mPendingPanel = MetScreen::FindScreenByName(HxStr("MetSoloStagesScreen"));
            GameParams cleared(*Application::shared()->GetGameManager()->GetParams());
            cleared.mLoadingGame = 0;
            Application::shared()->GetGameManager()->SetParams(cleared);
            CallScriptTemplate(kJukeboxTemplate, kJukeboxStopArgument);
        }
    } else {
        mPendingPanel = SelectEndScreen();
    }

    mFading = 1;
    Start();
    mFade->FadeOut(kFreqEndedFadeFrames, mAnimationFrame, this, 0);
    ResolveArenaView(0);
}

void MetRenderer::ResolveSceneViews() {
    // Yes, the binary polls the three boot loaders again and discards every result.
    float flProgress;
    gTopViewLoader->Poll(&flProgress);
    gFontLoader->Poll(&flProgress);
    gButtonLoader->Poll(&flProgress);
    mTopView = dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(HxStr(kTopView)));
    mBackgroundScene = dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(HxStr(kBackgroundView)));
    mScreenScene = dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(HxStr(kScreensView)));
    MetScreen::CreateStartupScreens(this);
    mFade = new MetFade(this);
}

void MetRenderer::Update() {
    if (mBootLoadPending != 0) {
        float flProgress;
        const int bMetagame = gTopViewLoader->Poll(&flProgress);
        const int bFonts = gFontLoader->Poll(&flProgress);
        const int bSharedTex = gButtonLoader->Poll(&flProgress);
        if (bMetagame != 0 && bFonts != 0 && bSharedTex != 0) {
            mBootLoadPending = 0;
            ResolveSceneViews();
            Start();
            CreateArenaLoader();
            ActivatePanel(MetScreen::FindScreenByName(HxStr(kStartupScreen)));
            const Color black{0.0f, 0.0f, 0.0f, kOpaque};
            Rnd::ThePs.SetClearColor(black);
        }
    }

    if (mDiscProblemPending != 0) {
        if (IsBankXferBusy() != 0) {
            AsyncPumpCompletedRequests();
        } else {
            Application::shared()->GetSynth()->AllNotesOff();
            mDiscProblemPending = 0;
            PlaySoundByName(kFrontEndMusic);
            const Color blue{0.0f, 0.0f, kClearBlue, kOpaque};
            Rnd::ThePs.SetClearColor(blue);
            if (mFading == 0) {
                ActivatePanel(mPendingPanel);
            }
        }
    }

    if (mBootLoadPending == 0) {
        if (IsMediaReady() == 0) {
            Rnd::Drawable *pProblem =
                dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(HxStr(kDiscProblemView)));
            pProblem->SetShowing(1);
            return;
        }
        Rnd::Drawable *pProblem =
            dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(HxStr(kDiscProblemView)));
        pProblem->SetShowing(0);
    }

    if (mRunning == 0) {
        return;
    }
    MetScreen::PollContainerLoads();

    const long long nNowNs = FrameClockNs(Application::shared()->GetWatchdog());
    const int nIntervalMs = FrameIntervalMs(nNowNs, mPreviousFrameNs);
    mPreviousFrameNs = nNowNs;
    mAnimationFrame += mFrameRate * static_cast<float>(nIntervalMs) / kMillisecondsPerSecondFloat;

    if (mFading != 0) {
        mFade->Update(mAnimationFrame);
    } else {
        for (std::vector<MetScreen *>::iterator it = mScreens.begin(); it != mScreens.end(); ++it) {
            (*it)->UpdateFrame(mAnimationFrame);
            if (mRunning == 0) {
                return;
            }
            // A screen that pushed or popped another one invalidated the iterator.
            if (mScreensChanged != 0) {
                mScreensChanged = 0;
                break;
            }
        }
        if (mRunning == 0) {
            return;
        }
        if (mPanelActive != 0) {
            mCommandRepeater->Update(mActivePanel, &nNowNs);
        }
    }
    mTopView->SetFrame(mAnimationFrame);
    mTopView->UpdateWorldXfm(nullptr, 0); // Yes, the binary discards the result.
}

void MetRenderer::RemoveScreen(MetScreen *pScreen) {
    for (std::vector<MetScreen *>::iterator it = mScreens.begin(); it != mScreens.end(); ++it) {
        if (*it == pScreen) {
            Rnd::View *pView = pScreen->mView;
            mScreenScene->RemoveTrans(pView);
            mScreenScene->RemoveDraw(pView);
            mScreenScene->RemoveAnim(pView);
            mScreens.erase(it);
            mScreensChanged = 1;
            return;
        }
    }
}
