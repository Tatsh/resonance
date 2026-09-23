#include "met/mettutorialscreen.h"

#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "os/hxstr.h"
#include "script/configquery.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "tut";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "tutorial";

// The two buttons ResolveContainerViews() appends, in ring order.
static const char *const kFirstButtonObject = "tut_01.but";
static const char *const kSecondButtonObject = "tut_02.but";

// The two prompts, matching the button order above. Each one is both the key the button's label
// is looked up under and the prompt the help screen displays for that button.
static const char *const kFirstPrompt = "tut_g";
static const char *const kSecondPrompt = "tut_r";

// Screens the class exits by registry key.
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kHelpScreen = "MetHelpScreen";

// The empty literal that clears the panel and the prompt.
static const char *const kNoName = "";

// Configuration codes the button labels and the screen title are read under.
constexpr int kPromptConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x269;

// The key the screen title is looked up under.
static const char *const kTitleKey = "tutorial";

// Index EnterAndShow() selects, which is the first button.
constexpr int kFirstButtonIndex = 0;

// What MetScreen::mUnknown18 records for the exit hook to act on. The back command writes the
// first and the alternation-finished hook writes the second.
constexpr int kExitBack = 0;
constexpr int kExitToButtonAction = 2;

// The selection alternation the select command starts.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

} // namespace

MetTutorialScreen::MetTutorialScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknown8c(nullptr) {
    mUnknown8c = new MetButtonList();
    mUnknown38.push_back(HxStr(kFirstPrompt));
    mUnknown38.push_back(HxStr(kSecondPrompt));
}

MetTutorialScreen::~MetTutorialScreen() {
    delete mUnknown8c;
}

void MetTutorialScreen::EnterAndShow() {
    if (MetFrontEndState::shared()->mUnknown18 != 0) {
        MetFrontEndState::shared()->mUnknown24 = HxStr(kNoName);
        MetFrontEndState *pState = MetFrontEndState::shared();
        pState->mUnknown1c = pState->mUnknown18;
        pState->mUnknown18 = 0;
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kHelpScreen));
        mUnknown10->SetActivePanel(this);
    }
    mUnknown8c->SetSelected(kFirstButtonIndex);

    {
        HxStr title;
        QueryConfigString(&title, kTitleConfigCode, kTitleKey);
        MetScreenTitleScreen::SetTitle(title);
    }

    MetHelpScreen::SetText(mUnknown38[mUnknown8c->mSelected], mUnknown10->mUnknown68);
    MetScreen::EnterAndShow();
}

void MetTutorialScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();

    {
        HxStr objectName(kFirstButtonObject);
        HxStr label;
        QueryConfigString(&label, kPromptConfigCode, kFirstPrompt);
        mUnknown8c->Add(objectName, label);
    }
    {
        HxStr objectName(kSecondButtonObject);
        HxStr label;
        QueryConfigString(&label, kPromptConfigCode, kSecondPrompt);
        mUnknown8c->Add(objectName, label);
    }
}

void MetTutorialScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mUnknown8c->OnUnknownSlot2();
        MetHelpScreen::SetText(mUnknown38[mUnknown8c->mSelected], mUnknown10->mUnknown68);
        break;

    case kMetScreenCommandNext:
        mUnknown8c->OnUnknownSlot3();
        MetHelpScreen::SetText(mUnknown38[mUnknown8c->mSelected], mUnknown10->mUnknown68);
        break;

    case kMetScreenCommandSelect:
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        ActivateNamedPanel(HxStr(kNoName));
        StartRepeatingSound(mUnknown10->mUnknown68,
                            kSelectAlternateInterval,
                            mUnknown8c->mUnknown00,
                            kSelectAlternateCycles);
        break;

    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        ActivateNamedPanel(HxStr(kNoName));
        mUnknown18 = kExitBack;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kLeftGizmoScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

void MetTutorialScreen::PlayCycleLeftSound(int) {
}

void MetTutorialScreen::PlayCycleRightSound(int) {
}

void MetTutorialScreen::OnUnknownSlot30(Rnd::Object *) {
    mUnknown18 = kExitToButtonAction;
    ExitScreenByName(HxStr(kLeftGizmoScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    ExitScreenByName(HxStr(kHelpScreen));
    BeginExit();
}
