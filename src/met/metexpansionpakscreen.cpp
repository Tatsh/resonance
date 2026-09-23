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

// The states mUnknowna8 takes beyond the DiscSwap::Step values.
constexpr int kStateIdle = 0;
constexpr int kStateReleased = 1;
constexpr int kStateRetryMount = 8;
constexpr int kStateLastFailure = 13;

// MetScreen::mUnknown18 on exit, read back by OnFadeOutDone().
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
void MetExpansionPakScreen::OnUnknownSlot26(float flTime) {
    mFade->Update(flTime);
    if (mUnknownb4 != 0) {
        return;
    }

    if (mUnknowna8 == kStateIdle && mUnknownac != 0) {
        mUnknowna8 = mUnknown90.ReleaseDisc();
    }
    if (mUnknowna8 == kStateReleased) {
        mUnknown90.BeginEject();
        mUnknowna8 = mUnknown90.PollEject();
        return;
    }
    if (mUnknowna8 == DiscSwap::kStepEjecting) {
        mUnknowna8 = mUnknown90.PollEject();
    }
    if (mUnknowna8 == DiscSwap::kStepTrayOpen && mUnknownbc == 0) {
        ExitScreenByName(HxStr(kMsgScreen));
        mUnknownbc = 1;
        return;
    }
    if (mUnknownb0 == 0 && mUnknowna8 == DiscSwap::kStepTrayOpen && mUnknownbc != 0) {
        mUnknowna8 = mUnknown90.CheckDisc();
        if (mUnknowna8 == DiscSwap::kStepDiscReady) {
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
    if (mUnknownb0 != 0 && mUnknowna8 == DiscSwap::kStepTrayOpen) {
        mUnknown90.BeginInsert();
        mUnknowna8 = mUnknown90.PollInsert();
        return;
    }
    if (mUnknowna8 == DiscSwap::kStepClosing) {
        mUnknowna8 = mUnknown90.PollInsert();
    }
    if (mUnknowna8 == DiscSwap::kStepCloseFailed) {
        mUnknowna8 = mUnknown90.MountDisc();
    }
    if (mUnknowna8 == DiscSwap::kStepDiscReady) {
        mUnknowna8 = mUnknown90.MountDisc();
    }
    if ((mUnknowna8 == kStateRetryMount || mUnknowna8 == DiscSwap::kStepNotReady) &&
        mUnknownb0 != 0) {
        mUnknowna8 = mUnknown90.MountDisc();
        return;
    }
    if (mUnknowna8 >= DiscSwap::kStepBadDisc && mUnknowna8 <= kStateLastFailure) {
        mUnknownb0 = 0;
        mUnknownac = 1;
        mUnknownb8 = 1;
        mUnknownbc = 0;
        mUnknowna8 = kStateIdle;
        std::vector<HxStr> buttons;
        MetMsgScreen::Show(HxStr(kPrepareMessage),
                           HxStr(kMessageTitle),
                           ConfigText(kPromptConfigCode, kPrepareMessage),
                           kNoButtons,
                           buttons,
                           this);
    }
    if (mUnknowna8 != DiscSwap::kStepMounted) {
        return;
    }

    RebuildStageLists();
    RebuildArenaLists();
    mUnknownb4 = 1;

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
        if (mUnknownb8 == 0) {
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
        mUnknownb0 = 0;
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
        mUnknown18 = kExitDone;
        BeginExit();
        return;
    }

    mUnknown18 = kExitCancelled;
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
    mUnknown10->RemoveScreen(this);
    if (mUnknown18 == kExitCancelled) {
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
    mUnknowna8 = kStateIdle;
    mUnknown90.Reset();
    mUnknownac = 0;
    mUnknownb0 = 0;
    mUnknownb4 = 0;
    mUnknownb8 = 0;
    mUnknownbc = 0;
    mFade->FadeIn(kFadeDuration, mUnknown10->mUnknown68, this, kRetainView);
}

// 0x0021d9a8
void MetExpansionPakScreen::BeginExit() {
    mFade->FadeOut(kFadeDuration, mUnknown10->mUnknown68, this, kReleaseView);
}

// 0x0021d9e0
void MetExpansionPakScreen::OnMsgScreenShown(const HxStr &name) {
    if (name == kPrepareMessage) {
        mUnknownac = 1;
        return;
    }

    if (name == kLoadMessage) {
        mUnknownb0 = 1;
    }
}
