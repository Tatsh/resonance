#include "met/metmemdetectstartup.h"

#include <vector>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "met/metmsgscreen.h"
#include "met/metrenderer.h"
#include "msg/message.h"
#include "os/hxstr.h"
#include "rnd/view.h"
#include "script/configquery.h"

namespace {

// The screen name. It is empty in the image, and the two animation views resolve as `_EE.anim`
// and `_BF.anim`.
static const char *const kScreenName = "";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "memdetect1";

// Dialogue names, keys, titles, and button labels.
static const char *const kDetectMessage = "mem_load";
static const char *const kDetectKey = "mem_detect";
static const char *const kNoCardMessage = "mem_check";
static const char *const kWarningTitle = "WARNING";
static const char *const kRetryButton = "RETRY";
static const char *const kContinueButton = "CONTINUE";

static const char *const kSonyScreen = "MetSonyScreen";

// The configuration code every dialogue text is read under.
constexpr int kDialogueConfigCode = 0x258;

// Button counts MetMsgScreen receives with each dialogue.
constexpr int kNoButtons = 0;
constexpr int kTwoButtons = 2;

// The length of each fade, and the delay before the fade out and before `mem_check`, in frames.
constexpr float kFadeFrames = 360.0f;

// The view MetFade releases when the fade ends.
constexpr int kReleaseView = 0;

// A configuration value read by value.
inline HxStr ConfigText(const char *pszKey) {
    HxStr value = QueryConfigString(kDialogueConfigCode, pszKey);
    return value;
}

} // namespace

// 0x002df058
MetMemDetectStartup::MetMemDetectStartup(MetRenderer *pRenderer, int nPriority)
    : MetMemDetectScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mFade(nullptr), mNoCardTime(0) {
    mFade = new MetFade(pRenderer);
}

// 0x002e2eb8
MetMemDetectStartup::~MetMemDetectStartup() {
    delete mFade;
}

// 0x002e2e30
MetMemDetectStartup *MetMemDetectStartup::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMemDetectStartup(pRenderer, nPriority);
}

// 0x002df250
void MetMemDetectStartup::OnUnknownSlot26(float flTime) {
    mFade->Update(flTime);
    if (mEnterTime != 0 && mEnterTime + kFadeFrames < flTime) {
        mEnterTime = 0;
        mFade->FadeOut(kFadeFrames, flTime, this, kReleaseView);
        mUnknown14->SetShowing(1);
    }
    if (mNoCardTime != 0 && mNoCardTime + kFadeFrames < flTime) {
        mNoCardTime = 0;
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kRetryButton));
        buttons.push_back(HxStr(kContinueButton));
        MetMsgScreen::ShowActive(HxStr(kNoCardMessage),
                                 HxStr(kWarningTitle),
                                 ConfigText(kNoCardMessage),
                                 kTwoButtons,
                                 buttons,
                                 this);
    }
    MetMemDetectScreen::OnUnknownSlot26(flTime);
}

// 0x002df6e0
void MetMemDetectStartup::StartDetect() {
    std::vector<HxStr> buttons;
    MetMsgScreen::Show(HxStr(kDetectMessage),
                       HxStr(kWarningTitle),
                       ConfigText(kDetectKey),
                       kNoButtons,
                       buttons,
                       this);
    MetMemDetectScreen::StartDetect();
}

// 0x002e2f40
void MetMemDetectStartup::EnterAndShow() {
    mUnknown10->AddScreenView(mUnknown14);
    Application::shared()->GetGameManager()->SetDrawEnabled(1);
    SetShowing(0);
    mEnterTime = mUnknown10->mUnknown68;
}

// 0x002e2fb8
void MetMemDetectStartup::BeginExit() {
    PushNamedScreen(HxStr(kSonyScreen));
}

// 0x002e3058
void MetMemDetectStartup::OnNoCard() {
    mNoCardTime = mUnknown10->mUnknown68;
}

// 0x002e3068
void MetMemDetectStartup::OnDetectFinished() {
    mFade->FadeIn(kFadeFrames, mUnknown10->mUnknown68, this, kReleaseView);
}

// 0x002e30a0
void MetMemDetectStartup::OnFadeInDone() {
    SetShowing(0);
    BeginExit();
}

// 0x002e30f0
void MetMemDetectStartup::OnFadeOutDone() {
    StartDetect();
}
