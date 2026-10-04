#include "met/metmemdetectstartup.h"

#include <vector>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "met/metmsgscreen.h"
#include "met/metrenderer.h"
#include "met/metstrings.h"
#include "msg/message.h"
#include "os/hxstr.h"
#include "rnd/view.h"

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

} // namespace

MetMemDetectStartup::MetMemDetectStartup(MetRenderer *pRenderer, int nPriority)
    : MetMemDetectScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mFade(nullptr), mNoCardTime(0) {
    mFade = new MetFade(pRenderer);
}

MetMemDetectStartup::~MetMemDetectStartup() {
    delete mFade;
}

MetMemDetectStartup *MetMemDetectStartup::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMemDetectStartup(pRenderer, nPriority);
}

void MetMemDetectStartup::UpdateIdle(float flTime) {
    mFade->Update(flTime);
    if (mEnterTime != 0 && mEnterTime + kFadeFrames < flTime) {
        mEnterTime = 0;
        mFade->FadeOut(kFadeFrames, flTime, this, kReleaseView);
        mView->SetShowing(1);
    }
    if (mNoCardTime != 0 && mNoCardTime + kFadeFrames < flTime) {
        mNoCardTime = 0;
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        MetMsgScreen::ShowActive(
            HxStr(kNoCardMessage),
            MetText(kMetStrMsgWARNING, kWarningTitle),
            MetConfigText(kMetStrMemCheck, kDialogueConfigCode, kNoCardMessage),
            kTwoButtons,
            buttons,
            this);
    }
    MetMemDetectScreen::UpdateIdle(flTime);
}

void MetMemDetectStartup::StartDetect() {
    std::vector<HxStr> buttons;
    MetMsgScreen::Show(HxStr(kDetectMessage),
                       MetText(kMetStrMsgWARNING, kWarningTitle),
                       MetConfigText(kMetStrMemDetect, kDialogueConfigCode, kDetectKey),
                       kNoButtons,
                       buttons,
                       this);
    MetMemDetectScreen::StartDetect();
}

void MetMemDetectStartup::EnterAndShow() {
    mRenderer->AddScreenView(mView);
    Application::shared()->GetGameManager()->SetDrawEnabled(1);
    SetShowing(0);
    mEnterTime = mRenderer->mAnimationFrame;
}

void MetMemDetectStartup::BeginExit() {
    PushNamedScreen(HxStr(kSonyScreen));
}

void MetMemDetectStartup::OnNoCard() {
    mNoCardTime = mRenderer->mAnimationFrame;
}

void MetMemDetectStartup::OnDetectFinished() {
    mFade->FadeIn(kFadeFrames, mRenderer->mAnimationFrame, this, kReleaseView);
}

void MetMemDetectStartup::OnFadeInDone() {
    SetShowing(0);
    BeginExit();
}

void MetMemDetectStartup::OnFadeOutDone() {
    StartDetect();
}
