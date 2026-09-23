#include "met/metjukeboxtopbuttonsscreen.h"

#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metremixmanager.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
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

// MetScreen::mUnknown18 on exit, read back by OnUnknownSlot36().
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

inline HxStr ConfigText(int nCode, const char *pszKey) {
    HxStr text = QueryConfigString(nCode, pszKey);
    return text;
}

inline void ShowScreen(const char *pszName, int nShowing) {
    MetScreen::FindScreenByName(HxStr(pszName))->SetShowing(nShowing);
}

} // namespace

// 0x00240c28
MetJukeboxTopButtonsScreen::MetJukeboxTopButtonsScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer,
                nPriority,
                HxStr(kScreenName),
                HxStr(kContainerDirectory),
                HxStr(kContainerFile)),
      mUnknown94(nullptr), mUnknown98(0) {
    mUnknown94 = new MetButtonList;
}

// 0x00246778
MetJukeboxTopButtonsScreen::~MetJukeboxTopButtonsScreen() {
}

// 0x002466f0
MetJukeboxTopButtonsScreen *MetJukeboxTopButtonsScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetJukeboxTopButtonsScreen(pRenderer, nPriority);
}

// 0x00240e28
void MetJukeboxTopButtonsScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mUnknown94->Add(HxStr(kSavedRemixesObject), HxStr(kSavedRemixesLabel));
    mUnknown94->Add(HxStr(kFactoryRemixesObject), HxStr(kFactoryRemixesLabel));
    mUnknown94->Add(HxStr(kEditPlaylistObject), HxStr(kEditPlaylistLabel));
    mUnknown94->Add(HxStr(kDoneObject), HxStr(kDoneLabel));
    mUnknown94->SetSelected(kSavedRemixesButton);
}

// 0x00241120
void MetJukeboxTopButtonsScreen::OnUnknownSlot36() {
    if (mUnknown18 == kExitCancelled) {
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kTitleScreen));
        PushNamedScreen(HxStr(kRemixTypeScreen));
        ActivateNamedPanel(HxStr(kRemixTypeScreen));
    }
}

// 0x00241320
void MetJukeboxTopButtonsScreen::ShowSelectedPanel() {
    const int nSelected = mUnknown94->mSelected;
    ShowScreen(kCustomRemixesScreen, kHidden);
    ShowScreen(kFactoryRemixesScreen, kHidden);
    ShowScreen(kEditPlaylistScreen, kHidden);
    ShowScreen(kLowerLeftScreen, kHidden);
    ShowScreen(kDoneScreen, kHidden);

    switch (nSelected) {
    case kSavedRemixesButton:
        ShowScreen(kCustomRemixesScreen, kShown);
        FindScreenByName(HxStr(kCustomRemixesScreen))->OnUnknownSlot7();
        mUnknown8c = kCustomRemixesScreen;
        MetScreenTitleScreen::ReplaceTitle(ConfigText(kTitleConfigCode, kCreateTitleKey));
        break;

    case kFactoryRemixesButton:
        ShowScreen(kFactoryRemixesScreen, kShown);
        FindScreenByName(HxStr(kFactoryRemixesScreen))->OnUnknownSlot7();
        mUnknown8c = kFactoryRemixesScreen;
        MetScreenTitleScreen::ReplaceTitle(ConfigText(kTitleConfigCode, kCreateTitleKey));
        break;

    case kEditPlaylistButton:
        ShowScreen(kEditPlaylistScreen, kShown);
        FindScreenByName(HxStr(kEditPlaylistScreen))->OnUnknownSlot7();
        ShowScreen(kLowerLeftScreen, kShown);
        mUnknown8c = kEditPlaylistScreen;
        MetScreenTitleScreen::ReplaceTitle(ConfigText(kTitleConfigCode, kEditTitleKey));
        break;

    case kDoneButton:
        ShowScreen(kEditPlaylistScreen, kShown);
        ShowScreen(kDoneScreen, kShown);
        mUnknown8c = kDoneScreen;
        MetScreenTitleScreen::ReplaceTitle(ConfigText(kTitleConfigCode, kPlayTitleKey));
        break;

    default:
        break;
    }
}

// 0x00241b08
void MetJukeboxTopButtonsScreen::EnterAndShow() {
    MetScreen::EnterAndShow();
    mUnknown18 = kExitNotCancelled;
    MetScreenTitleScreen::SetTitle(ConfigText(kTitleConfigCode, kTitleKey));

    if (MetFrontEndState::shared()->mUnknown18 != 0) {
        MetFrontEndState *pState = MetFrontEndState::shared();
        pState->mUnknown1c = pState->mUnknown18;
        pState->mUnknown18 = 0;
        MetHelpScreen::SelectPreset(HxStr(kStandardPreset));
        PushNamedScreen(HxStr(kHelpScreen));
        mUnknown10->SetActivePanel(this);
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
    mUnknown94->SetSelected(kSavedRemixesButton);
    ShowSelectedPanel();
}

// 0x00242140
void MetJukeboxTopButtonsScreen::BeginExit() {
    MetScreen::BeginExit();
    ExitScreenByName(HxStr(kCustomRemixesScreen));
    ExitScreenByName(HxStr(kFactoryRemixesScreen));
    ExitScreenByName(HxStr(kEditPlaylistScreen));
    ExitScreenByName(HxStr(kLowerLeftScreen));
    ExitScreenByName(HxStr(kDoneScreen));
}

// 0x002467e8
void MetJukeboxTopButtonsScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandLeft:
        mUnknown94->OnUnknownSlot2();
        ShowSelectedPanel();
        break;

    case kMetScreenCommandRight:
        mUnknown94->OnUnknownSlot3();
        ShowSelectedPanel();
        break;

    case kMetScreenCommandBack:
        mUnknown18 = kExitCancelled;
        BeginExit();
        break;

    default:
        FindScreenByName(mUnknown8c)->DeliverCommand(pCommand);
        break;
    }
}

// 0x002468b8
void MetJukeboxTopButtonsScreen::OnUnknownSlot33() {
    ShowSelectedPanel();
}

// 0x002468d8
void MetJukeboxTopButtonsScreen::OnUnknownSlot26(float) {
}
