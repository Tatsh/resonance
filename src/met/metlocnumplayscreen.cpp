#include "met/metlocnumplayscreen.h"

#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "met/metstrings.h"
#include "os/hxstr.h"
#include "script/configquery.h"

namespace {

static const char *const kScreenName = "mnp";
static const char *const kDirectory = "metagame/_Local";
static const char *const kContainerName = "num_players";

// The prompt keys, one per button in ring order.
static const char *const kTwoPlayerKey = "loc_2p";
static const char *const kThreePlayerKey = "loc_3p";
static const char *const kFourPlayerKey = "loc_4p";
static const char *const kTipsKey = "multi_tips";

// The four buttons, in ring order.
static const char *const kTwoPlayerButton = "2player.but";
static const char *const kThreePlayerButton = "3player.but";
static const char *const kFourPlayerButton = "4player.but";
static const char *const kTipsButton = "mnp_info.but";

// Configuration codes of the button labels and the title.
constexpr int kLabelConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x269;
static const char *const kTitleKey = "m_num_p";

static const char *const kStandardTitlePreset = "standard_title";
static const char *const kNoName = "";

// Screens the class departs to and pushes.
static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kTopLogoScreen = "MetTopLogoScreen";
static const char *const kLeftGizmoSmallScreen = "MetLeftGizmoSmallScreen";
static const char *const kMainScreen = "MetMainScreen";
static const char *const kLocNumPlayScreen = "MetLocNumPlayScreen";
static const char *const kLocPickCharScreen = "MetLocPickCharScreen";
static const char *const kMultiTips1Screen = "MetMultiTips1Screen";

// The fewest players, which the first button offers.
constexpr int kFewestPlayers = 2;
// The buttons that choose a player count, ahead of the tips button.
constexpr int kPlayerCountButtons = 3;
// The controllers MetRenderer accepts while the screen shows.
constexpr int kMaxControllers = 4;

// MetScreen::mExitChoice values.
constexpr int kExitBack = 0;
constexpr int kExitChoice = 2;

// The alternation select plays.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

} // namespace

MetLocNumPlayScreen::MetLocNumPlayScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mButtonList(nullptr) {
    mButtonList = new MetButtonList;
    mHelpKeys.push_back(MetText(kMetStrHLoc2p, kTwoPlayerKey));
    mHelpKeys.push_back(MetText(kMetStrHLoc3p, kThreePlayerKey));
    mHelpKeys.push_back(MetText(kMetStrHLoc4p, kFourPlayerKey));
    mHelpKeys.push_back(MetText(kMetStrHMultiTips, kTipsKey));
}

void MetLocNumPlayScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();

    HxStr twoLabel = MetConfigText(kMetStrLoc2p, kLabelConfigCode, kTwoPlayerKey);
    mButtonList->Add(HxStr(kTwoPlayerButton), twoLabel);

    HxStr threeLabel = MetConfigText(kMetStrLoc3p, kLabelConfigCode, kThreePlayerKey);
    mButtonList->Add(HxStr(kThreePlayerButton), threeLabel);

    HxStr fourLabel = MetConfigText(kMetStrLoc4p, kLabelConfigCode, kFourPlayerKey);
    mButtonList->Add(HxStr(kFourPlayerButton), fourLabel);

    HxStr tipsLabel = MetConfigText(kMetStrMultiTips, kLabelConfigCode, kTipsKey);
    mButtonList->Add(HxStr(kTipsButton), tipsLabel);
}

void MetLocNumPlayScreen::HandleCommand(const MetScreenCommand *pCommand) {
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
        ExitScreenByName(HxStr(kLeftGizmoScreen));
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

void MetLocNumPlayScreen::EnterAndShow() {
    HxStr title = MetConfigText(kMetStrTMNumP, kTitleConfigCode, kTitleKey);
    MetScreenTitleScreen::SetTitle(title);

    mRenderer->mMaxPadIndex = kMaxControllers;
    int nIndex = MetFrontEndState::shared()->mPlayerCount - kFewestPlayers;
    if (nIndex < 0 || nIndex >= kPlayerCountButtons) {
        nIndex = 0;
    }
    mButtonList->SetSelected(nIndex);

    MetHelpScreen::SelectPreset(MetText(kMetStrHStandardTitle, kStandardTitlePreset));
    MetHelpScreen::SetText(mHelpKeys[mButtonList->mSelected], mRenderer->mAnimationFrame);
    MetScreen::EnterAndShow();
}

void MetLocNumPlayScreen::OnRepeatingSoundFinished(Rnd::Button *) {
    mExitChoice = kExitChoice;
    ExitScreenByName(HxStr(kLeftGizmoScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    ExitScreenByName(HxStr(kHelpScreen));
    BeginExit();
}

void MetLocNumPlayScreen::OnExitFinished() {
    if (mExitChoice == kExitBack) {
        PushNamedScreen(HxStr(kTopLogoScreen));
        PushNamedScreen(HxStr(kLeftGizmoSmallScreen));
        PushNamedScreen(HxStr(kMainScreen));
        ActivateNamedPanel(HxStr(kMainScreen));
        return;
    }

    const int nSelected = mButtonList->mSelected;
    if (nSelected < kPlayerCountButtons) {
        MetFrontEndState::shared()->mPlayerCount = nSelected + kFewestPlayers;
        mRenderer->mMaxPadIndex = nSelected + kFewestPlayers;
        MetFrontEndState::shared()->mReturnScreen = HxStr(kLocNumPlayScreen);
        PushNamedScreen(HxStr(kLocPickCharScreen));
        ActivateNamedPanel(HxStr(kLocPickCharScreen));
    } else {
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kMultiTips1Screen));
        ActivateNamedPanel(HxStr(kMultiTips1Screen));
    }
}

void MetLocNumPlayScreen::PlayCycleLeftSound(int) {
}

void MetLocNumPlayScreen::PlayCycleRightSound(int) {
}

MetLocNumPlayScreen *MetLocNumPlayScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetLocNumPlayScreen(pRenderer, nPriority);
}

MetLocNumPlayScreen::~MetLocNumPlayScreen() {
    delete mButtonList;
}
