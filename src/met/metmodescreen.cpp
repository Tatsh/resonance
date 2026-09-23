#include "met/metmodescreen.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
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

// The values mUnknown18 records for an exit through a select and through a back.
constexpr int kExitBySelect = 2;
constexpr int kExitByBack = 0;

// The selection alternation the select path starts.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

} // namespace

// 0x002e72c0
MetModeScreen::MetModeScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknown8c(new MetButtonList()) {
}

// 0x002e7490
void MetModeScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();

    const HxStr gameButton(kGameButtonObject);
    HxStr gameLabel = QueryConfigString(kLabelConfigCode, kGameLabelKey);
    mUnknown8c->Add(gameButton, gameLabel);

    const HxStr jamButton(kJamButtonObject);
    HxStr jamLabel = QueryConfigString(kLabelConfigCode, kJamLabelKey);
    mUnknown8c->Add(jamButton, jamLabel);
}

// 0x002e7628
void MetModeScreen::HandleCommand(const MetScreenCommand *pCommand) {
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
        ActivateNamedPanel(HxStr(kNoName));
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        StartRepeatingSound(mUnknown10->mUnknown68,
                            kSelectAlternateInterval,
                            mUnknown8c->mUnknown00,
                            kSelectAlternateCycles);
        break;

    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        ActivateNamedPanel(HxStr(kNoName));
        mUnknown18 = kExitByBack;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kLeftGizmoScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// 0x002e79b8
void MetModeScreen::EnterAndShow() {
    HxStr prefix;
    const GameParams params(*Application::shared()->GetGameManager()->GetParams());
    mUnknown8c->SetSelected(params.mUnknown1c == kPlayModeJam ? kJamButtonIndex : kGameButtonIndex);

    mUnknown38.clear();
    if (Application::shared()->GetGameManager()->GetGameMode() == kGameModeSolo) {
        HxStr solo = QueryConfigString(kTitleConfigCode, kSoloTitleKey);
        prefix = solo;
        mUnknown38.push_back(HxStr(kSoloGamePrompt));
        mUnknown38.push_back(HxStr(kSoloJamPrompt));
    } else {
        HxStr multi = QueryConfigString(kTitleConfigCode, kMultiTitleKey);
        prefix = multi;
        mUnknown38.push_back(HxStr(kMultiGamePrompt));
        mUnknown38.push_back(HxStr(kMultiJamPrompt));
    }
    MetHelpScreen::SetText(mUnknown38[mUnknown8c->mSelected], mUnknown10->mUnknown68);
    MetHelpScreen::SelectPreset(HxStr(kPromptLayout));

    HxStr mode = QueryConfigString(kTitleConfigCode, kModeTitleKey);
    MetScreenTitleScreen::SetTitle(prefix + mode);

    MetScreen::EnterAndShow();
}

// 0x002e8008
void MetModeScreen::OnUnknownSlot36() {
    if (mUnknown18 != kExitByBack) {
        GoToSelectedMode();
        return;
    }

    const int nGameMode = Application::shared()->GetGameManager()->GetGameMode();
    if (nGameMode == kGameModeLocal) {
        MetFrontEndState::shared()->mUnknown24 = HxStr(kOwnScreenName);
        PushNamedScreen(HxStr(kCharacterPickScreen));
        ActivateNamedPanel(HxStr(kCharacterPickScreen));
    } else if (Application::shared()->GetGameManager()->GetGameMode() == kGameModeSolo) {
        if (MetFrontEndState::shared()->mUnknown0c != 0) {
            PushNamedScreen(HxStr(kLoadFreqScreen));
            ActivateNamedPanel(HxStr(kLoadFreqScreen));
        } else {
            PushNamedScreen(HxStr(kLoadPrefabScreen));
            ActivateNamedPanel(HxStr(kLoadPrefabScreen));
        }
    }
}

// 0x002e8398
void MetModeScreen::GoToSelectedMode() {
    switch (mUnknown8c->mSelected) {
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

// 0x002eb808
void MetModeScreen::PlayCycleLeftSound(int) {
}

// 0x002eb810
void MetModeScreen::PlayCycleRightSound(int) {
}

// 0x002eb818
MetModeScreen *MetModeScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetModeScreen(pRenderer, nPriority);
}

// 0x002eb8a0
MetModeScreen::~MetModeScreen() {
    delete mUnknown8c;
}

// 0x002eb920
void MetModeScreen::OnUnknownSlot33() {
    MetHelpScreen::SetText(mUnknown38[mUnknown8c->mSelected], mUnknown10->mUnknown68);
}

// 0x002eb958
void MetModeScreen::OnUnknownSlot30(Rnd::Button *) {
    mUnknown18 = kExitBySelect;
    ExitScreenByName(HxStr(kTitleScreen));
    BeginExit();
}
