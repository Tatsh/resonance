#include "met/metconfiggameoptionsscreen.h"

#include <cstdio>
#include <vector>

#include "app/application.h"
#include "game/forcefeedbackmgr.h"
#include "game/globalsettings.h"
#include "game/grooveworld.h"
#include "met/metfrontendstate.h"
#include "met/metglobalsettingssaverscreen.h"
#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "mid/mbt.h"
#include "os/hxstr.h"
#include "rnd/button.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "script/configquery.h"
#include "synth/ps2hardsynth.h"

namespace {

static const char *const kScreenName = "nop";
static const char *const kDirectory = "metagame/shared";
static const char *const kContainerName = "net_options_pangame";

constexpr int kPromptConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x269;

// The two rows, in MetButtonList order.
enum Row {
    kRowAudio = 0,
    kRowForceFeedback = 1,
    kRowCount = 2,
};

// MetScreen::mUnknown18 on exit, read back by OnUnknownSlot36().
constexpr int kExitCancelled = 0;
constexpr int kExitApplied = 2;

constexpr int kFirstRow = 0;
constexpr int kFrontEndFlagSet = 1;
constexpr int kArrowFlashCycles = 2;
constexpr float kArrowFlashInterval = 30.0f;
constexpr int kArrowNameBufferSize = 112;

static const char *const kAudioRowKey = "pangame_audio";
static const char *const kForceFeedbackRowKey = "pangame_force_feedback";

static const char *const kAudioLabel = "nop_audio.txt";
static const char *const kAudioLabelKey = "pangame_audio_lbl";
static const char *const kForceFeedbackLabel = "nop_feedback.txt";
static const char *const kForceFeedbackLabelKey = "pangame_force_lbl";
static const char *const kAudioButton = "pangame_audio.but";
static const char *const kForceFeedbackButton = "pangame_force_feedback.but";
// Counted from 1.
static const char *const kLeftArrowFormat = "arr_left_0%i.but";
static const char *const kRightArrowFormat = "arr_right_0%i.but";

static const char *const kTitleKey = "pangame_options";
static const char *const kTabPreset = "pangame_tab_text";

static const char *const kStereoText = "STEREO";
static const char *const kMonoText = "MONO";
static const char *const kOnText = "ON";
static const char *const kOffText = "OFF";

static const char *const kPauseGameScreen = "MetPauseSoloGameScreen";
static const char *const kPauseRemixScreen = "MetPauseSoloRemixScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kRightGizmoScreen = "MetRightGizmoScreen";
static const char *const kOptionsButtonsScreen = "MetConfigOptionsButtonsScreen";

inline HxStr ConfigText(int nCode, const char *pszKey) {
    HxStr text;
    QueryConfigString(&text, nCode, pszKey);
    return text;
}

inline Rnd::Button *FindArrow(const char *pszFormat, int nNumber) {
    char szName[kArrowNameBufferSize];
    sprintf(szName, pszFormat, nNumber);
    return dynamic_cast<Rnd::Button *>(Rnd::g_manager.Find(HxStr(szName)));
}

} // namespace

// 0x0020c3e0
MetConfigGameOptionsScreen::MetConfigGameOptionsScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreenMultiSoundBank(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknown8c(nullptr) {
    mUnknown8c = new MetButtonList();
    mUnknown38.push_back(HxStr(kAudioRowKey));
    mUnknown38.push_back(HxStr(kForceFeedbackRowKey));
}

// 0x0020c800
void MetConfigGameOptionsScreen::ResolveContainerViews() {
    MetScreenMultiSoundBank::ResolveContainerViews();

    // Yes, the binary does not test either label for null.
    dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(kAudioLabel)))
        ->SetText(ConfigText(kPromptConfigCode, kAudioLabelKey));
    dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(kForceFeedbackLabel)))
        ->SetText(ConfigText(kPromptConfigCode, kForceFeedbackLabelKey));

    mUnknown8c->Add(HxStr(kAudioButton), ConfigText(kPromptConfigCode, kAudioRowKey));
    mUnknown8c->Add(HxStr(kForceFeedbackButton),
                    ConfigText(kPromptConfigCode, kForceFeedbackRowKey));

    mUnknown90.resize(kRowCount);
    mUnknown9c.resize(kRowCount);
    for (int nRow = 0; nRow < kRowCount; ++nRow) {
        mUnknown90[nRow] = FindArrow(kLeftArrowFormat, nRow + 1);
        mUnknown9c[nRow] = FindArrow(kRightArrowFormat, nRow + 1);
    }
}

// 0x0020ce20
MetConfigGameOptionsScreen::~MetConfigGameOptionsScreen() {
    delete mUnknown8c;
}

