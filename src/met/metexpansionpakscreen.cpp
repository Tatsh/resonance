#include "met/metexpansionpakscreen.h"

#include <vector>

#include "game/campaignstats.h"
#include "met/metfade.h"
#include "met/metfreqmakerassetmanager.h"
#include "met/metmsgscreen.h"
#include "met/metpersonadata.h"
#include "met/metrenderer.h"
#include "met/metsonglists.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "script/configquery.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "dlg";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "dialogue";

// The message screens the state machine drives. Each name is also its text's configuration key.
static const char *const kPrepareMessage = "expansion_prepare";
static const char *const kLoadMessage = "expansion_load";
static const char *const kCheckMessage = "expansion_check";
static const char *const kDoneMessage = "expansion_done";
static const char *const kRetryText = "expansion_retry";
static const char *const kDoneTextBefore = "expansion_done1";
static const char *const kDoneTextAfter = "expansion_done2";
static const char *const kDoneTextFormat = "%s  \"%s\"  %s";
static const char *const kMessageTitle = "FREQUENCY";

static const char *const kContinueButton = "CONTINUE";
static const char *const kCancelButton = "CANCEL";
static const char *const kRetryButton = "RETRY";

static const char *const kMsgScreen = "MetMsgScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kRightGizmoScreen = "MetRightGizmoScreen";
static const char *const kOptionsButtonsScreen = "MetConfigOptionsButtonsScreen";
static const char *const kLeftGizmoSmallScreen = "MetLeftGizmoSmallScreen";
static const char *const kTopLogoScreen = "MetTopLogoScreen";
static const char *const kMainScreen = "MetMainScreen";

constexpr int kPromptConfigCode = 0x258;
// The configuration code of the expansion disc's title, read with no key.
constexpr int kExpansionTitleConfigCode = 0x515;

// The states mState takes beyond the DiscSwap::Step values.
constexpr int kStateIdle = 0;
constexpr int kStateReleased = 1;
constexpr int kStateRetryMount = 8;
constexpr int kStateLastFailure = 13;

// MetScreen::mExitChoice on exit, read back by OnFadeOutDone().
constexpr int kExitCancelled = 0;
constexpr int kExitDone = 2;

constexpr int kChoiceFirst = 1;
constexpr int kNoButtons = 0;
constexpr int kOneButton = 1;
constexpr int kTwoButtons = 2;
constexpr float kFadeDuration = 360.0f;
constexpr int kRetainView = 1;
constexpr int kReleaseView = 0;

inline HxStr ConfigText(int nCode, const char *pszKey) {
    HxStr text = QueryConfigString(nCode, pszKey);
    return text;
}

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// Merges the level list of every record in a list again.
inline void MergeLevelLists(const std::vector<MetPersonaData *> &personas) {
    for (std::vector<MetPersonaData *>::size_type i = 0; i < personas.size(); ++i) {
        personas[i]->mStats.MergeLevelList();
    }
}

} // namespace

// 0x00218320
MetExpansionPakScreen::MetExpansionPakScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mFade(nullptr) {
    mFade = new MetFade(pRenderer);
}

// 0x00218518
void MetExpansionPakScreen::UpdateIdle(float flTime) {
    mFade->Update(flTime);
    if (mFinished != 0) {
        return;
    }

    if (mState == kStateIdle && mPrepareShown != 0) {
        mState = mDiscSwap.ReleaseDisc();
    }
    if (mState == kStateReleased) {
        mDiscSwap.BeginEject();
        mState = mDiscSwap.PollEject();
        return;
    }
    if (mState == DiscSwap::kStepEjecting) {
        mState = mDiscSwap.PollEject();
    }
    if (mState == DiscSwap::kStepTrayOpen && mMsgScreenExited == 0) {
        ExitScreenByName(HxStr(kMsgScreen));
        mMsgScreenExited = 1;
        return;
    }
    if (mLoadShown == 0 && mState == DiscSwap::kStepTrayOpen && mMsgScreenExited != 0) {
        mState = mDiscSwap.CheckDisc();
        if (mState == DiscSwap::kStepDiscReady) {
            std::vector<HxStr> buttons;
            MetMsgScreen::Show(HxStr(kLoadMessage),
                               HxStr(kMessageTitle),
                               ConfigText(kPromptConfigCode, kLoadMessage),
                               kNoButtons,
                               buttons,
                               this);
        }
        return;
    }
    if (mLoadShown != 0 && mState == DiscSwap::kStepTrayOpen) {
        mDiscSwap.BeginInsert();
        mState = mDiscSwap.PollInsert();
        return;
    }
    if (mState == DiscSwap::kStepClosing) {
        mState = mDiscSwap.PollInsert();
    }
    if (mState == DiscSwap::kStepCloseFailed) {
        mState = mDiscSwap.MountDisc();
    }
    if (mState == DiscSwap::kStepDiscReady) {
        mState = mDiscSwap.MountDisc();
    }
    if ((mState == kStateRetryMount || mState == DiscSwap::kStepNotReady) && mLoadShown != 0) {
        mState = mDiscSwap.MountDisc();
        return;
    }
    if (mState >= DiscSwap::kStepBadDisc && mState <= kStateLastFailure) {
        mLoadShown = 0;
        mPrepareShown = 1;
        mRetry = 1;
        mMsgScreenExited = 0;
        mState = kStateIdle;
        std::vector<HxStr> buttons;
        MetMsgScreen::Show(HxStr(kPrepareMessage),
                           HxStr(kMessageTitle),
                           ConfigText(kPromptConfigCode, kPrepareMessage),
                           kNoButtons,
                           buttons,
                           this);
    }
    if (mState != DiscSwap::kStepMounted) {
        return;
    }

    RebuildStageLists();
    RebuildArenaLists();
    mFinished = 1;

    std::vector<MetPersonaData *> personas(*MetFreqMakerAssetManager::shared()->GetIdentityList());
    MergeLevelLists(personas);
    personas = *MetPersonaData::savedList();
    MergeLevelLists(personas);
    personas = *MetPersonaData::loadList();
    MergeLevelLists(personas);

    HxStr before = ConfigText(kPromptConfigCode, kDoneTextBefore);
    HxStr title = QueryConfigString(kExpansionTitleConfigCode);
    HxStr after = ConfigText(kPromptConfigCode, kDoneTextAfter);
    HxStr text(
        FormatString(kDoneTextFormat, TextOrEmpty(before), TextOrEmpty(title), TextOrEmpty(after)));
    std::vector<HxStr> buttons;
    buttons.push_back(HxStr(kContinueButton));
    MetMsgScreen::ShowActive(
        HxStr(kDoneMessage), HxStr(kMessageTitle), text, kOneButton, buttons, this);
}

