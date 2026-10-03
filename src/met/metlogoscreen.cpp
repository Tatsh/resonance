#include "met/metlogoscreen.h"

#include <vector>

#include "app/application.h"
#include "app/playsound.h"
#include "app/scheduler.h"
#include "game/gamemanagerimpl.h"
#include "game/inputpoller.h"
#include "met/metfrontendstate.h"
#include "met/metloadgamescreen.h"
#include "met/metrenderer.h"
#include "met/metstrings.h"
#include "msg/message.h"
#include "msg/metunlockstagesmsg.h"
#include "os/cycles.h"
#include "os/formatstring.h"
#include "os/hostmode.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "rnd/view.h"
#include "script/configquery.h"

#ifdef ENABLE_PATCHES
#include "buildinfo.h"
#endif

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "fl";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "freq_logo_panel";

// Container objects.
static const char *const kLogoView = "freq_logo.view";
static const char *const kStartText = "start text.txt";
static const char *const kWaveView = "wave.view";
static const char *const kVersionText = "version.txt";
static const char *const kLegalTextFormat = "flp_legal%d.txt";

// The label the version text starts with.
static const char *const kVersionLabel = "Version:";

#ifdef ENABLE_PATCHES
// Heads the legal text of a patched build, so it is never mistaken for the retail disc.
static const char *const kBuildTag =
    "github.com/Tatsh/resonance " RESONANCE_GIT_REVISION " " RESONANCE_BUILD_TIME;
#endif

// Sounds.
static const char *const kSlideSound = "SND_MET_SLIDE";
static const char *const kFrequencySound = "SND_MET_FREQUENCY";

static const char *const kOwnScreenName = "MetLogoScreen";
static const char *const kLoadGameScreen = "MetLoadGameScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kLeftGizmoSmallScreen = "MetLeftGizmoSmallScreen";
static const char *const kTopLogoScreen = "MetTopLogoScreen";
static const char *const kMainScreen = "MetMainScreen";

// Configuration codes of the attract-mode switch and delay.
constexpr int kAttractEnabledConfigCode = 0x26b;
constexpr int kAttractDelayConfigCode = 0x26c;

// The number of legal texts.
constexpr int kLegalTextCount = 4;

// The command beyond MetScreenCommandCode that also starts the game.
constexpr int kCommandStart = 10;

#ifdef VIDEO_STANDARD_PAL
// The MetScreen::mExitChoice values OnExitFinished() acts on.
constexpr int kExitFadeIn = 0;
constexpr int kExitToMainMenu = 2;

// The fade OnExitFinished() runs. The fade view stays attached when the fade finishes.
constexpr float kFadeInFrames = 360.0f;
constexpr int kFadeRetainView = 1;
#endif

// The interval between two toggles of the start text, in frames.
constexpr float kBlinkFrames = 120.0f;
// The value slot 33 gives mBlinkTime to start the blink on the next frame.
constexpr float kBlinkStart = 1.0f;

constexpr long long kNanosecondsPerMillisecond = 1000000;

// mLastActivityNs before the first measurement.
constexpr long long kNoActivity = -1;

// Resolve one named object of the renderer as T.
template <class T>
inline T *FindObject(const HxStr &name) {
    return dynamic_cast<T *>(Rnd::g_manager.Find(name));
}

// The watchdog time in nanoseconds.
inline long long WatchdogNowNs() {
    Sch::Scheduler *pWatchdog = Application::shared()->GetWatchdog();
    return (GetElapsedMilliseconds() - pWatchdog->mClock.mOriginMs) * kNanosecondsPerMillisecond;
}

} // namespace

// NTSC-U/C: 0x002ba4a0, PAL: 0x002d9c10
MetLogoScreen::MetLogoScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mBlinkTime(0), mAttractEnabled(0), mLastActivityNs(kNoActivity) {
    mAttractDelaySeconds = QueryConfigValue(kAttractDelayConfigCode);
    mAttractStarted = 0;
    mShowsLoadedDrawables = 0;
#ifdef VIDEO_STANDARD_PAL
    mFade = new MetFade(pRenderer);
#endif
}

// NTSC-U/C: 0x002be418, PAL: 0x002da698
MetLogoScreen::~MetLogoScreen() {
#ifdef VIDEO_STANDARD_PAL
    delete mFade;
#endif
}

// NTSC-U/C: 0x002be390, PAL: 0x002de138
MetLogoScreen *MetLogoScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetLogoScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x002be670, PAL: 0x002de378
void MetLogoScreen::RecordUnlock() {
    PlayActivateSound();
    MetFrontEndState::shared()->mUnlockAll = 1;
}

void MetLogoScreen::UpdateBlink(float flTime) {
    if (mBlinkTime != 0 && mBlinkTime + kBlinkFrames < flTime) {
        mStartText->SetShowing(mStartText->GetShowing() ? 0 : 1);
        mBlinkTime = flTime + kBlinkFrames;
    }
    mWaveView->SetFrame(flTime);
}

