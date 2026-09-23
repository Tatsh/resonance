#include "met/mettutorialscreen.h"

#include <vector>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "met/metbuttonlist.h"
#include "met/metfreqmakerassetmanager.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metpersonadata.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "met/metsonglists.h"
#include "os/hxstr.h"
#include "os/r250.h"
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

// Screens OnUnknownSlot36() pushes.
static const char *const kLeftGizmoSmallScreen = "MetLeftGizmoSmallScreen";
static const char *const kTopLogoScreen = "MetTopLogoScreen";
static const char *const kMainScreen = "MetMainScreen";
static const char *const kLoadGameScreen = "MetLoadGameScreen";
// The registry key OnUnknownSlot36() records as the screen to return to.
static const char *const kTutorialScreen = "MetTutorialScreen";

// The level each button starts, matching the button order.
static const char *const kFirstButtonLevel = "tutorial";
static const char *const kSecondButtonLevel = "tutorialrmx";

// The difficulty and the burn slot the tutorial game uses.
constexpr int kTutorialDifficulty = 0;
constexpr int kTutorialBurnSlot = 0;

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

MetTutorialScreen *MetTutorialScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetTutorialScreen(pRenderer, nPriority);
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

void MetTutorialScreen::OnUnknownSlot30(Rnd::Button *) {
    mUnknown18 = kExitToButtonAction;
    ExitScreenByName(HxStr(kLeftGizmoScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    ExitScreenByName(HxStr(kHelpScreen));
    BeginExit();
}

void MetTutorialScreen::OnUnknownSlot36() {
    if (mUnknown18 == kExitBack) {
        PushNamedScreen(HxStr(kLeftGizmoSmallScreen));
        PushNamedScreen(HxStr(kTopLogoScreen));
        PushNamedScreen(HxStr(kMainScreen));
        ActivateNamedPanel(HxStr(kMainScreen));
        return;
    }

    Application::shared()->GetGameManager()->SetGameMode(kGameModeSolo);
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    if (mUnknown8c->mSelected == kFirstButtonIndex) {
        params.mUnknown1c = kPlayModeGame;
        params.mLevelName = kFirstButtonLevel;
    } else {
        params.mUnknown1c = kPlayModeJam;
        params.mLevelName = kSecondButtonLevel;
    }
    params.mArenaName = (*GetArenaList())[0].mName;
    params.mDifficulty = kTutorialDifficulty;
    Application::shared()->GetGameManager()->SetParams(params);

    std::vector<MetPersonaData *> identities(
        *MetFreqMakerAssetManager::shared()->GetIdentityList());
    MetPersonaData *pIdentity = identities[RandomInt(0, identities.size())];
    pIdentity->AttachToBurnSlot(kTutorialBurnSlot);
    Application::shared()->GetGameManager()->ClearPersonas();
    Application::shared()->GetGameManager()->AddPersona(*pIdentity);

    MetFrontEndState::shared()->mUnknown24 = HxStr(kTutorialScreen);
    PushNamedScreen(HxStr(kLoadGameScreen));
    ActivateNamedPanel(HxStr(kLoadGameScreen));
}