// 0x002193e8
void MetExpansionPakScreen::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (name == kPrepareMessage) {
        std::vector<HxStr> buttons;
        if (mRetry == 0) {
            buttons.push_back(HxStr(kContinueButton));
            buttons.push_back(HxStr(kCancelButton));
            MetMsgScreen::Show(HxStr(kCheckMessage),
                               HxStr(kMessageTitle),
                               ConfigText(kPromptConfigCode, kCheckMessage),
                               kTwoButtons,
                               buttons,
                               this);
        } else {
            buttons.push_back(HxStr(kRetryButton));
            buttons.push_back(HxStr(kCancelButton));
            MetMsgScreen::Show(HxStr(kCheckMessage),
                               HxStr(kMessageTitle),
                               ConfigText(kPromptConfigCode, kRetryText),
                               kTwoButtons,
                               buttons,
                               this);
        }
        return;
    }

    if (name == kCheckMessage) {
        mLoadShown = 0;
        // Yes, the binary shows the same dialogue on both branches.
        if (nChoice == kChoiceFirst) {
            std::vector<HxStr> buttons;
            MetMsgScreen::Show(HxStr(kLoadMessage),
                               HxStr(kMessageTitle),
                               ConfigText(kPromptConfigCode, kLoadMessage),
                               kNoButtons,
                               buttons,
                               this);
        } else {
            std::vector<HxStr> buttons;
            MetMsgScreen::Show(HxStr(kLoadMessage),
                               HxStr(kMessageTitle),
                               ConfigText(kPromptConfigCode, kLoadMessage),
                               kNoButtons,
                               buttons,
                               this);
        }
        return;
    }

    if (name == kDoneMessage) {
        mExitChoice = kExitDone;
        BeginExit();
        return;
    }

    mExitChoice = kExitCancelled;
    BeginExit();
}

// 0x00219e80
void MetExpansionPakScreen::OnFadeInDone() {
    std::vector<HxStr> buttons;
    MetMsgScreen::Show(HxStr(kPrepareMessage),
                       HxStr(kMessageTitle),
                       ConfigText(kPromptConfigCode, kPrepareMessage),
                       kNoButtons,
                       buttons,
                       this);
}

// 0x0021a128
void MetExpansionPakScreen::OnFadeOutDone() {
    mRenderer->RemoveScreen(this);
    if (mExitChoice == kExitCancelled) {
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kRightGizmoScreen));
        PushNamedScreen(HxStr(kOptionsButtonsScreen));
        ActivateNamedPanel(HxStr(kOptionsButtonsScreen));
    } else {
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kLeftGizmoSmallScreen));
        PushNamedScreen(HxStr(kTopLogoScreen));
        PushNamedScreen(HxStr(kMainScreen));
        ActivateNamedPanel(HxStr(kMainScreen));
    }
}

// 0x0021d820
MetExpansionPakScreen *MetExpansionPakScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetExpansionPakScreen(pRenderer, nPriority);
}

// 0x0021d8a8
MetExpansionPakScreen::~MetExpansionPakScreen() {
    delete mFade;
}

// 0x0021d928
void MetExpansionPakScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
}

// 0x0021d948
void MetExpansionPakScreen::EnterAndShow() {
    mState = kStateIdle;
    mDiscSwap.Reset();
    mPrepareShown = 0;
    mLoadShown = 0;
    mFinished = 0;
    mRetry = 0;
    mMsgScreenExited = 0;
    mFade->FadeIn(kFadeDuration, mRenderer->mAnimationFrame, this, kRetainView);
}

// 0x0021d9a8
void MetExpansionPakScreen::BeginExit() {
    mFade->FadeOut(kFadeDuration, mRenderer->mAnimationFrame, this, kReleaseView);
}

// 0x0021d9e0
void MetExpansionPakScreen::OnMsgScreenShown(const HxStr &name) {
    if (name == kPrepareMessage) {
        mPrepareShown = 1;
        return;
    }

    if (name == kLoadMessage) {
        mLoadShown = 1;
    }
}
