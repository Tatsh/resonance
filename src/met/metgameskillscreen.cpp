#include "met/metgameskillscreen.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "met/metbuttonlist.h"
#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "met/metstrings.h"
#include "os/formatstring.h"
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

// The values MetScreen::mExitChoice takes on the way out.
constexpr int kExitBack = 0;
constexpr int kExitToButtonAction = 2;

// The alternation the select command starts on the chosen button.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

} // namespace

// NTSC-U/C: 0x00273140, PAL: 0x0028b620
MetGameSkillScreen::MetGameSkillScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mButtonList(nullptr) {
    mButtonList = new MetButtonList();
}

// NTSC-U/C: 0x00276ad0, PAL: 0x0028f410
MetGameSkillScreen::~MetGameSkillScreen() {
    delete mButtonList;
}

// NTSC-U/C: 0x00276a48, PAL: 0x0028f388
MetGameSkillScreen *MetGameSkillScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetGameSkillScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x00273800, PAL: 0x0028be60
void MetGameSkillScreen::EnterAndShow() {
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    mButtonList->SetSelected(params.mDifficulty);
    HxStr mode;
    mHelpKeys.clear();
    if (Application::shared()->GetGameManager()->GetGameMode() == kGameModeSolo) {
        {
            HxStr key = MetConfigText(kMetStrTSolo, kTitleConfigCode, kSoloTitleKey);
            mode = key;
        }
        mHelpKeys.push_back(MetText(kMetStrHSmgsEasy, kSoloEasyHelp));
        mHelpKeys.push_back(MetText(kMetStrHSmgsNormal, kSoloNormalHelp));
        mHelpKeys.push_back(MetText(kMetStrHSmgsExpert, kSoloExpertHelp));
    } else {
        {
            HxStr key = MetConfigText(kMetStrTMulti, kTitleConfigCode, kMultiTitleKey);
            mode = key;
        }
        mHelpKeys.push_back(MetText(kMetStrHMgsEasy, kMultiEasyHelp));
        mHelpKeys.push_back(MetText(kMetStrHMgsNormal, kMultiNormalHelp));
        mHelpKeys.push_back(MetText(kMetStrHMgsExpert, kMultiExpertHelp));
    }
    {
#ifdef VIDEO_STANDARD_PAL
        // The text is a format with the mode in place of `%s`.
        HxStr body = GetMetString(kMetStrTSkill);
        MetScreenTitleScreen::SetTitle(
            HxStr(FormatString(body.mStr != nullptr ? body.mStr : g_szEmptyString,
                               mode.mStr != nullptr ? mode.mStr : g_szEmptyString)));
#else
        HxStr body = QueryConfigString(kTitleConfigCode, kTitleKey);
        MetScreenTitleScreen::SetTitle(mode + body);
#endif
    }
    MetHelpScreen::SetText(mHelpKeys[mButtonList->mSelected], mRenderer->mAnimationFrame);
    MetScreen::EnterAndShow();
}

// NTSC-U/C: 0x00273558, PAL: 0x0028bb38
void MetGameSkillScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mButtonList->SelectPrevious();
        MetHelpScreen::SetText(mHelpKeys[mButtonList->mSelected], mRenderer->mAnimationFrame);
        break;

    case kMetScreenCommandNext:
        mButtonList->SelectNext();
        MetHelpScreen::SetText(mHelpKeys[mButtonList->mSelected], mRenderer->mAnimationFrame);
        break;

    case kMetScreenCommandSelect:
        ActivateNamedPanel(HxStr(kNoName));
        MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
        StartRepeatingSound(mRenderer->mAnimationFrame,
                            kSelectAlternateInterval,
                            mButtonList->mSelectedButton,
                            kSelectAlternateCycles);
        break;

    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
        mExitChoice = kExitBack;
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// NTSC-U/C: 0x00276a38, PAL: 0x0028f378
void MetGameSkillScreen::PlayCycleLeftSound(int) {
}

// NTSC-U/C: 0x00276a40, PAL: 0x0028f380
void MetGameSkillScreen::PlayCycleRightSound(int) {
}

// NTSC-U/C: 0x00273f28, PAL: 0x0028c670
void MetGameSkillScreen::OnRepeatingSoundFinished(Rnd::Button *) {
    mExitChoice = kExitToButtonAction;
    ExitScreenByName(HxStr(kLeftGizmoScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    BeginExit();
}

// NTSC-U/C: 0x00274058, PAL: 0x0028c7e8
void MetGameSkillScreen::OnExitFinished() {
    if (mExitChoice == kExitBack) {
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

// NTSC-U/C: 0x00273310, PAL: 0x0028b858
void MetGameSkillScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    {
        HxStr objectName(kEasyButton);
        HxStr label = MetConfigText(kMetStrMsEasy, kPromptConfigCode, kEasyPrompt);
        mButtonList->Add(objectName, label);
    }
    {
        HxStr objectName(kNormalButton);
        HxStr label = MetConfigText(kMetStrMsNormal, kPromptConfigCode, kNormalPrompt);
        mButtonList->Add(objectName, label);
    }
    {
        HxStr objectName(kExpertButton);
        HxStr label = MetConfigText(kMetStrMsExpert, kPromptConfigCode, kExpertPrompt);
        mButtonList->Add(objectName, label);
    }
}
