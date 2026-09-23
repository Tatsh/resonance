#include "met/metlocnumplayscreen.h"

#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
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

// MetScreen::mUnknown18 values.
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
    mUnknown38.push_back(HxStr(kTwoPlayerKey));
    mUnknown38.push_back(HxStr(kThreePlayerKey));
    mUnknown38.push_back(HxStr(kFourPlayerKey));
    mUnknown38.push_back(HxStr(kTipsKey));
}

// 0x002adbf0
void MetLocNumPlayScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();

    HxStr twoLabel;
    QueryConfigString(&twoLabel, kLabelConfigCode, kTwoPlayerKey);
    mButtonList->Add(HxStr(kTwoPlayerButton), twoLabel);

    HxStr threeLabel;
    QueryConfigString(&threeLabel, kLabelConfigCode, kThreePlayerKey);
    mButtonList->Add(HxStr(kThreePlayerButton), threeLabel);

    HxStr fourLabel;
    QueryConfigString(&fourLabel, kLabelConfigCode, kFourPlayerKey);
    mButtonList->Add(HxStr(kFourPlayerButton), fourLabel);

    HxStr tipsLabel;
    QueryConfigString(&tipsLabel, kLabelConfigCode, kTipsKey);
    mButtonList->Add(HxStr(kTipsButton), tipsLabel);
}

// 0x002adef0
void MetLocNumPlayScreen::HandleCommand(const MetScreenCommand *pCommand) {
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
        ExitScreenByName(HxStr(kLeftGizmoScreen));
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// 0x002ae218
void MetLocNumPlayScreen::EnterAndShow() {
    HxStr title;
    QueryConfigString(&title, kTitleConfigCode, kTitleKey);
    MetScreenTitleScreen::SetTitle(title);

    mUnknown10->mUnknownd4 = kMaxControllers;
    int nIndex = MetFrontEndState::shared()->mUnknown2c - kFewestPlayers;
    if (nIndex < 0 || nIndex >= kPlayerCountButtons) {
        nIndex = 0;
    }
    mButtonList->SetSelected(nIndex);

    MetHelpScreen::SelectPreset(HxStr(kStandardTitlePreset));
    MetHelpScreen::SetText(mUnknown38[mButtonList->mSelected], mUnknown10->mUnknown68);
    MetScreen::EnterAndShow();
}

// 0x002ae350
void MetLocNumPlayScreen::OnUnknownSlot30(Rnd::Button *) {
    mUnknown18 = kExitChoice;
    ExitScreenByName(HxStr(kLeftGizmoScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    ExitScreenByName(HxStr(kHelpScreen));
    BeginExit();
}

// 0x002ae4f0
void MetLocNumPlayScreen::OnUnknownSlot36() {
    if (mUnknown18 == kExitBack) {
        PushNamedScreen(HxStr(kTopLogoScreen));
        PushNamedScreen(HxStr(kLeftGizmoSmallScreen));
        PushNamedScreen(HxStr(kMainScreen));
        ActivateNamedPanel(HxStr(kMainScreen));
        return;
    }

    const int nSelected = mButtonList->mSelected;
    if (nSelected < kPlayerCountButtons) {
        MetFrontEndState::shared()->mUnknown2c = nSelected + kFewestPlayers;
        mUnknown10->mUnknownd4 = nSelected + kFewestPlayers;
        MetFrontEndState::shared()->mUnknown24 = HxStr(kLocNumPlayScreen);
        PushNamedScreen(HxStr(kLocPickCharScreen));
        ActivateNamedPanel(HxStr(kLocPickCharScreen));
    } else {
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kMultiTips1Screen));
        ActivateNamedPanel(HxStr(kMultiTips1Screen));
    }
}

// 0x002b0f70
void MetLocNumPlayScreen::PlayCycleLeftSound(int) {
}

// 0x002b0f78
void MetLocNumPlayScreen::PlayCycleRightSound(int) {
}

// 0x002b0f80
MetLocNumPlayScreen *MetLocNumPlayScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetLocNumPlayScreen(pRenderer, nPriority);
}

MetLocNumPlayScreen::~MetLocNumPlayScreen() {
    delete mButtonList;
}
