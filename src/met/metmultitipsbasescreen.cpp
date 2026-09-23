#include "met/metmultitipsbasescreen.h"

#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "script/configquery.h"

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

inline HxStr ConfigText(int nCode, const char *pszKey) {
    HxStr text = QueryConfigString(nCode, pszKey);
    return text;
}

} // namespace

// 0x00306cd8
MetMultiTipsBaseScreen::MetMultiTipsBaseScreen(MetRenderer *pRenderer,
                                               int nPriority,
                                               const HxStr &name,
                                               const HxStr &file,
                                               int nPage,
                                               const HxStr &previous,
                                               const HxStr &next)
    : MetScreen(pRenderer, nPriority, name, HxStr(kDirectory), file), mUnknown8c(previous),
      mUnknown94(next), mUnknown9c(nPage) {
    mUnknown38.push_back(HxStr(kHelpPrompt));
}

// 0x00306ee0
void MetMultiTipsBaseScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandSelect:
        ActivateNamedPanel(HxStr(""));
        mUnknown18 = kExitNext;
        BeginExit();
        break;

    case kMetScreenCommandBack:
        ActivateNamedPanel(HxStr(""));
        mUnknown18 = kExitPrevious;
        BeginExit();
        break;

    case kCommandQuit:
        MetScreen::PlayLeaveSound(pCommand->mPadIndex);
        ActivateNamedPanel(HxStr(""));
        mUnknown18 = kExitQuit;
        BeginExit();
        break;

    default:
        break;
    }
}

// 0x003070f0
void MetMultiTipsBaseScreen::EnterAndShow() {
    HxStr title = ConfigText(kTitleConfigCode, kTitleKey) + FormatString(kPageFormat, mUnknown9c);
    MetScreenTitleScreen::SetTitle(title);
    MetScreen::EnterAndShow();
    MetHelpScreen::SelectPreset(HxStr(kHelpLayout));
}

// 0x003072a0
void MetMultiTipsBaseScreen::OnUnknownSlot36() {
    if (mUnknown18 == kExitPrevious) {
        PushNamedScreen(mUnknown8c);
        ActivateNamedPanel(mUnknown8c);
    } else if (mUnknown18 == kExitNext) {
        PushNamedScreen(mUnknown94);
        ActivateNamedPanel(mUnknown94);
    } else {
        ReturnToPlayerCount();
    }
}

// 0x0030d710
MetMultiTipsBaseScreen::~MetMultiTipsBaseScreen() {
}

// 0x0030d790
void MetMultiTipsBaseScreen::PlayCycleLeftSound(int) {
}

// 0x0030d798
void MetMultiTipsBaseScreen::PlayCycleRightSound(int) {
}

// 0x0030d7a0
void MetMultiTipsBaseScreen::PlayHighSound(int) {
}

// 0x0030d7a8
void MetMultiTipsBaseScreen::OnUnknownSlot33() {
    MetHelpScreen::SetText(mUnknown38[0], mUnknown10->mUnknown68);
}

// 0x0030d7d0
void MetMultiTipsBaseScreen::BeginExit() {
    ExitScreenByName(HxStr(kTitleScreen));
    MetScreen::BeginExit();
}
