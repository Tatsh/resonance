#include "met/metgameskillscreen.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "met/metbuttonlist.h"
#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "os/hxstr.h"
#include "script/configquery.h"

namespace {

static const char *const kScreenName = "smgs";
static const char *const kDirectory = "metagame/_Solo";
static const char *const kContainerName = "gameskill";

// The three buttons and the prompts their labels are read under.
static const char *const kEasyButton = "smgs_01.but";
static const char *const kNormalButton = "smgs_02.but";
static const char *const kExpertButton = "smgs_03.but";
static const char *const kEasyPrompt = "ms_easy";
static const char *const kNormalPrompt = "ms_normal";
static const char *const kExpertPrompt = "ms_expert";

// The keys of the title's mode prefix and of its body.
static const char *const kSoloTitleKey = "solo";
static const char *const kMultiTitleKey = "multi";
static const char *const kTitleKey = "skill";

// The help texts of the three buttons, for the solo mode and for every other mode.
static const char *const kSoloEasyHelp = "smgs_easy";
static const char *const kSoloNormalHelp = "smgs_normal";
static const char *const kSoloExpertHelp = "smgs_expert";
static const char *const kMultiEasyHelp = "mgs_easy";
static const char *const kMultiNormalHelp = "mgs_normal";
static const char *const kMultiExpertHelp = "mgs_expert";

static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kModeScreen = "MetModeScreen";
static const char *const kSoloStagesScreen = "MetSoloStagesScreen";
static const char *const kNoName = "";

// Configuration codes the button labels and the screen title are read under.
constexpr int kPromptConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x269;

// The values MetScreen::mUnknown18 takes on the way out.
constexpr int kExitBack = 0;
constexpr int kExitToButtonAction = 2;

// The alternation the select command starts on the chosen button.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

} // namespace

// 0x00273140
MetGameSkillScreen::MetGameSkillScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mButtonList(nullptr) {
    mButtonList = new MetButtonList();
}

// 0x00276ad0
MetGameSkillScreen::~MetGameSkillScreen() {
    delete mButtonList;
}

// 0x00276a48
MetGameSkillScreen *MetGameSkillScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetGameSkillScreen(pRenderer, nPriority);
}

// 0x00273800
void MetGameSkillScreen::EnterAndShow() {
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    mButtonList->SetSelected(params.mDifficulty);
    HxStr mode;
    mUnknown38.clear();
    if (Application::shared()->GetGameManager()->GetGameMode() == kGameModeSolo) {
        {
            HxStr key;
            QueryConfigString(&key, kTitleConfigCode, kSoloTitleKey);
            mode = key;
        }
        mUnknown38.push_back(HxStr(kSoloEasyHelp));
        mUnknown38.push_back(HxStr(kSoloNormalHelp));
        mUnknown38.push_back(HxStr(kSoloExpertHelp));
    } else {
        {
            HxStr key;
            QueryConfigString(&key, kTitleConfigCode, kMultiTitleKey);
            mode = key;
        }
        mUnknown38.push_back(HxStr(kMultiEasyHelp));
        mUnknown38.push_back(HxStr(kMultiNormalHelp));
        mUnknown38.push_back(HxStr(kMultiExpertHelp));
    }
    {
        HxStr body;
        QueryConfigString(&body, kTitleConfigCode, kTitleKey);
        MetScreenTitleScreen::SetTitle(mode + body);
    }
    MetHelpScreen::SetText(mUnknown38[mButtonList->mSelected], mUnknown10->mUnknown68);
    MetScreen::EnterAndShow();
}

// 0x00273558
void MetGameSkillScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mButtonList->OnUnknownSlot2();
        MetHelpScreen::SetText(mUnknown38[mButtonList->mSelected], mUnknown10->mUnknown68);
        break;

    case kMetScreenCommandNext:
        mButtonList->OnUnknownSlot3();
        MetHelpScreen::SetText(mUnknown38[mButtonList->mSelected], mUnknown10->mUnknown68);
        break;

    case kMetScreenCommandSelect:
        ActivateNamedPanel(HxStr(kNoName));
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        StartRepeatingSound(mUnknown10->mUnknown68,
                            kSelectAlternateInterval,
                            mButtonList->mUnknown00,
                            kSelectAlternateCycles);
        break;

    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        mUnknown18 = kExitBack;
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// 0x00276a38
void MetGameSkillScreen::PlayCycleLeftSound(int) {
}

// 0x00276a40
void MetGameSkillScreen::PlayCycleRightSound(int) {
}

// 0x00273f28
void MetGameSkillScreen::OnUnknownSlot30(Rnd::Button *) {
    mUnknown18 = kExitToButtonAction;
    ExitScreenByName(HxStr(kLeftGizmoScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    BeginExit();
}

// 0x00274058
void MetGameSkillScreen::OnUnknownSlot36() {
    if (mUnknown18 == kExitBack) {
        PushNamedScreen(HxStr(kModeScreen));
        ActivateNamedPanel(HxStr(kModeScreen));
        return;
    }
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    params.mDifficulty = mButtonList->mSelected;
    Application::shared()->GetGameManager()->SetParams(params);
    PushNamedScreen(HxStr(kSoloStagesScreen));
    ActivateNamedPanel(HxStr(kSoloStagesScreen));
}

// 0x00273310
void MetGameSkillScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    {
        HxStr objectName(kEasyButton);
        HxStr label;
        QueryConfigString(&label, kPromptConfigCode, kEasyPrompt);
        mButtonList->Add(objectName, label);
    }
    {
        HxStr objectName(kNormalButton);
        HxStr label;
        QueryConfigString(&label, kPromptConfigCode, kNormalPrompt);
        mButtonList->Add(objectName, label);
    }
    {
        HxStr objectName(kExpertButton);
        HxStr label;
        QueryConfigString(&label, kPromptConfigCode, kExpertPrompt);
        mButtonList->Add(objectName, label);
    }
}
