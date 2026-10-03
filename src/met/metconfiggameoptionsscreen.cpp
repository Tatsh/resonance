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
#include "met/metstrings.h"
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

// MetScreen::mExitChoice on exit, read back by OnExitFinished().
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

inline Rnd::Button *FindArrow(const char *pszFormat, int nNumber) {
    char szName[kArrowNameBufferSize];
    sprintf(szName, pszFormat, nNumber);
    return dynamic_cast<Rnd::Button *>(Rnd::g_manager.Find(HxStr(szName)));
}

} // namespace

// NTSC-U/C: 0x0020c3e0, PAL: 0x00215828
MetConfigGameOptionsScreen::MetConfigGameOptionsScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreenMultiSoundBank(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mRows(nullptr) {
    mRows = new MetButtonList();
    mHelpKeys.push_back(MetText(kMetStrHPangameAudio, kAudioRowKey));
    mHelpKeys.push_back(MetText(kMetStrHPangameForceFeedback, kForceFeedbackRowKey));
}

// NTSC-U/C: 0x0020c800, PAL: 0x00215ce0
void MetConfigGameOptionsScreen::ResolveContainerViews() {
    MetScreenMultiSoundBank::ResolveContainerViews();

    // Yes, the binary does not test either label for null.
    dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(kAudioLabel)))
        ->SetText(MetConfigText(kMetStrPangameAudioLbl, kPromptConfigCode, kAudioLabelKey));
    dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(kForceFeedbackLabel)))
        ->SetText(MetConfigText(kMetStrPangameForceLbl, kPromptConfigCode, kForceFeedbackLabelKey));

    mRows->Add(HxStr(kAudioButton),
               MetConfigText(kMetStrPangameAudio, kPromptConfigCode, kAudioRowKey));
    mRows->Add(HxStr(kForceFeedbackButton),
               MetConfigText(kMetStrPangameForceFeedback, kPromptConfigCode, kForceFeedbackRowKey));

    mLeftArrows.resize(kRowCount);
    mRightArrows.resize(kRowCount);
    for (int nRow = 0; nRow < kRowCount; ++nRow) {
        mLeftArrows[nRow] = FindArrow(kLeftArrowFormat, nRow + 1);
        mRightArrows[nRow] = FindArrow(kRightArrowFormat, nRow + 1);
    }
}

// 0x0020ce20
MetConfigGameOptionsScreen::~MetConfigGameOptionsScreen() {
    delete mRows;
}

// 0x0020cf70
void MetConfigGameOptionsScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mRows->SelectPrevious();
        MetHelpScreen::SetText(mHelpKeys[mRows->mSelected], mRenderer->mAnimationFrame);
        break;

    case kMetScreenCommandNext:
        mRows->SelectNext();
        MetHelpScreen::SetText(mHelpKeys[mRows->mSelected], mRenderer->mAnimationFrame);
        break;

    case kMetScreenCommandLeft: {
        const int nRow = mRows->mSelected;
        StartRepeatingSound(
            mRenderer->mAnimationFrame, kArrowFlashInterval, mLeftArrows[nRow], kArrowFlashCycles);
        ToggleOption(nRow);
        break;
    }

    case kMetScreenCommandRight: {
        const int nRow = mRows->mSelected;
        StartRepeatingSound(
            mRenderer->mAnimationFrame, kArrowFlashInterval, mRightArrows[nRow], kArrowFlashCycles);
        ToggleOption(nRow);
        break;
    }

    case kMetScreenCommandSelect:
        ActivateNamedPanel(HxStr(""));
        ApplyOptions();
        mExitChoice = kExitApplied;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kHelpScreen));
        BeginExit();
        break;

    case kMetScreenCommandBack:
        mExitChoice = kExitCancelled;
        if (MetFrontEndState::shared()->mReturnScreen == kPauseGameScreen ||
            MetFrontEndState::shared()->mReturnScreen == kPauseRemixScreen) {
            ExitScreenByName(HxStr(kHelpScreen));
            ExitScreenByName(HxStr(kTitleScreen));
        }
        BeginExit();
        break;

    default:
        break;
    }
}

