#include "met/metcreditsscreen.h"

#include "met/metrenderer.h"
#include "os/hxstr.h"
#include "rnd/cam.h"
#include "rnd/manager.h"
#include "rnd/transanim.h"
#include "rnd/view.h"

namespace {

static const char *const kScreenName = "cred";
static const char *const kDirectory = "metagame/Shared";
static const char *const kContainerName = "credit";

static const char *const kViewName = "credit.view";
static const char *const kAnimationName = "Group_credit.tnm";
static const char *const kCameraName = "credit_cam.cam";
static const char *const kPicturePrefix = "cpic_";
static const char *const kTextPrefix = "ctxt_";
constexpr int kFirstCredit = 1;

// How far past the animation's end frame the screen stays before exiting.
constexpr float kExitDelayFrames = 100.0f;

static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kRightGizmoScreen = "MetRightGizmoScreen";
static const char *const kOptionsButtonsScreen = "MetConfigOptionsButtonsScreen";

} // namespace

// 0x00211970
MetCreditsScreen::MetCreditsScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mCreditsRoll(nullptr) {
    mShowsLoadedDrawables = 0;
}

// 0x00211ae8
void MetCreditsScreen::ResolveContainerViews() {
    ResolveAnimationViews();
    mView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kViewName)));
    mView->ReleaseAnimsRefs(); // Yes, the binary does not test the view for null.
    mViewsUnresolved = 0;

    mAnimation = dynamic_cast<Rnd::TransAnim *>(Rnd::g_manager.Find(HxStr(kAnimationName)));
    mEndFrame = mAnimation->EndFrame();
    mAnimation->SetFrame(0.0f);

    Rnd::Cam *pCam = dynamic_cast<Rnd::Cam *>(Rnd::g_manager.Find(HxStr(kCameraName)));
    mCreditsRoll = new CreditsRoll(HxStr(kPicturePrefix), HxStr(kTextPrefix), pCam, kFirstCredit);
    mCreditsRoll->Build();
}

// 0x00211e40
void MetCreditsScreen::OnExitFinished() {
    mCreditsRoll->HideAll();
    PushNamedScreen(HxStr(kHelpScreen));
    PushNamedScreen(HxStr(kRightGizmoScreen));
    PushNamedScreen(HxStr(kOptionsButtonsScreen));
    ActivateNamedPanel(HxStr(kOptionsButtonsScreen));
}

// 0x00214d58
void MetCreditsScreen::PlaySlideSound([[maybe_unused]] int nSelector) {
}

// 0x00214d60
void MetCreditsScreen::PlayHighSound([[maybe_unused]] int nSelector) {
}

// 0x00214d68
void MetCreditsScreen::PlayCycleLeftSound([[maybe_unused]] int nSelector) {
}

// 0x00214d70
void MetCreditsScreen::PlayCycleRightSound([[maybe_unused]] int nSelector) {
}

// 0x00214d78
MetCreditsScreen *MetCreditsScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetCreditsScreen(pRenderer, nPriority);
}

// 0x00214e00
MetCreditsScreen::~MetCreditsScreen() {
    delete mCreditsRoll;
}

// 0x00214e70
void MetCreditsScreen::EnterAndShow() {
    mCreditsRoll->Reset();
    SetShowing(1);
    mEnterStartTime = 0.0f;
    mAcceptsCommands = 1;
    mStartFrame = mRenderer->mAnimationFrame;
    OnEnterFinished();
}

// 0x00214ee0
void MetCreditsScreen::UpdateIdle(float flTime) {
    const float flFrame = flTime - mStartFrame;
    mAnimation->SetFrame(flFrame);
    if (mEndFrame + kExitDelayFrames < flFrame) {
        BeginExit();
    } else {
        mCreditsRoll->Update(); // Yes, the binary discards this call's result.
    }
}

// 0x00214f68
void MetCreditsScreen::HandleCommand(const MetScreenCommand *pCommand) {
    if (pCommand->mCommand == kMetScreenCommandBack) {
        BeginExit();
    }
}
