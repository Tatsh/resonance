#include "met/metremixtypescreen.h"

#include <vector>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "game/globalsettings.h"
#include "memcard/memcardconnectstate.h"
#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metmsgscreen.h"
#include "met/metremixmanager.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "met/metsonglists.h"
#include "met/metstrings.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/manager.h"
#include "rnd/transanim.h"
#include "rnd/view.h"
#include "script/configquery.h"
#include "script/scripthost.h"

#ifdef VIDEO_STANDARD_PAL
#include <libscf.h>

#include "os/hostmode.h"
#endif

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "smrt";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "sm_remixtype";

// The three container objects the screen registers, in the order the constructor pushes them.
static const char *const kNewObjectName = "smrt_new";
static const char *const kLoadObjectName = "smrt_load";
static const char *const kJukeboxObjectName = "smrt_jukebox";

// The two button layouts and their animations.
static const char *const kTwoButtonView = "smrt_2but.view";
static const char *const kThreeButtonView = "smrt_3but.view";
static const char *const kTwoButtonAnim = "smrt_2but.tnm";
static const char *const kThreeButtonAnim = "smrt_3but.tnm";

// The three buttons and the prompts their labels are read under.
static const char *const kNewButton = "smrt_01.but";
static const char *const kLoadButton = "smrt_02.but";
static const char *const kJukeboxButton = "smrt_03.but";
static const char *const kNewPrompt = "ms_newjam";
static const char *const kLoadPrompt = "ms_loadjam";
static const char *const kJukeboxPrompt = "ms_jukebox";

// The keys of the title's mode prefix and of its body.
static const char *const kSoloTitleKey = "solo";
static const char *const kMultiTitleKey = "multi";
static const char *const kTitleKey = "remix_type";

// The help preset EnterAndShow() selects.
static const char *const kStandardTitlePreset = "standard_title";

static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kNoName = "";

// Configuration codes the button labels and the screen title are read under.
constexpr int kPromptConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x269;

// The script template EnterAndShow() runs on a pending transition, and its one argument.
constexpr int kTransitionTemplate = 0x267;
static const char *const kTransitionArgument = "0";

// Index EnterAndShow() selects, which is the first button.
constexpr int kFirstButtonIndex = 0;

// The three buttons in the order EnterAndShow() adds them.
constexpr int kNewButtonIndex = 0;
constexpr int kLoadButtonIndex = 1;
constexpr int kJukeboxButtonIndex = 2;

// Screens the selected button leads to, and the screens ListRemixes() returns to.
static const char *const kModeScreen = "MetModeScreen";
static const char *const kSoloStagesScreen = "MetSoloStagesScreen";
static const char *const kRemixTypeScreen = "MetRemixTypeScreen";
static const char *const kRemixLoadScreen = "MetRemixLoadScreen";
static const char *const kRemixDataScreen = "MetRemixDataScreen";
static const char *const kJukeboxTopButtonsScreen = "MetJukeboxTopButtonsScreen";
constexpr int kLoadReturnScreenCount = 4;
constexpr int kJukeboxReturnScreenCount = 2;

// The disc entry ListRemixes() lists after the memory card.
static const char *const kDiscSlotName = "disc";
constexpr int kDiscSlot = -1;

// The low-space warning, its buttons, and the choice that backs out of it.
static const char *const kNoSpaceDialogue = "warn_remix_no_space";
static const char *const kWarningTitle = "WARNING";
static const char *const kBackButton = "BACK";
static const char *const kContinueButton = "CONTINUE";
constexpr int kTwoButtons = 2;
constexpr int kChoiceBack = 0;

// Values of the ListRemixes() playlist flag.
constexpr int kSkipPlayList = 0;
constexpr int kLoadPlayList = 1;

// Adds the first memory-card slot when a card is in use.
inline void AddCardSlot(std::vector<MemcardConnectState> &slots) {
    if (MetFrontEndState::shared()->mUsingMemcard != 0) {
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        slots.push_back(GlobalSettings::shared()->mCardSlots[0]);
    }
}

// What MetScreen::mExitChoice records for the exit hook to act on.
constexpr int kExitBack = 0;
constexpr int kExitToButtonAction = 2;

// The selection alternation the select command starts.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

#ifdef VIDEO_STANDARD_PAL
inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}
#endif

} // namespace

// NTSC-U/C: 0x00361d18, PAL: 0x0038f220
MetRemixTypeScreen::MetRemixTypeScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mButtons(nullptr) {
    mHelpKeys.push_back(MetText(kMetStrHSmrtNew, kNewObjectName));
    mHelpKeys.push_back(MetText(kMetStrHSmrtLoad, kLoadObjectName));
    mHelpKeys.push_back(MetText(kMetStrHSmrtJukebox, kJukeboxObjectName));
    mButtons = new MetButtonList;
}

