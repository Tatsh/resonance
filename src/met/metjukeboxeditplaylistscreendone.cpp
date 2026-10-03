#include "met/metjukeboxeditplaylistscreendone.h"

#include <vector>

#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metremixmanager.h"
#include "met/metrenderer.h"
#include "met/metstrings.h"
#include "os/hxstr.h"
#include "rnd/button.h"

namespace {

// The screen name, the container directory, and the container.
static const char *const kScreenName = "jbd";
static const char *const kContainerDirectory = "metagame/Shared";
static const char *const kContainerFile = "juke_done_butts";

static const char *const kRandomButton = "jbd_random.but";
static const char *const kRandomLabel = "PLAY RANDOM";
static const char *const kOrderButton = "jbd_order.but";
static const char *const kOrderLabel = "PLAY IN ORDER";
static const char *const kSaveButton = "jbd_save.but";
static const char *const kSaveLabel = "SAVE PLAYLIST";

static const char *const kPlayLayout = "met_jukebox_done_screen_tab_play";
static const char *const kPlayText = "met_jukebox_done_screen_ticker_play";
static const char *const kSaveLayout = "met_jukebox_done_screen_tab_save";
static const char *const kSaveText = "met_jukebox_done_screen_ticker_save";

static const char *const kTopButtonsScreen = "MetJukeboxTopButtonsScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kThisScreen = "MetJukeboxEditPlaylistScreenDone";

// The three buttons, in the order slot 38 adds them.
enum Button {
    kButtonRandom = 0,
    kButtonOrder = 1,
    kButtonSave = 2,
};

// Rnd::Button states. MetButtonList passes over a button in the disabled state.
constexpr int kButtonStateNormal = 0;
constexpr int kButtonStateDisabled = 3;

constexpr int kButtonFlashCycles = 2;
constexpr float kButtonFlashInterval = 30.0f;

} // namespace

// 0x00231728
MetJukeboxEditPlaylistScreenDone::MetJukeboxEditPlaylistScreenDone(MetRenderer *pRenderer,
                                                                   int nPriority)
    : MetScreen(pRenderer,
                nPriority,
                HxStr(kScreenName),
                HxStr(kContainerDirectory),
                HxStr(kContainerFile)),
      // Yes, the binary clears the member and then overwrites it in the body below.
      mButtons(nullptr), mSaveChosen(0), mShuffle(0), mPlayChosen(0) {
    mButtons = new MetButtonList;
}

// NTSC-U/C: 0x00231908, PAL: 0x002452a0
void MetJukeboxEditPlaylistScreenDone::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mButtons->Add(HxStr(kRandomButton), MetText(kMetStrJbdRandom, kRandomLabel));
    mButtons->Add(HxStr(kOrderButton), MetText(kMetStrJbdOrder, kOrderLabel));
    mButtons->Add(HxStr(kSaveButton), MetText(kMetStrJbdSave, kSaveLabel));
    mButtons->SetSelected(kButtonRandom);
}

// 0x00231b50
void MetJukeboxEditPlaylistScreenDone::OnExitFinished() {
    if (mSaveChosen != 0) {
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kHelpScreen));
        std::vector<HxStr> screens;
        screens.push_back(HxStr(kTopButtonsScreen));
        screens.push_back(HxStr(kTitleScreen));
        screens.push_back(HxStr(kHelpScreen));
        MetRemixManager::shared()->SavePlayList(screens);
    } else if (mPlayChosen != 0) {
        std::vector<HxStr> screens(1);
        screens[0] = kTopButtonsScreen;
        ActivateNamedPanel(HxStr(""));
        MetRemixManager::shared()->StartPlayList(screens, mShuffle);
        MetFrontEndState::shared()->mReturnScreen = HxStr(kThisScreen);
    }
    mPlayChosen = 0;
    mShuffle = 0;
    mSaveChosen = 0;
}

