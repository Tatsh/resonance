#include "met/metlogoscreen.h"

#include <vector>

#include "app/application.h"
#include "app/playsound.h"
#include "app/watchdog.h"
#include "game/gamemanagerimpl.h"
#include "game/inputpoller.h"
#include "met/metfrontendstate.h"
#include "met/metloadgamescreen.h"
#include "met/metrenderer.h"
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
    Watchdog *pWatchdog = Application::shared()->GetWatchdog();
    return (GetElapsedMilliseconds() - pWatchdog->mClock.mOriginMs) * kNanosecondsPerMillisecond;
}

} // namespace

// 0x002ba4a0
MetLogoScreen::MetLogoScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mBlinkTime(0), mAttractEnabled(0), mLastActivityNs(kNoActivity) {
    mAttractDelaySeconds = QueryConfigValue(kAttractDelayConfigCode);
    mAttractStarted = 0;
    mUnknown60 = 0;
}

// 0x002be418
MetLogoScreen::~MetLogoScreen() {
}

// 0x002be390
MetLogoScreen *MetLogoScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetLogoScreen(pRenderer, nPriority);
}

// 0x002be670
void MetLogoScreen::RecordUnlock() {
    PlayActivateSound();
    MetFrontEndState::shared()->mUnknown14 = 1;
}

void MetLogoScreen::UpdateBlink(float flTime) {
    if (mBlinkTime != 0 && mBlinkTime + kBlinkFrames < flTime) {
        mStartText->SetShowing(mStartText->GetShowing() ? 0 : 1);
        mBlinkTime = flTime + kBlinkFrames;
    }
    mWaveView->SetFrame(flTime);
}

// 0x002ba6d0
void MetLogoScreen::ResolveContainerViews() {
    ResolveAnimationViews();
    mUnknown14 = FindObject<Rnd::View>(HxStr(kLogoView));
    mUnknown14->ReleaseAnimsRefs();
    mUnknown48 = 0;
    mStartText = FindObject<Rnd::Text>(HxStr(kStartText));
    mWaveView = FindObject<Rnd::View>(HxStr(kWaveView));

    Rnd::Text *pVersion = FindObject<Rnd::Text>(HxStr(kVersionText));
    if (pVersion != nullptr) {
        const HxStr label(kVersionLabel);
        pVersion->SetText(label + GetVersionString());
    }

    for (int i = 1; i <= kLegalTextCount; ++i) {
        Rnd::Text *pLegal = FindObject<Rnd::Text>(HxStr(FormatString(kLegalTextFormat, i)));
        pLegal->SetShowing(0);
        mLegalTexts.push_back(pLegal);
    }
    SetShowing(0);
}

// 0x002bac40
void MetLogoScreen::OnUnknownSlot26(float flTime) {
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
}

// 0x002bae40
void MetLogoScreen::OnUnknownSlot33() {
    mLastActivityNs = WatchdogNowNs();
    mAttractEnabled = QueryConfigFlag(kAttractEnabledConfigCode);
    mBlinkTime = kBlinkStart;
    mUnknown10->mUnknown60 = 1;
    PlaySoundByName(kFrequencySound);
}

// 0x002baf20
void MetLogoScreen::OnUnknownSlot36() {
    if (mAttractStarted) {
        mAttractStarted = 0;
        static_cast<MetLoadGameScreen *>(FindScreenByName(HxStr(kLoadGameScreen)))->mUnknown9c = 1;
        PushNamedScreen(HxStr(kLoadGameScreen));
    } else {
        MetFrontEndState::shared()->mUnknown24 = HxStr(kOwnScreenName);
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kLeftGizmoSmallScreen));
        PushNamedScreen(HxStr(kTopLogoScreen));
        PushNamedScreen(HxStr(kMainScreen));
        ActivateNamedPanel(HxStr(kMainScreen));
    }
    for (int i = 0; i < kLegalTextCount; ++i) {
        mLegalTexts[i]->SetShowing(0);
    }
}

// 0x002be4d8
void MetLogoScreen::HandleCommand(const MetScreenCommand *pCommand) {
    if (pCommand->mCommand == kMetScreenCommandSelect || pCommand->mCommand == kCommandStart) {
        PlaySoundByName(kSlideSound);
        mUnknown10->mUnknown60 = 0;
        mBlinkTime = 0;
        BeginExit();
    }
}

// 0x002be540
void MetLogoScreen::EnterAndShow() {
    MetScreen::EnterAndShow();
    for (int i = 0; i < kLegalTextCount; ++i) {
        mLegalTexts[i]->SetShowing(1);
    }
}

// 0x002be5a8
void MetLogoScreen::UpdateIdleAnimation(float flTime) {
    UpdateBlink(flTime);
}

// 0x002be6a0
void MetLogoScreen::HandleMessage(Message *pMsg) {
    if (pMsg->Type() == g_nMetUnlockStagesMsgType) {
        RecordUnlock();
    }
}
