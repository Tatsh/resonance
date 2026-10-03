#include "met/metjukeboxtopbuttonsscreen.h"

#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metremixmanager.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "met/metstrings.h"
#include "os/async.h"
#include "os/hxstr.h"
#include "rnd/asyncloader.h"
#include "script/configquery.h"

namespace {

// The screen name, the container directory, and the container.
static const char *const kScreenName = "jbb";
static const char *const kContainerDirectory = "metagame/Shared";
static const char *const kContainerFile = "juke_butts";

constexpr int kTitleConfigCode = 0x269;

// MetScreen::mExitChoice on exit, read back by OnExitFinished().
constexpr int kExitCancelled = 0;
constexpr int kExitNotCancelled = 2;

// The MetScreen::SetShowing() argument.
constexpr int kHidden = 0;
constexpr int kShown = 1;

// The button indices, in the order ResolveContainerViews() adds them.
enum {
    kSavedRemixesButton = 0,
    kFactoryRemixesButton = 1,
    kEditPlaylistButton = 2,
    kDoneButton = 3,
};

static const char *const kSavedRemixesObject = "jbb_SAVED REMIXES.but";
static const char *const kSavedRemixesLabel = "SAVED REMIXES";
static const char *const kFactoryRemixesObject = "jbb_FACTORY REMIXES.but";
static const char *const kFactoryRemixesLabel = "FACTORY REMIXES";
static const char *const kEditPlaylistObject = "jbb_EDIT PLAYLIST.but";
static const char *const kEditPlaylistLabel = "EDIT PLAYLIST";
static const char *const kDoneObject = "jbb_DONE.but";
static const char *const kDoneLabel = "DONE";

static const char *const kTitleKey = "met_jukebox_title";
static const char *const kCreateTitleKey = "met_jukebox_create_title";
static const char *const kEditTitleKey = "met_jukebox_edit_title";
static const char *const kPlayTitleKey = "met_jukebox_play_title";
static const char *const kStandardPreset = "standard_title";

static const char *const kCustomRemixesScreen = "MetJukeboxCustomRemixesScreen";
static const char *const kFactoryRemixesScreen = "MetJukeboxFactoryRemixesScreen";
static const char *const kEditPlaylistScreen = "MetJukeboxEditPlaylistScreen";
static const char *const kLowerLeftScreen = "MetJukeboxEditPlaylistScreenLowerLeft";
static const char *const kDoneScreen = "MetJukeboxEditPlaylistScreenDone";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kRemixTypeScreen = "MetRemixTypeScreen";

inline void ShowScreen(const char *pszName, int nShowing) {
    MetScreen::FindScreenByName(HxStr(pszName))->SetShowing(nShowing);
}

} // namespace

// NTSC-U/C: 0x00240c28, PAL: 0x002556d8
MetJukeboxTopButtonsScreen::MetJukeboxTopButtonsScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer,
                nPriority,
                HxStr(kScreenName),
                HxStr(kContainerDirectory),
                HxStr(kContainerFile)),
      mButtons(nullptr), mUnused(0) {
    mButtons = new MetButtonList;
}

// NTSC-U/C: 0x00246778, PAL: 0x0025b890
MetJukeboxTopButtonsScreen::~MetJukeboxTopButtonsScreen() {
}

// NTSC-U/C: 0x002466f0, PAL: 0x0025b808
MetJukeboxTopButtonsScreen *MetJukeboxTopButtonsScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetJukeboxTopButtonsScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x00240e28, PAL: 0x00255950
void MetJukeboxTopButtonsScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mButtons->Add(HxStr(kSavedRemixesObject), MetText(kMetStrJbbSavedRemixes, kSavedRemixesLabel));
    mButtons->Add(HxStr(kFactoryRemixesObject),
                  MetText(kMetStrJbbFactoryRemixes, kFactoryRemixesLabel));
    mButtons->Add(HxStr(kEditPlaylistObject), MetText(kMetStrJbbEditPlaylist, kEditPlaylistLabel));
    mButtons->Add(HxStr(kDoneObject), MetText(kMetStrJbbDone, kDoneLabel));
    mButtons->SetSelected(kSavedRemixesButton);
}

// NTSC-U/C: 0x00241120, PAL: 0x00255d20
void MetJukeboxTopButtonsScreen::OnExitFinished() {
    if (mExitChoice == kExitCancelled) {
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kTitleScreen));
        PushNamedScreen(HxStr(kRemixTypeScreen));
        ActivateNamedPanel(HxStr(kRemixTypeScreen));
    }
}

