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

// NTSC-U/C: 0x002df058, PAL: 0x00301db8
MetMemDetectStartup::MetMemDetectStartup(MetRenderer *pRenderer, int nPriority)
    : MetMemDetectScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mFade(nullptr), mNoCardTime(0) {
    mFade = new MetFade(pRenderer);
}

// NTSC-U/C: 0x002e2eb8, PAL: 0x00305e40
MetMemDetectStartup::~MetMemDetectStartup() {
    delete mFade;
}

// NTSC-U/C: 0x002e2e30, PAL: 0x00305db8
MetMemDetectStartup *MetMemDetectStartup::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMemDetectStartup(pRenderer, nPriority);
}

// NTSC-U/C: 0x002df250, PAL: 0x00302018
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

// NTSC-U/C: 0x002df6e0, PAL: 0x00302530
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

// NTSC-U/C: 0x002e2f40, PAL: 0x00305ec8
void MetMemDetectStartup::EnterAndShow() {
    mRenderer->AddScreenView(mView);
    Application::shared()->GetGameManager()->SetDrawEnabled(1);
    SetShowing(0);
    mEnterTime = mRenderer->mAnimationFrame;
}

// NTSC-U/C: 0x002e2fb8, PAL: 0x00305f40
void MetMemDetectStartup::BeginExit() {
    PushNamedScreen(HxStr(kSonyScreen));
}

// NTSC-U/C: 0x002e3058, PAL: 0x00306000
void MetMemDetectStartup::OnNoCard() {
    mNoCardTime = mRenderer->mAnimationFrame;
}

// NTSC-U/C: 0x002e3068, PAL: 0x00306010
void MetMemDetectStartup::OnDetectFinished() {
    mFade->FadeIn(kFadeFrames, mRenderer->mAnimationFrame, this, kReleaseView);
}

// NTSC-U/C: 0x002e30a0, PAL: 0x00306048
void MetMemDetectStartup::OnFadeInDone() {
    SetShowing(0);
    BeginExit();
}

// NTSC-U/C: 0x002e30f0, PAL: 0x00306098
void MetMemDetectStartup::OnFadeOutDone() {
    StartDetect();
}