// NTSC-U/C: 0x0020d310, PAL: 0x00216980
void MetConfigGameOptionsScreen::EnterAndShow() {
    mRows->SetSelected(kFirstRow);
    MetScreenTitleScreen::SetTitle(
        MetConfigText(kMetStrTPangameOptions, kTitleConfigCode, kTitleKey));
    MetHelpScreen::SetText(mHelpKeys[mRows->mSelected], mRenderer->mAnimationFrame);
#ifdef VIDEO_STANDARD_PAL
    if (MetFrontEndState::shared()->mReturnScreen == kPauseGameScreen ||
        MetFrontEndState::shared()->mReturnScreen == kPauseRemixScreen) {
        MetHelpScreen::SelectPreset(GetMetString(kMetStrHStandardTitle));
    } else {
        MetHelpScreen::SelectPreset(GetMetString(kMetStrHPangameTabText));
    }
#else
    MetHelpScreen::SelectPreset(HxStr(kTabPreset));
#endif
    mOptions = GlobalSettings::shared()->mGameOptions;
    UpdateOptionLabels();
    MetScreenMultiSoundBank::EnterAndShow();
}

// NTSC-U/C: 0x0020d448, PAL: 0x00216b98
void MetConfigGameOptionsScreen::UpdateOptionLabels() {
    mRows->ButtonAt(kRowAudio)->mText->SetText(mOptions.mStereo != 0 ?
                                                   MetText(kMetStrGsStereo, kStereoText) :
                                                   MetText(kMetStrGsMono, kMonoText));
    mRows->ButtonAt(kRowForceFeedback)
        ->mText->SetText(mOptions.mForceFeedback != 0 ? MetText(kMetStrGsFfOn, kOnText) :
                                                        MetText(kMetStrGsFfOff, kOffText));
}

// 0x0020d5a8
void MetConfigGameOptionsScreen::OnExitFinished() {
    if (MetFrontEndState::shared()->mReturnScreen == kPauseGameScreen ||
        MetFrontEndState::shared()->mReturnScreen == kPauseRemixScreen) {
        if (MetFrontEndState::shared()->mUsingMemcard == kFrontEndFlagSet &&
            mExitChoice != kExitCancelled) {
            MetFrontEndState::shared()->mSettingsDirty = kFrontEndFlagSet;
        }
        if (MetFrontEndState::shared()->mReturnScreen == kPauseGameScreen) {
            PushNamedScreen(HxStr(kPauseGameScreen));
            ActivateNamedPanel(HxStr(kPauseGameScreen));
        } else {
            ExitScreenByName(HxStr(kHelpScreen));
            PushNamedScreen(HxStr(kPauseRemixScreen));
            ActivateNamedPanel(HxStr(kPauseRemixScreen));
        }
        MetFrontEndState::shared()->mReturnScreen = HxStr("");
        return;
    }

    if (mExitChoice == kExitCancelled) {
        PushNamedScreen(HxStr(kRightGizmoScreen));
        PushNamedScreen(HxStr(kOptionsButtonsScreen));
        ActivateNamedPanel(HxStr(kOptionsButtonsScreen));
        return;
    }

    GlobalSettings::shared()->mGameOptions = mOptions;
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
        mOptions.mStereo ^= 1;
    } else if (nRow == kRowForceFeedback) {
        mOptions.mForceFeedback ^= 1;
    }
    UpdateOptionLabels();
}

// 0x002115c8
void MetConfigGameOptionsScreen::ApplyOptions() {
    GlobalSettings::shared()->mGameOptions = mOptions;
    Application::shared()->GetSynth()->SetStereo(mOptions.mStereo);
    GrooveWorld *pWorld = Application::shared()->GetWorld();
    if (pWorld != nullptr) {
        pWorld->mForceFeedback->SetEnabled(mOptions.mForceFeedback);
        pWorld->mForceFeedback->StartMetronome(Mid::MBT(0));
    }
}