// 0x002321f8
void MetJukeboxEditPlaylistScreenDone::OnRepeatingSoundFinished(
    [[maybe_unused]] Rnd::Button *pButton) {
    switch (mButtons->mSelected) {
    case kButtonRandom:
        if (MetRemixManager::shared()->mPlayList.entries.size() == 0) {
            return;
        }
        mShuffle = 1;
        mPlayChosen = 1;
        ExitScreenByName(HxStr(kTopButtonsScreen));
        ExitScreenByName(HxStr(kHelpScreen));
        ExitScreenByName(HxStr(kTitleScreen));
        break;

    case kButtonOrder:
        if (MetRemixManager::shared()->mPlayList.entries.size() == 0) {
            return;
        }
        mPlayChosen = 1;
        mShuffle = 0;
        ExitScreenByName(HxStr(kTopButtonsScreen));
        ExitScreenByName(HxStr(kHelpScreen));
        ExitScreenByName(HxStr(kTitleScreen));
        break;

    case kButtonSave:
        mSaveChosen = 1;
        ExitScreenByName(HxStr(kTopButtonsScreen));
        break;

    default:
        break;
    }
}

// NTSC-U/C: 0x00232598, PAL: 0x00246400
void MetJukeboxEditPlaylistScreenDone::UpdateHelpText() {
    HxStr layout;
    HxStr text;
    switch (mButtons->mSelected) {
    case kButtonRandom:
    case kButtonOrder:
        layout = MetText(kMetStrHMetJukeboxDoneScreenTabPlay, kPlayLayout);
        text = MetText(kMetStrHMetJukeboxDoneScreenTickerPlay, kPlayText);
        break;

    case kButtonSave:
        layout = MetText(kMetStrHMetJukeboxDoneScreenTabSave, kSaveLayout);
        text = MetText(kMetStrHMetJukeboxDoneScreenTickerSave, kSaveText);
        break;

    default:
        break;
    }
    MetHelpScreen::SelectPreset(layout);
    MetHelpScreen::SetText(text, mRenderer->mAnimationFrame);
}

// 0x00237160
void MetJukeboxEditPlaylistScreenDone::PlaySlideSound(int) {
}

// 0x00237168
void MetJukeboxEditPlaylistScreenDone::PlayLeaveSound(int) {
}

// 0x00237170
void MetJukeboxEditPlaylistScreenDone::PlayHighSound(int) {
}

// 0x00237178
void MetJukeboxEditPlaylistScreenDone::PlayCycleLeftSound(int) {
}

// 0x00237180
void MetJukeboxEditPlaylistScreenDone::PlayCycleRightSound(int) {
}

// 0x00237188
MetJukeboxEditPlaylistScreenDone *MetJukeboxEditPlaylistScreenDone::New(MetRenderer *pRenderer,
                                                                        int nPriority) {
    return new MetJukeboxEditPlaylistScreenDone(pRenderer, nPriority);
}

// 0x00237210
MetJukeboxEditPlaylistScreenDone::~MetJukeboxEditPlaylistScreenDone() {
}

// 0x00237268
void MetJukeboxEditPlaylistScreenDone::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mButtons->SelectPrevious();
        UpdateHelpText();
        break;

    case kMetScreenCommandNext:
        mButtons->SelectNext();
        UpdateHelpText();
        break;

    case kMetScreenCommandSelect:
        StartRepeatingSound(mRenderer->mAnimationFrame,
                            kButtonFlashInterval,
                            mButtons->mSelectedButton,
                            kButtonFlashCycles);
        break;

    default:
        break;
    }
}

// 0x00237330
void MetJukeboxEditPlaylistScreenDone::EnterAndShow() {
    MetScreen::EnterAndShow();
    mButtons->SetSelected(kButtonRandom);
    if (MetFrontEndState::shared()->mUsingMemcard != 0) {
        mButtons->ButtonAt(kButtonSave)->SetState(kButtonStateNormal);
    } else {
        mButtons->ButtonAt(kButtonSave)->SetState(kButtonStateDisabled);
    }
}

// 0x002373a8
void MetJukeboxEditPlaylistScreenDone::OnEnterFinished() {
    mButtons->SetSelected(kButtonRandom);
    mPlayChosen = 0;
    mShuffle = 0;
    mSaveChosen = 0;
}

// 0x002373e0
void MetJukeboxEditPlaylistScreenDone::SetShowing(int nShowing) {
    MetScreen::SetShowing(nShowing);
    if (nShowing != 0) {
        UpdateHelpText();
    }
}
