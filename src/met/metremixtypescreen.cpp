#include "met/metremixtypescreen.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/manager.h"
#include "rnd/transanim.h"
#include "rnd/view.h"
#include "script/configquery.h"
#include "script/scripthost.h"

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

// What MetScreen::mUnknown18 records for the exit hook to act on.
constexpr int kExitBack = 0;
constexpr int kExitToButtonAction = 2;

// The selection alternation the select command starts.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

} // namespace

// 0x00361d18
MetRemixTypeScreen::MetRemixTypeScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknown8c(nullptr) {
    mUnknown38.push_back(HxStr(kNewObjectName));
    mUnknown38.push_back(HxStr(kLoadObjectName));
    mUnknown38.push_back(HxStr(kJukeboxObjectName));
    mUnknown8c = new MetButtonList;
}

// 0x003696c0
MetRemixTypeScreen::~MetRemixTypeScreen() {
    delete mUnknown8c;
}

// 0x00369638
MetRemixTypeScreen *MetRemixTypeScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetRemixTypeScreen(pRenderer, nPriority);
}

// 0x00362600
void MetRemixTypeScreen::EnterAndShow() {
    SetShowing(0);
    if (MetFrontEndState::shared()->mUnknown18 != 0) {
        MetFrontEndState *pState = MetFrontEndState::shared();
        pState->mUnknown1c = pState->mUnknown18;
        pState->mUnknown18 = 0;
        MetHelpScreen::SelectPreset(HxStr(kStandardTitlePreset));
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kHelpScreen));
        mUnknown10->SetActivePanel(this);
        GameParams params(*Application::shared()->GetGameManager()->GetParams());
        params.mLoadingGame = 0;
        Application::shared()->GetGameManager()->SetParams(params);
        CallScriptTemplate(kTransitionTemplate, kTransitionArgument);
    }

    HxStr mode;
    if (Application::shared()->GetGameMode() == kGameModeSolo) {
        {
            HxStr key;
            QueryConfigString(&key, kTitleConfigCode, kSoloTitleKey);
            mode = key;
        }
        mThreeButtonView->SetShowing(1);
        mTwoButtonView->SetShowing(0);
        mUnknown30->ReleaseAnimsRefs();
        mUnknown30->AddAnim(mThreeButtonAnim);
        mUnknown30->SetFrame(mUnknown04);
        mUnknown8c->Clear();
        {
            HxStr objectName(kNewButton);
            HxStr label;
            QueryConfigString(&label, kPromptConfigCode, kNewPrompt);
            mUnknown8c->Add(objectName, label);
        }
        {
            HxStr objectName(kLoadButton);
            HxStr label;
            QueryConfigString(&label, kPromptConfigCode, kLoadPrompt);
            mUnknown8c->Add(objectName, label);
        }
        {
            HxStr objectName(kJukeboxButton);
            HxStr label;
            QueryConfigString(&label, kPromptConfigCode, kJukeboxPrompt);
            mUnknown8c->Add(objectName, label);
        }
    } else {
        {
            HxStr key;
            QueryConfigString(&key, kTitleConfigCode, kMultiTitleKey);
            mode = key;
        }
        mTwoButtonView->SetShowing(1);
        mThreeButtonView->SetShowing(0);
        mUnknown30->ReleaseAnimsRefs();
        mUnknown30->AddAnim(mTwoButtonAnim);
        mUnknown30->SetFrame(mUnknown04);
        mUnknown8c->Clear();
        {
            HxStr objectName(kNewButton);
            HxStr label;
            QueryConfigString(&label, kPromptConfigCode, kNewPrompt);
            mUnknown8c->Add(objectName, label);
        }
        {
            HxStr objectName(kLoadButton);
            HxStr label;
            QueryConfigString(&label, kPromptConfigCode, kLoadPrompt);
            mUnknown8c->Add(objectName, label);
        }
    }

    mUnknown14->UpdateWorldXfm(nullptr, 1); // Yes, the binary discards the result.
    mUnknown8c->SetSelected(kFirstButtonIndex);
    {
        HxStr body;
        QueryConfigString(&body, kTitleConfigCode, kTitleKey);
        MetScreenTitleScreen::SetTitle(mode + body);
    }
    MetHelpScreen::SelectPreset(HxStr(kStandardTitlePreset));
    MetHelpScreen::SetText(mUnknown38[mUnknown8c->mSelected], mUnknown10->mUnknown68);
    MetScreen::EnterAndShow();
}

// 0x00362348
void MetRemixTypeScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mUnknown8c->OnUnknownSlot2();
        MetHelpScreen::SetText(mUnknown38[mUnknown8c->mSelected], mUnknown10->mUnknown68);
        break;

    case kMetScreenCommandNext:
        mUnknown8c->OnUnknownSlot3();
        MetHelpScreen::SetText(mUnknown38[mUnknown8c->mSelected], mUnknown10->mUnknown68);
        break;

    case kMetScreenCommandSelect:
        ActivateNamedPanel(HxStr(kNoName));
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        StartRepeatingSound(mUnknown10->mUnknown68,
                            kSelectAlternateInterval,
                            mUnknown8c->mUnknown00,
                            kSelectAlternateCycles);
        break;

    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        mUnknown18 = kExitBack;
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// 0x00362fa0
void MetRemixTypeScreen::OnUnknownSlot30([[maybe_unused]] Rnd::Object *pObject) {
    mUnknown18 = kExitToButtonAction;
    ExitScreenByName(HxStr(kLeftGizmoScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    if (mUnknown8c->mSelected != kFirstButtonIndex) {
        ExitScreenByName(HxStr(kHelpScreen));
    }
    BeginExit();
}

// 0x00362098
void MetRemixTypeScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mTwoButtonView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kTwoButtonView)));
    mThreeButtonView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kThreeButtonView)));
    mTwoButtonAnim = dynamic_cast<Rnd::TransAnim *>(Rnd::g_manager.Find(HxStr(kTwoButtonAnim)));
    mThreeButtonAnim = dynamic_cast<Rnd::TransAnim *>(Rnd::g_manager.Find(HxStr(kThreeButtonAnim)));
}