// 0x003696c0
MetRemixTypeScreen::~MetRemixTypeScreen() {
    delete mButtons;
}

// 0x00369638
MetRemixTypeScreen *MetRemixTypeScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetRemixTypeScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x00362600, PAL: 0x0038fcb0
void MetRemixTypeScreen::EnterAndShow() {
    SetShowing(0);
    if (MetFrontEndState::shared()->mPendingTransition != 0) {
        MetFrontEndState *pState = MetFrontEndState::shared();
        pState->mLastTransition = pState->mPendingTransition;
        pState->mPendingTransition = 0;
        MetHelpScreen::SelectPreset(MetText(kMetStrHStandardTitle, kStandardTitlePreset));
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kHelpScreen));
        mRenderer->SetActivePanel(this);
        GameParams params(*Application::shared()->GetGameManager()->GetParams());
        params.mLoadingGame = 0;
        Application::shared()->GetGameManager()->SetParams(params);
        CallScriptTemplate(kTransitionTemplate, kTransitionArgument);
    }

    HxStr mode;
    if (Application::shared()->GetGameMode() == kGameModeSolo) {
        {
            HxStr key = MetConfigText(kMetStrTSolo, kTitleConfigCode, kSoloTitleKey);
            mode = key;
        }
        mThreeButtonView->SetShowing(1);
        mTwoButtonView->SetShowing(0);
        mEnterAnim->ReleaseAnimsRefs();
        mEnterAnim->AddAnim(mThreeButtonAnim);
        mEnterAnim->SetFrame(mAnimEndFrame);
        mButtons->Clear();
        {
            HxStr objectName(kNewButton);
            HxStr label = MetConfigText(kMetStrMsNewjam, kPromptConfigCode, kNewPrompt);
            mButtons->Add(objectName, label);
        }
        {
            HxStr objectName(kLoadButton);
            HxStr label = MetConfigText(kMetStrMsLoadjam, kPromptConfigCode, kLoadPrompt);
            mButtons->Add(objectName, label);
        }
        {
            HxStr objectName(kJukeboxButton);
            HxStr label = MetConfigText(kMetStrMsJukebox, kPromptConfigCode, kJukeboxPrompt);
            mButtons->Add(objectName, label);
        }
    } else {
        {
            HxStr key = MetConfigText(kMetStrTMulti, kTitleConfigCode, kMultiTitleKey);
            mode = key;
        }
        mTwoButtonView->SetShowing(1);
        mThreeButtonView->SetShowing(0);
        mEnterAnim->ReleaseAnimsRefs();
        mEnterAnim->AddAnim(mTwoButtonAnim);
        mEnterAnim->SetFrame(mAnimEndFrame);
        mButtons->Clear();
        {
            HxStr objectName(kNewButton);
            HxStr label = MetConfigText(kMetStrMsNewjam, kPromptConfigCode, kNewPrompt);
            mButtons->Add(objectName, label);
        }
        {
            HxStr objectName(kLoadButton);
            HxStr label = MetConfigText(kMetStrMsLoadjam, kPromptConfigCode, kLoadPrompt);
            mButtons->Add(objectName, label);
        }
    }

    mView->UpdateWorldXfm(nullptr, 1); // Yes, the binary discards the result.
    mButtons->SetSelected(kFirstButtonIndex);
    {
        HxStr body = MetConfigText(kMetStrTRemixType, kTitleConfigCode, kTitleKey);
        MetScreenTitleScreen::SetTitle(mode + body);
    }
    MetHelpScreen::SelectPreset(MetText(kMetStrHStandardTitle, kStandardTitlePreset));
    MetHelpScreen::SetText(mHelpKeys[mButtons->mSelected], mRenderer->mAnimationFrame);
    MetScreen::EnterAndShow();
}

// 0x00364478
void MetRemixTypeScreen::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (!(name == kNoSpaceDialogue)) {
        return;
    }
    if (nChoice == kChoiceBack) {
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kRemixTypeScreen));
        ActivateNamedPanel(HxStr(kRemixTypeScreen));
    } else {
        OpenSelectedButton();
    }
}

