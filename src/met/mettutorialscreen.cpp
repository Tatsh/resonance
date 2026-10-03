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
#include "met/metstrings.h"
#include "os/hxstr.h"
#include "os/r250.h"

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

// Screens OnExitFinished() pushes.
static const char *const kLeftGizmoSmallScreen = "MetLeftGizmoSmallScreen";
static const char *const kTopLogoScreen = "MetTopLogoScreen";
static const char *const kMainScreen = "MetMainScreen";
static const char *const kLoadGameScreen = "MetLoadGameScreen";
// The registry key OnExitFinished() records as the screen to return to.
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

// What MetScreen::mExitChoice records for the exit hook to act on. The back command writes the
// first and the alternation-finished hook writes the second.
constexpr int kExitBack = 0;
constexpr int kExitToButtonAction = 2;

// The selection alternation the select command starts.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

} // namespace

// NTSC-U/C: 0x003c7a88, PAL: 0x003fed10
MetTutorialScreen::MetTutorialScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mButtonList(nullptr) {
    mButtonList = new MetButtonList();
    mHelpKeys.push_back(MetText(kMetStrHTutG, kFirstPrompt));
    mHelpKeys.push_back(MetText(kMetStrHTutR, kSecondPrompt));
}

// NTSC-U/C: 0x003cc1a8, PAL: 0x00403a40
MetTutorialScreen *MetTutorialScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetTutorialScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x003cc230, PAL: 0x00403ac8
MetTutorialScreen::~MetTutorialScreen() {
    delete mButtonList;
}

// NTSC-U/C: 0x003c82a0, PAL: 0x003ff6e8
void MetTutorialScreen::EnterAndShow() {
    if (MetFrontEndState::shared()->mPendingTransition != 0) {
        MetFrontEndState::shared()->mReturnScreen = HxStr(kNoName);
        MetFrontEndState *pState = MetFrontEndState::shared();
        pState->mLastTransition = pState->mPendingTransition;
        pState->mPendingTransition = 0;
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kHelpScreen));
        mRenderer->SetActivePanel(this);
    }
    mButtonList->SetSelected(kFirstButtonIndex);

    {
        HxStr title = MetConfigText(kMetStrTTutorial, kTitleConfigCode, kTitleKey);
        MetScreenTitleScreen::SetTitle(title);
    }

    MetHelpScreen::SetText(mHelpKeys[mButtonList->mSelected], mRenderer->mAnimationFrame);
    MetScreen::EnterAndShow();
}

// NTSC-U/C: 0x003c7d78, PAL: 0x003ff098
void MetTutorialScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();

    {
        HxStr objectName(kFirstButtonObject);
        HxStr label = MetConfigText(kMetStrTutG, kPromptConfigCode, kFirstPrompt);
        mButtonList->Add(objectName, label);
    }
    {
        HxStr objectName(kSecondButtonObject);
        HxStr label = MetConfigText(kMetStrTutR, kPromptConfigCode, kSecondPrompt);
        mButtonList->Add(objectName, label);
    }
}

// NTSC-U/C: 0x003c7f10, PAL: 0x003ff2a0
void MetTutorialScreen::HandleCommand(const MetScreenCommand *pCommand) {
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
        MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
        ActivateNamedPanel(HxStr(kNoName));
        StartRepeatingSound(mRenderer->mAnimationFrame,
                            kSelectAlternateInterval,
                            mButtonList->mSelectedButton,
                            kSelectAlternateCycles);
        break;

    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
        ActivateNamedPanel(HxStr(kNoName));
        mExitChoice = kExitBack;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kLeftGizmoScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// NTSC-U/C: 0x003cc198, PAL: 0x00403a30
void MetTutorialScreen::PlayCycleLeftSound(int) {
}

// NTSC-U/C: 0x003cc1a0, PAL: 0x00403a38
void MetTutorialScreen::PlayCycleRightSound(int) {
}

// NTSC-U/C: 0x003c84d8, PAL: 0x003ff998
void MetTutorialScreen::OnRepeatingSoundFinished(Rnd::Button *) {
    mExitChoice = kExitToButtonAction;
    ExitScreenByName(HxStr(kLeftGizmoScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    ExitScreenByName(HxStr(kHelpScreen));
    BeginExit();
}

// NTSC-U/C: 0x003c8678, PAL: 0x003ffb98
void MetTutorialScreen::OnExitFinished() {
    if (mExitChoice == kExitBack) {
        PushNamedScreen(HxStr(kLeftGizmoSmallScreen));
        PushNamedScreen(HxStr(kTopLogoScreen));
        PushNamedScreen(HxStr(kMainScreen));
        ActivateNamedPanel(HxStr(kMainScreen));
        return;
    }

    Application::shared()->GetGameManager()->SetGameMode(kGameModeSolo);
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
#ifdef VIDEO_STANDARD_PAL
    HxStr suffix;
    suffix = LocalizedAssetSuffix();
    HxStr level;
#endif
    if (mButtonList->mSelected == kFirstButtonIndex) {
        params.mPlayMode = kPlayModeGame;
#ifdef VIDEO_STANDARD_PAL
        level = kFirstButtonLevel;
#else
        params.mLevelName = kFirstButtonLevel;
#endif
    } else {
        params.mPlayMode = kPlayModeJam;
#ifdef VIDEO_STANDARD_PAL
        level = kSecondButtonLevel;
#else
        params.mLevelName = kSecondButtonLevel;
#endif
    }
#ifdef VIDEO_STANDARD_PAL
    level += suffix;
    params.mLevelName = level;
#endif
    params.mArenaName = (*GetArenaList())[0].mName;
    params.mDifficulty = kTutorialDifficulty;
    Application::shared()->GetGameManager()->SetParams(params);

    std::vector<MetPersonaData *> identities(
        *MetFreqMakerAssetManager::shared()->GetIdentityList());
    MetPersonaData *pIdentity = identities[RandomInt(0, identities.size())];
    pIdentity->AttachToBurnSlot(kTutorialBurnSlot);
    Application::shared()->GetGameManager()->ClearPersonas();
    Application::shared()->GetGameManager()->AddPersona(*pIdentity);

    MetFrontEndState::shared()->mReturnScreen = HxStr(kTutorialScreen);
    PushNamedScreen(HxStr(kLoadGameScreen));
    ActivateNamedPanel(HxStr(kLoadGameScreen));
}
