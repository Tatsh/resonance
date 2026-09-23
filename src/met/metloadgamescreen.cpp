#include "met/metloadgamescreen.h"

#include <vector>

#include "app/application.h"
#include "app/renderer.h"
#include "app/watchdog.h"
#include "game/campaignstats.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "gfx/gfxdevice.h"
#include "math/color.h"
#include "met/metfreqmakerassetmanager.h"
#include "met/metfrontendstate.h"
#include "met/metpersonadata.h"
#include "met/metrenderer.h"
#include "msg/begingamelocalmsg.h"
#include "msg/gamemanagerdoplaybackmsg.h"
#include "msg/message.h"
#include "os/cycles.h"
#include "os/hxstr.h"
#include "os/r250.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "script/configquery.h"
#include "synth/ps2hardsynth.h"

namespace {

// The screen name. It is empty in the image, which makes the two animation views resolve as
// `_EE.anim` and `_BF.anim`.
static const char *const kScreenName = "";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Transition";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "loadgame";

// The two texts the screen fills.
static const char *const kEventText = "event.txt";
static const char *const kLoadingText = "loading.txt";

// The captions, read under kCaptionConfigCode.
constexpr int kCaptionConfigCode = 0x258;
static const char *const kLoadingCaption = "load_loading";
static const char *const kDemoCaption = "load_demo";
static const char *const kTutorialCaption = "load_tut";
static const char *const kRemixCaption = "load_remix";
static const char *const kGameCaption = "load_game";
static const char *const kJukeboxCaption = "load_jukebox";
static const char *const kNoCaption = "";

// Return screens that change the captions.
static const char *const kTutorialScreen = "MetTutorialScreen";
static const char *const kJukeboxDoneScreen = "MetJukeboxEditPlaylistScreenDone";
static const char *const kNoScreen = "";

// GameParams::mUnknown1c values.
constexpr int kPlayModeGameValue = 1;
constexpr int kPlayModeJamValue = 2;

// MetFrontEndState::mUnknown18 values the level loads record.
constexpr int kGamePhase = 1;
constexpr int kTutorialPhase = 5;

// How long the music takes to fade, in milliseconds.
constexpr int kMusicFadeMs = 2000;
// How long slot 26 waits after the fade starts, in nanoseconds.
constexpr long long kLoadDelayNs = 2100000000;
constexpr long long kNanosecondsPerMillisecond = 1000000;

// The length of the fade in.
constexpr float kFadeInDuration = 360.0f;
constexpr int kFadeDropsView = 0;

// The black the display clears to once the game starts.
constexpr float kOpaque = 1.0f;

// The watchdog time in nanoseconds.
inline long long WatchdogNowNs() {
    Watchdog *pWatchdog = Application::shared()->GetWatchdog();
    return (GetElapsedMilliseconds() - pWatchdog->mClock.mOriginMs) * kNanosecondsPerMillisecond;
}

// A caption read from the configuration.
inline HxStr Caption(const char *pszKey) {
    HxStr caption;
    QueryConfigString(&caption, kCaptionConfigCode, pszKey);
    return caption;
}

} // namespace

// 0x0028d2c8
MetLoadGameScreen::MetLoadGameScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mPolling(0), mWaiting(0), mUnknown9c(0), mDeadlineNs(0), mFade(nullptr) {
    mFade = new MetFade(pRenderer);
}

// 0x0028d4c0
void MetLoadGameScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mpEvent = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(kEventText)));
}

// 0x0028d590
void MetLoadGameScreen::EnterAndShow() {
    const GameParams params(*Application::shared()->GetGameManager()->GetParams());
    Rnd::Text *pLoading = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(kLoadingText)));

    if (params.mJukeboxMode == 0) {
        pLoading->SetText(Caption(kLoadingCaption));
        if (mUnknown9c != 0) {
            mpEvent->SetText(Caption(kDemoCaption));
        } else if (MetFrontEndState::shared()->mUnknown24 == kTutorialScreen) {
            mpEvent->SetText(Caption(kTutorialCaption));
        } else if (params.mUnknown1c == kPlayModeJamValue) {
            mpEvent->SetText(Caption(kRemixCaption));
        } else {
            mpEvent->SetText(Caption(kGameCaption));
        }

        MetPersonaData *pPersona = MetFrontEndState::shared()->GetFirstPersona();
        int nDoWinSequence = 0;
        if (params.mUnknown1c == kPlayModeGameValue &&
            Application::shared()->GetGameMode() == kGameModeSolo &&
            MetFrontEndState::shared()->mUnknown24 != kTutorialScreen &&
            pPersona->mStats.IsLastLevelRemaining(params) != 0) {
            nDoWinSequence = 1;
        }
        SetDoWinSequence(nDoWinSequence);
        AssignBurnSlots();
    } else if (MetFrontEndState::shared()->mUnknown24 == kJukeboxDoneScreen) {
        MetFrontEndState::shared()->mUnknown24 = HxStr(kNoScreen);
        pLoading->SetText(Caption(kLoadingCaption));
        mpEvent->SetText(Caption(kJukeboxCaption));
    } else {
        pLoading->SetText(HxStr(kNoCaption));
        mpEvent->SetText(HxStr(kNoCaption));
    }

    MetScreen::EnterAndShow();
}

