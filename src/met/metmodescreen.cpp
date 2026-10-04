#include "met/metmodescreen.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "met/metstrings.h"
#include "os/hxstr.h"
#include "script/configquery.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "smm";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "sm_mode";

// The two buttons ResolveContainerViews() appends, and the keys their labels are read under.
static const char *const kGameButtonObject = "smm_01.but";
static const char *const kJamButtonObject = "smm_02.but";
static const char *const kGameLabelKey = "ms_game";
static const char *const kJamLabelKey = "ms_jam";

// The help prompts EnterAndShow() records for a solo and for a multiplayer session.
static const char *const kSoloGamePrompt = "sms_game";
static const char *const kSoloJamPrompt = "sms_jam";
static const char *const kMultiGamePrompt = "mms_game";
static const char *const kMultiJamPrompt = "mms_jam";

// The title EnterAndShow() builds, a session prefix followed by the mode word.
static const char *const kSoloTitleKey = "solo";
static const char *const kMultiTitleKey = "multi";
static const char *const kModeTitleKey = "mode";
static const char *const kPromptLayout = "standard_title";

// Screens the class exits or goes on to by registry key.
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kCharacterPickScreen = "MetLocPickCharScreen";
static const char *const kLoadFreqScreen = "MetLoadFreqScreen";
static const char *const kLoadPrefabScreen = "MetLoadPreFabScreen";
static const char *const kSkillScreen = "MetGameSkillScreen";
static const char *const kRemixTypeScreen = "MetRemixTypeScreen";

// This screen's own registry key, which the character picker returns to.
static const char *const kOwnScreenName = "MetModeScreen";

// The empty literal the select and back paths pass to clear the panel and the prompt.
static const char *const kNoName = "";

// Configuration codes the button labels and the title are read under.
constexpr int kLabelConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x269;

// The button indices, in ring order.
constexpr int kGameButtonIndex = 0;
constexpr int kJamButtonIndex = 1;

// The values mExitChoice records for an exit through a select and through a back.
constexpr int kExitBySelect = 2;
constexpr int kExitByBack = 0;

// The selection alternation the select path starts.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

} // namespace

MetModeScreen::MetModeScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mButtonList(new MetButtonList()) {
}

void MetModeScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();

    const HxStr gameButton(kGameButtonObject);
    HxStr gameLabel = MetConfigText(kMetStrMsGame, kLabelConfigCode, kGameLabelKey);
    mButtonList->Add(gameButton, gameLabel);

    const HxStr jamButton(kJamButtonObject);
    HxStr jamLabel = MetConfigText(kMetStrMsJam, kLabelConfigCode, kJamLabelKey);
    mButtonList->Add(jamButton, jamLabel);
}

void MetModeScreen::HandleCommand(const MetScreenCommand *pCommand) {
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
        ActivateNamedPanel(HxStr(kNoName));
        mExitChoice = kExitByBack;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kLeftGizmoScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

void MetModeScreen::EnterAndShow() {
    HxStr prefix;
    const GameParams params(*Application::shared()->GetGameManager()->GetParams());
    mButtonList->SetSelected(params.mPlayMode == kPlayModeJam ? kJamButtonIndex : kGameButtonIndex);

    mHelpKeys.clear();
    if (Application::shared()->GetGameManager()->GetGameMode() == kGameModeSolo) {
        HxStr solo = MetConfigText(kMetStrTSolo, kTitleConfigCode, kSoloTitleKey);
        prefix = solo;
        mHelpKeys.push_back(MetText(kMetStrHSmsGame, kSoloGamePrompt));
        mHelpKeys.push_back(MetText(kMetStrHSmsJam, kSoloJamPrompt));
    } else {
        HxStr multi = MetConfigText(kMetStrTMulti, kTitleConfigCode, kMultiTitleKey);
        prefix = multi;
        mHelpKeys.push_back(MetText(kMetStrHMmsGame, kMultiGamePrompt));
        mHelpKeys.push_back(MetText(kMetStrHMmsJam, kMultiJamPrompt));
    }
    MetHelpScreen::SetText(mHelpKeys[mButtonList->mSelected], mRenderer->mAnimationFrame);
    MetHelpScreen::SelectPreset(MetText(kMetStrHStandardTitle, kPromptLayout));

    HxStr mode = MetConfigText(kMetStrTMode, kTitleConfigCode, kModeTitleKey);
    MetScreenTitleScreen::SetTitle(prefix + mode);

    MetScreen::EnterAndShow();
}

void MetModeScreen::OnExitFinished() {
    if (mExitChoice != kExitByBack) {
        GoToSelectedMode();
        return;
    }

    const int nGameMode = Application::shared()->GetGameManager()->GetGameMode();
    if (nGameMode == kGameModeLocal) {
        MetFrontEndState::shared()->mReturnScreen = HxStr(kOwnScreenName);
        PushNamedScreen(HxStr(kCharacterPickScreen));
        ActivateNamedPanel(HxStr(kCharacterPickScreen));
    } else if (Application::shared()->GetGameManager()->GetGameMode() == kGameModeSolo) {
        if (MetFrontEndState::shared()->mUsingMemcard != 0) {
            PushNamedScreen(HxStr(kLoadFreqScreen));
            ActivateNamedPanel(HxStr(kLoadFreqScreen));
        } else {
            PushNamedScreen(HxStr(kLoadPrefabScreen));
            ActivateNamedPanel(HxStr(kLoadPrefabScreen));
        }
    }
}

void MetModeScreen::GoToSelectedMode() {
    switch (mButtonList->mSelected) {
    case kGameButtonIndex:
        Application::shared()->GetGameManager()->SetPlayMode(kPlayModeGame);
        PushNamedScreen(HxStr(kSkillScreen));
        ActivateNamedPanel(HxStr(kSkillScreen));
        break;

    case kJamButtonIndex:
        Application::shared()->GetGameManager()->SetPlayMode(kPlayModeJam);
        PushNamedScreen(HxStr(kRemixTypeScreen));
        ActivateNamedPanel(HxStr(kRemixTypeScreen));
        break;

    default:
        break;
    }
}

void MetModeScreen::PlayCycleLeftSound(int) {
}

void MetModeScreen::PlayCycleRightSound(int) {
}

MetModeScreen *MetModeScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetModeScreen(pRenderer, nPriority);
}

MetModeScreen::~MetModeScreen() {
    delete mButtonList;
}

void MetModeScreen::OnEnterFinished() {
    MetHelpScreen::SetText(mHelpKeys[mButtonList->mSelected], mRenderer->mAnimationFrame);
}

void MetModeScreen::OnRepeatingSoundFinished(Rnd::Button *) {
    mExitChoice = kExitBySelect;
    ExitScreenByName(HxStr(kTitleScreen));
    BeginExit();
}