// NTSC-U/C: 0x002ba6d0, PAL: 0x002d9f00
void MetLogoScreen::ResolveContainerViews() {
    ResolveAnimationViews();
    mView = FindObject<Rnd::View>(HxStr(kLogoView));
    mView->ReleaseAnimsRefs();
    mViewsUnresolved = 0;
    mStartText = FindObject<Rnd::Text>(HxStr(kStartText));
#ifdef VIDEO_STANDARD_PAL
    mStartText->SetText(GetMetString(kMetStrLogoStart));
#endif
    mWaveView = FindObject<Rnd::View>(HxStr(kWaveView));

    Rnd::Text *pVersion = FindObject<Rnd::Text>(HxStr(kVersionText));
    if (pVersion != nullptr) {
        const HxStr label(kVersionLabel);
        pVersion->SetText(label + GetVersionString());
    }

    for (int i = 1; i <= kLegalTextCount; ++i) {
        Rnd::Text *pLegal = FindObject<Rnd::Text>(HxStr(FormatString(kLegalTextFormat, i)));
        pLegal->SetShowing(0);
#ifdef VIDEO_STANDARD_PAL
        pLegal->SetText(GetMetString(kMetStrLegal1 + i - 1));
#endif
        mLegalTexts.push_back(pLegal);
    }
#ifdef ENABLE_PATCHES
    // The title screen never draws the version text, so the build tag heads the first legal text.
    Rnd::Text *pFirstLegal = mLegalTexts.front();
    pFirstLegal->SetText(HxStr(kBuildTag) + "\n" + pFirstLegal->mPreWrapText);
#endif
    SetShowing(0);
}

// NTSC-U/C: 0x002bac40, PAL: 0x002da778
void MetLogoScreen::UpdateIdle(float flTime) {
    const long long llNowNs = WatchdogNowNs();
    if (Application::shared()->GetGameManager()->GetPoller()->mPressedThisPoll) {
        mLastActivityNs = llNowNs;
    } else if (mAttractEnabled && !mAttractStarted) {
        const int nIdleMs =
            static_cast<int>((llNowNs - mLastActivityNs + kNanosecondsPerMillisecond / 2) /
                             kNanosecondsPerMillisecond);
        if (mAttractDelaySeconds * kMillisecondsPerSecond < nIdleMs) {
            mAttractStarted = 1;
            BeginExit();
        }
    }
    UpdateBlink(flTime);
#ifdef VIDEO_STANDARD_PAL
    mFade->Update(flTime);
#endif
}

// NTSC-U/C: 0x002bae40, PAL: 0x002da988
void MetLogoScreen::OnEnterFinished() {
    mLastActivityNs = WatchdogNowNs();
    mAttractEnabled = QueryConfigFlag(kAttractEnabledConfigCode);
    mBlinkTime = kBlinkStart;
    mRenderer->mTitlePromptShowing = 1;
    PlaySoundByName(kFrequencySound);
}

// NTSC-U/C: 0x002baf20, PAL: 0x002daa68
void MetLogoScreen::OnExitFinished() {
    if (mAttractStarted) {
        mAttractStarted = 0;
        static_cast<MetLoadGameScreen *>(FindScreenByName(HxStr(kLoadGameScreen)))->mDemoPlayback =
            1;
        PushNamedScreen(HxStr(kLoadGameScreen));
#ifdef VIDEO_STANDARD_PAL
    } else if (mExitChoice == kExitToMainMenu) {
#else
    } else {
#endif
        MetFrontEndState::shared()->mReturnScreen = HxStr(kOwnScreenName);
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kLeftGizmoSmallScreen));
        PushNamedScreen(HxStr(kTopLogoScreen));
        PushNamedScreen(HxStr(kMainScreen));
        ActivateNamedPanel(HxStr(kMainScreen));
#ifdef VIDEO_STANDARD_PAL
    } else if (mExitChoice == kExitFadeIn) {
        mFade->FadeIn(kFadeInFrames, mRenderer->mAnimationFrame, this, kFadeRetainView);
        mRenderer->AddScreen(this);
#endif
    }
    for (int i = 0; i < kLegalTextCount; ++i) {
        mLegalTexts[i]->SetShowing(0);
    }
}

// NTSC-U/C: 0x002be4d8, PAL: 0x002de1c0
void MetLogoScreen::HandleCommand(const MetScreenCommand *pCommand) {
    if (pCommand->mCommand == kMetScreenCommandSelect || pCommand->mCommand == kCommandStart) {
        PlaySoundByName(kSlideSound);
        mRenderer->mTitlePromptShowing = 0;
#ifdef VIDEO_STANDARD_PAL
        mExitChoice = kExitToMainMenu;
#endif
        mBlinkTime = 0;
        BeginExit();
    }
}

// NTSC-U/C: 0x002be540, PAL: 0x002de248
void MetLogoScreen::EnterAndShow() {
    MetScreen::EnterAndShow();
    for (int i = 0; i < kLegalTextCount; ++i) {
        mLegalTexts[i]->SetShowing(1);
    }
}

// NTSC-U/C: 0x002be5a8, PAL: 0x002de2b0
void MetLogoScreen::UpdateIdleAnimation(float flTime) {
    UpdateBlink(flTime);
}

// NTSC-U/C: 0x002be6a0, PAL: 0x002de3c8
void MetLogoScreen::HandleMessage(Message *pMsg) {
    if (pMsg->Type() == g_nMetUnlockStagesMsgType) {
        RecordUnlock();
    }
}

#ifdef VIDEO_STANDARD_PAL
// PAL: 0x002de3a8
void MetLogoScreen::OnFadeInDone() {
    mRenderer->RemoveScreen(this);
}
#endif