// 0x0028dc60
void MetLoadGameScreen::OnUnknownSlot33() {
    Application::shared()->GetSynth()->FadeOut(kMusicFadeMs);
    mWaiting = 1;
    mDeadlineNs = WatchdogNowNs() + kLoadDelayNs;
}

// 0x0028dd38
void MetLoadGameScreen::OnUnknownSlot26(float flTime) {
    if (mWaiting != 0 && mDeadlineNs < WatchdogNowNs()) {
        mWaiting = 0;
        Renderer::LoadCommon();
        if (mUnknown9c != 0) {
            // Yes, the binary returns here without advancing the fade.
            mFade->FadeIn(kFadeInDuration, flTime, this, kFadeDropsView);
            return;
        }

        mPolling = 1;
        if (Application::shared()->GetGameManager()->GetParams()->mUnknown28 != 0) {
            LoadNetLevel();
        } else if (MetFrontEndState::shared()->mUnknown24 == kTutorialScreen) {
            LoadTutorialLevel();
        } else {
            LoadGameLevel();
        }
    }

    if (mPolling != 0) {
        float flCommonProgress;
        float flLevelProgress;
        const int nCommonDone = Renderer::PollCommon(&flCommonProgress);
        const int nLevelDone = Renderer::PollLevel(&flLevelProgress);
        if (nCommonDone != 0 && nLevelDone != 0) {
            mPolling = 0;
            mFade->FadeIn(kFadeInDuration, flTime, this, kFadeDropsView);
        }
    }
    mFade->Update(flTime);
}

// 0x0028df18
void MetLoadGameScreen::LoadNetLevel() {
    const GameParams params(*Application::shared()->GetGameManager()->GetParams());
    Renderer::LoadLevel(params);
}

// 0x0028e018
void MetLoadGameScreen::OnFadeInDone() {
    mUnknown10->ClearBackgroundScene();
    if (mUnknown9c != 0) {
        GameManagerDoPlaybackMsg msg;
        Application::shared()->GetGameManager()->QueueMessage(&msg);
        mUnknown9c = 0;
    } else {
        BeginGameLocalMsg msg;
        Application::shared()->GetGameManager()->QueueMessage(&msg);
    }
    mUnknown10->RemoveScreen(this);
    MetFrontEndState::shared()->mUnknown10 = 0;
    const Color black{0.0f, 0.0f, 0.0f, kOpaque};
    g_gfxDevice.SetClearColor(black);
    g_gfxDevice.FlipFrameBuffer();
}

// 0x0028e158
void MetLoadGameScreen::AssignBurnSlots() {
    if (mUnknown9c == 0) {
        std::vector<MetPersonaData *> personas(
            *Application::shared()->GetGameManager()->GetPersonas());
        for (int i = 0; i < static_cast<int>(personas.size()); ++i) {
            personas[i]->AttachToBurnSlot(i);
        }
    } else {
        std::vector<MetPersonaData *> identities(
            *MetFreqMakerAssetManager::shared()->GetIdentityList());
        MetPersonaData *pPersona = identities[RandomInt(0, identities.size())];
        pPersona->AttachToBurnSlot(0);
        Application::shared()->GetGameManager()->ClearPersonas();
        Application::shared()->GetGameManager()->AddPersona(*pPersona);
    }
}

// 0x002919c8
MetScreen *MetLoadGameScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetLoadGameScreen(pRenderer, nPriority);
}

// 0x00291a50
MetLoadGameScreen::~MetLoadGameScreen() {
    delete mFade;
}

// 0x00291ad0
void MetLoadGameScreen::LoadGameLevel() {
    MetFrontEndState *pState = MetFrontEndState::shared();
    pState->mUnknown1c = pState->mUnknown18;
    pState->mUnknown18 = kGamePhase;
    Renderer::LoadLevel(*Application::shared()->GetGameManager()->GetParams());
}

// 0x00291b28
void MetLoadGameScreen::LoadTutorialLevel() {
    MetFrontEndState *pState = MetFrontEndState::shared();
    pState->mUnknown1c = pState->mUnknown18;
    pState->mUnknown18 = kTutorialPhase;
    Renderer::LoadLevel(*Application::shared()->GetGameManager()->GetParams());
}

// 0x00291b80
void MetLoadGameScreen::OnFadeOutDone() {
}