// NTSC-U/C: 0x00241320, PAL: 0x00255f98
void MetJukeboxTopButtonsScreen::ShowSelectedPanel() {
    const int nSelected = mButtons->mSelected;
    ShowScreen(kCustomRemixesScreen, kHidden);
    ShowScreen(kFactoryRemixesScreen, kHidden);
    ShowScreen(kEditPlaylistScreen, kHidden);
    ShowScreen(kLowerLeftScreen, kHidden);
    ShowScreen(kDoneScreen, kHidden);

    switch (nSelected) {
    case kSavedRemixesButton:
        ShowScreen(kCustomRemixesScreen, kShown);
        FindScreenByName(HxStr(kCustomRemixesScreen))->OnPanelActivated();
        mCommandTargetScreen = kCustomRemixesScreen;
        MetScreenTitleScreen::ReplaceTitle(
            MetConfigText(kMetStrTMetJukeboxCreateTitle, kTitleConfigCode, kCreateTitleKey));
        break;

    case kFactoryRemixesButton:
        ShowScreen(kFactoryRemixesScreen, kShown);
        FindScreenByName(HxStr(kFactoryRemixesScreen))->OnPanelActivated();
        mCommandTargetScreen = kFactoryRemixesScreen;
        MetScreenTitleScreen::ReplaceTitle(
            MetConfigText(kMetStrTMetJukeboxCreateTitle, kTitleConfigCode, kCreateTitleKey));
        break;

    case kEditPlaylistButton:
        ShowScreen(kEditPlaylistScreen, kShown);
        FindScreenByName(HxStr(kEditPlaylistScreen))->OnPanelActivated();
        ShowScreen(kLowerLeftScreen, kShown);
        mCommandTargetScreen = kEditPlaylistScreen;
        MetScreenTitleScreen::ReplaceTitle(
            MetConfigText(kMetStrTMetJukeboxEditTitle, kTitleConfigCode, kEditTitleKey));
        break;

    case kDoneButton:
        ShowScreen(kEditPlaylistScreen, kShown);
        ShowScreen(kDoneScreen, kShown);
        mCommandTargetScreen = kDoneScreen;
        MetScreenTitleScreen::ReplaceTitle(
            MetConfigText(kMetStrTMetJukeboxPlayTitle, kTitleConfigCode, kPlayTitleKey));
        break;

    default:
        break;
    }
}

// NTSC-U/C: 0x00241b08, PAL: 0x00256940
void MetJukeboxTopButtonsScreen::EnterAndShow() {
    MetScreen::EnterAndShow();
    mExitChoice = kExitNotCancelled;
    MetScreenTitleScreen::SetTitle(
        MetConfigText(kMetStrTMetJukeboxTitle, kTitleConfigCode, kTitleKey));

    if (MetFrontEndState::shared()->mPendingTransition != 0) {
        MetFrontEndState *pState = MetFrontEndState::shared();
        pState->mLastTransition = pState->mPendingTransition;
        pState->mPendingTransition = 0;
        MetHelpScreen::SelectPreset(MetText(kMetStrHStandardTitle, kStandardPreset));
        PushNamedScreen(HxStr(kHelpScreen));
        mRenderer->SetActivePanel(this);
    }

    bool loaded;
    do {
        loaded = FindScreenByName(HxStr(kCustomRemixesScreen))->PollContainerLoad() != 0;
        loaded =
            (FindScreenByName(HxStr(kFactoryRemixesScreen))->PollContainerLoad() != 0) && loaded;
        loaded = (FindScreenByName(HxStr(kEditPlaylistScreen))->PollContainerLoad() != 0) && loaded;
        loaded = (FindScreenByName(HxStr(kLowerLeftScreen))->PollContainerLoad() != 0) && loaded;
        loaded = (FindScreenByName(HxStr(kDoneScreen))->PollContainerLoad() != 0) && loaded;
        RndAsyncLoader::PollAsyncLoads();
        AsyncPumpCompletedRequests();
    } while (!loaded);

    MetRemixManager::shared()->PrunePlayList();
    PushNamedScreen(HxStr(kEditPlaylistScreen));
    PushNamedScreen(HxStr(kLowerLeftScreen));
    PushNamedScreen(HxStr(kDoneScreen));
    PushNamedScreen(HxStr(kCustomRemixesScreen));
    PushNamedScreen(HxStr(kFactoryRemixesScreen));
    mButtons->SetSelected(kSavedRemixesButton);
    ShowSelectedPanel();
}

// NTSC-U/C: 0x00242140, PAL: 0x002570e8
void MetJukeboxTopButtonsScreen::BeginExit() {
    MetScreen::BeginExit();
    ExitScreenByName(HxStr(kCustomRemixesScreen));
    ExitScreenByName(HxStr(kFactoryRemixesScreen));
    ExitScreenByName(HxStr(kEditPlaylistScreen));
    ExitScreenByName(HxStr(kLowerLeftScreen));
    ExitScreenByName(HxStr(kDoneScreen));
}

// NTSC-U/C: 0x002467e8, PAL: 0x0025b910
void MetJukeboxTopButtonsScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandLeft:
        mButtons->SelectPrevious();
        ShowSelectedPanel();
        break;

    case kMetScreenCommandRight:
        mButtons->SelectNext();
        ShowSelectedPanel();
        break;

    case kMetScreenCommandBack:
        mExitChoice = kExitCancelled;
        BeginExit();
        break;

    default:
        FindScreenByName(mCommandTargetScreen)->DeliverCommand(pCommand);
        break;
    }
}

// NTSC-U/C: 0x002468b8, PAL: 0x0025b9e0
void MetJukeboxTopButtonsScreen::OnEnterFinished() {
    ShowSelectedPanel();
}

// NTSC-U/C: 0x002468d8, PAL: 0x0025ba00
void MetJukeboxTopButtonsScreen::UpdateIdle(float) {
}