// 0x00362348
void MetRemixTypeScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mButtons->SelectPrevious();
        MetHelpScreen::SetText(mHelpKeys[mButtons->mSelected], mRenderer->mAnimationFrame);
        break;

    case kMetScreenCommandNext:
        mButtons->SelectNext();
        MetHelpScreen::SetText(mHelpKeys[mButtons->mSelected], mRenderer->mAnimationFrame);
        break;

    case kMetScreenCommandSelect:
        ActivateNamedPanel(HxStr(kNoName));
        MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
        StartRepeatingSound(mRenderer->mAnimationFrame,
                            kSelectAlternateInterval,
                            mButtons->mSelectedButton,
                            kSelectAlternateCycles);
        break;

    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
        mExitChoice = kExitBack;
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// 0x00362fa0
void MetRemixTypeScreen::OnRepeatingSoundFinished([[maybe_unused]] Rnd::Button *pButton) {
    mExitChoice = kExitToButtonAction;
    ExitScreenByName(HxStr(kLeftGizmoScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    if (mButtons->mSelected != kFirstButtonIndex) {
        ExitScreenByName(HxStr(kHelpScreen));
    }
    BeginExit();
}

// NTSC-U/C: 0x00363920, PAL: 0x00391470
void MetRemixTypeScreen::OnExitFinished() {
    if (mExitChoice == kExitBack) {
        PushNamedScreen(HxStr(kModeScreen));
        ActivateNamedPanel(HxStr(kModeScreen));
        return;
    }

    switch (mButtons->mSelected) {
    case kNewButtonIndex:
    case kLoadButtonIndex:
        if (MetFrontEndState::shared()->mUsingMemcard != 0 &&
            Application::shared()->GetGameMode() == kGameModeSolo) {
#ifdef VIDEO_STANDARD_PAL
            const bool bLowSpace = !GlobalSettings::shared()->mCardSlots.empty() &&
                                   (GlobalSettings::shared()->mCardSlots[0].mFree <
                                    GlobalSettings::shared()->mMinimumFreeClusters);
#else
            GlobalSettings::shared(); // Yes, the binary discards this call's result.
            const bool bLowSpace = GlobalSettings::shared()->mCardSlots[0].mFree <
                                   GlobalSettings::shared()->mMinimumFreeClusters;
#endif
            if (bLowSpace) {
                std::vector<HxStr> buttons;
                buttons.push_back(MetText(kMetStrMsgBACK, kBackButton));
                buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
#ifdef VIDEO_STANDARD_PAL
                // The German text also includes the memory card name.
                const HxStr format = GetMetString(kMetStrWarnRemixNoSpace);
                HxStr text;
                if (GetLanguage() == SCE_GERMAN_LANGUAGE) {
                    const HxStr cardName = FirstCardSlotName();
                    text = FormatString(TextOrEmpty(format),
                                        TextOrEmpty(cardName),
                                        GlobalSettings::shared()->mMinimumFreeClusters);
                } else {
                    text = FormatString(TextOrEmpty(format),
                                        GlobalSettings::shared()->mMinimumFreeClusters);
                }
                const HxStr dialogue(kNoSpaceDialogue);
                const HxStr title = GetMetString(kMetStrMsgWARNING);
#else
                const HxStr dialogue(kNoSpaceDialogue);
                const HxStr title(kWarningTitle);
                HxStr text = QueryConfigString(kPromptConfigCode, kNoSpaceDialogue);
#endif
                MetMsgScreen::Show(dialogue, title, text, kTwoButtons, buttons, this);
                break;
            }
        }
        OpenSelectedButton();
        break;

    case kJukeboxButtonIndex: {
        std::vector<HxStr> screens;
        screens.resize(kJukeboxReturnScreenCount);
        screens[0] = kJukeboxTopButtonsScreen;
        screens[1] = kHelpScreen;
        std::vector<MemcardConnectState> slots;
        AddCardSlot(slots);
        MemcardConnectState disc;
        disc.mSlotName = kDiscSlotName;
        disc.mPortSlot = kDiscSlot;
        slots.push_back(disc);
        MetRemixManager::shared()->ListRemixes(screens, slots, kLoadPlayList);
        break;
    }

    default:
        break;
    }
}

// 0x00362098
void MetRemixTypeScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mTwoButtonView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kTwoButtonView)));
    mThreeButtonView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kThreeButtonView)));
    mTwoButtonAnim = dynamic_cast<Rnd::TransAnim *>(Rnd::g_manager.Find(HxStr(kTwoButtonAnim)));
    mThreeButtonAnim = dynamic_cast<Rnd::TransAnim *>(Rnd::g_manager.Find(HxStr(kThreeButtonAnim)));
}

// 0x00363148
void MetRemixTypeScreen::OpenSelectedButton() {
    switch (mButtons->mSelected) {
    case kNewButtonIndex:
        PushNamedScreen(HxStr(kSoloStagesScreen));
        ActivateNamedPanel(HxStr(kSoloStagesScreen));
        break;

    case kLoadButtonIndex: {
        std::vector<HxStr> screens;
        screens.resize(kLoadReturnScreenCount);
        screens[0] = kRemixLoadScreen;
        screens[1] = kTitleScreen;
        screens[2] = kRemixDataScreen;
        screens[3] = kHelpScreen;
        std::vector<MemcardConnectState> slots;
        AddCardSlot(slots);
        MemcardConnectState disc;
        disc.mSlotName = kDiscSlotName;
        disc.mPortSlot = kDiscSlot;
        slots.push_back(disc);
        MetRemixManager::shared()->ListRemixes(screens, slots, kSkipPlayList);
        break;
    }

    default:
        break;
    }
}
