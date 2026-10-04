#include "met/metmultitipsbasescreen.h"

#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "met/metstrings.h"
#include "os/formatstring.h"
#include "os/hxstr.h"

namespace {

static const char *const kDirectory = "metagame/_Local";
static const char *const kHelpPrompt = "multi_tip_help";
static const char *const kTitleKey = "multi_tips";
static const char *const kPageFormat = "%d";
static const char *const kHelpLayout = "multi_tips_tab";
static const char *const kTitleScreen = "MetScreenTitleScreen";

constexpr int kTitleConfigCode = 0x269;

// A command code above MetScreenCommandCode's range, which the tip pages treat as a quit.
constexpr int kCommandQuit = 8;

} // namespace

MetMultiTipsBaseScreen::MetMultiTipsBaseScreen(MetRenderer *pRenderer,
                                               int nPriority,
                                               const HxStr &name,
                                               const HxStr &file,
                                               int nPage,
                                               const HxStr &previous,
                                               const HxStr &next)
    : MetScreen(pRenderer, nPriority, name, HxStr(kDirectory), file), mPreviousScreen(previous),
      mNextScreen(next), mPage(nPage) {
    mHelpKeys.push_back(MetText(kMetStrHMultiTipHelp, kHelpPrompt));
}

void MetMultiTipsBaseScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandSelect:
        ActivateNamedPanel(HxStr(""));
        mExitChoice = kExitNext;
        BeginExit();
        break;

    case kMetScreenCommandBack:
        ActivateNamedPanel(HxStr(""));
        mExitChoice = kExitPrevious;
        BeginExit();
        break;

    case kCommandQuit:
        MetScreen::PlayLeaveSound(pCommand->mPadIndex);
        ActivateNamedPanel(HxStr(""));
        mExitChoice = kExitQuit;
        BeginExit();
        break;

    default:
        break;
    }
}

void MetMultiTipsBaseScreen::EnterAndShow() {
    HxStr title = MetConfigText(kMetStrTMultiTips, kTitleConfigCode, kTitleKey) +
                  Rnd::MakeString(kPageFormat, mPage);
    MetScreenTitleScreen::SetTitle(title);
    MetScreen::EnterAndShow();
    MetHelpScreen::SelectPreset(MetText(kMetStrHMultiTipsTab, kHelpLayout));
}

void MetMultiTipsBaseScreen::OnExitFinished() {
    if (mExitChoice == kExitPrevious) {
        PushNamedScreen(mPreviousScreen);
        ActivateNamedPanel(mPreviousScreen);
    } else if (mExitChoice == kExitNext) {
        PushNamedScreen(mNextScreen);
        ActivateNamedPanel(mNextScreen);
    } else {
        ReturnToPlayerCount();
    }
}

MetMultiTipsBaseScreen::~MetMultiTipsBaseScreen() {
}

void MetMultiTipsBaseScreen::PlayCycleLeftSound(int) {
}

void MetMultiTipsBaseScreen::PlayCycleRightSound(int) {
}

void MetMultiTipsBaseScreen::PlayHighSound(int) {
}

void MetMultiTipsBaseScreen::OnEnterFinished() {
    MetHelpScreen::SetText(mHelpKeys[0], mRenderer->mAnimationFrame);
}

void MetMultiTipsBaseScreen::BeginExit() {
    ExitScreenByName(HxStr(kTitleScreen));
    MetScreen::BeginExit();
}