// 0x0020cf70
void MetConfigGameOptionsScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mUnknown8c->OnUnknownSlot2();
        MetHelpScreen::SetText(mUnknown38[mUnknown8c->mSelected], mUnknown10->mUnknown68);
        break;

    case kMetScreenCommandNext:
        mUnknown8c->OnUnknownSlot3();
        MetHelpScreen::SetText(mUnknown38[mUnknown8c->mSelected], mUnknown10->mUnknown68);
        break;

    case kMetScreenCommandLeft: {
        const int nRow = mUnknown8c->mSelected;
        StartRepeatingSound(
            mUnknown10->mUnknown68, kArrowFlashInterval, mUnknown90[nRow], kArrowFlashCycles);
        ToggleOption(nRow);
        break;
    }

    case kMetScreenCommandRight: {
        const int nRow = mUnknown8c->mSelected;
        StartRepeatingSound(
            mUnknown10->mUnknown68, kArrowFlashInterval, mUnknown9c[nRow], kArrowFlashCycles);
        ToggleOption(nRow);
        break;
    }

    case kMetScreenCommandSelect:
        ActivateNamedPanel(HxStr(""));
        ApplyOptions();
        mUnknown18 = kExitApplied;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kHelpScreen));
        BeginExit();
        break;

    case kMetScreenCommandBack:
        mUnknown18 = kExitCancelled;
        if (MetFrontEndState::shared()->mUnknown24 == kPauseGameScreen ||
            MetFrontEndState::shared()->mUnknown24 == kPauseRemixScreen) {
            ExitScreenByName(HxStr(kHelpScreen));
            ExitScreenByName(HxStr(kTitleScreen));
        }
        BeginExit();
        break;

    default:
        break;
    }
}

// 0x0020d310
void MetConfigGameOptionsScreen::EnterAndShow() {
    mUnknown8c->SetSelected(kFirstRow);
    MetScreenTitleScreen::SetTitle(ConfigText(kTitleConfigCode, kTitleKey));
    MetHelpScreen::SetText(mUnknown38[mUnknown8c->mSelected], mUnknown10->mUnknown68);
    MetHelpScreen::SelectPreset(HxStr(kTabPreset));
    mUnknowna8 = GlobalSettings::shared()->mGameOptions;
    UpdateOptionLabels();
    MetScreenMultiSoundBank::EnterAndShow();
}

// 0x0020d448
void MetConfigGameOptionsScreen::UpdateOptionLabels() {
    mUnknown8c->ButtonAt(kRowAudio)->mText->SetText(
        HxStr(mUnknowna8.mUnknown00 != 0 ? kStereoText : kMonoText));
    mUnknown8c->ButtonAt(kRowForceFeedback)
        ->mText->SetText(HxStr(mUnknowna8.mUnknown08 != 0 ? kOnText : kOffText));
}

// 0x0020d5a8
void MetConfigGameOptionsScreen::OnUnknownSlot36() {
    if (MetFrontEndState::shared()->mUnknown24 == kPauseGameScreen ||
        MetFrontEndState::shared()->mUnknown24 == kPauseRemixScreen) {
        if (MetFrontEndState::shared()->mUnknown0c == kFrontEndFlagSet &&
            mUnknown18 != kExitCancelled) {
            MetFrontEndState::shared()->mUnknown10 = kFrontEndFlagSet;
        }
        if (MetFrontEndState::shared()->mUnknown24 == kPauseGameScreen) {
            PushNamedScreen(HxStr(kPauseGameScreen));
            ActivateNamedPanel(HxStr(kPauseGameScreen));
        } else {
            ExitScreenByName(HxStr(kHelpScreen));
            PushNamedScreen(HxStr(kPauseRemixScreen));
            ActivateNamedPanel(HxStr(kPauseRemixScreen));
        }
        MetFrontEndState::shared()->mUnknown24 = HxStr("");
        return;
    }

    if (mUnknown18 == kExitCancelled) {
        PushNamedScreen(HxStr(kRightGizmoScreen));
        PushNamedScreen(HxStr(kOptionsButtonsScreen));
        ActivateNamedPanel(HxStr(kOptionsButtonsScreen));
        return;
    }

    GlobalSettings::shared()->mGameOptions = mUnknowna8;
    std::vector<HxStr> screens;
    screens.push_back(HxStr(kOptionsButtonsScreen));
    screens.push_back(HxStr(kRightGizmoScreen));
    screens.push_back(HxStr(kHelpScreen));
    MetGlobalSettingsSaverScreen::StartSave(screens);
}

// 0x002114f0
MetConfigGameOptionsScreen *MetConfigGameOptionsScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetConfigGameOptionsScreen(pRenderer, nPriority);
}

// 0x00211578
void MetConfigGameOptionsScreen::ToggleOption(int nRow) {
    if (nRow == kRowAudio) {
        mUnknowna8.mUnknown00 ^= 1;
    } else if (nRow == kRowForceFeedback) {
        mUnknowna8.mUnknown08 ^= 1;
    }
    UpdateOptionLabels();
}

// 0x002115c8
void MetConfigGameOptionsScreen::ApplyOptions() {
    GlobalSettings::shared()->mGameOptions = mUnknowna8;
    Application::shared()->GetSynth()->Slot12(mUnknowna8.mUnknown00);
    GrooveWorld *pWorld = Application::shared()->GetWorld();
    if (pWorld != nullptr) {
        pWorld->mForceFeedback->SetEnabled(mUnknowna8.mUnknown08);
        pWorld->mForceFeedback->StartMetronome(Mid::MBT(0));
    }
}
