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
      mUnknown8c(nullptr) {
    mUnknown60 = 0;
}

// 0x00211ae8
void MetCreditsScreen::ResolveContainerViews() {
    ResolveAnimationViews();
    mUnknown14 = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kViewName)));
    mUnknown14->ReleaseAnimsRefs(); // Yes, the binary does not test the view for null.
    mUnknown48 = 0;

    mUnknown90 = dynamic_cast<Rnd::TransAnim *>(Rnd::g_manager.Find(HxStr(kAnimationName)));
    mUnknown98 = mUnknown90->EndFrame();
    mUnknown90->SetFrame(0.0f);

    Rnd::Cam *pCam = dynamic_cast<Rnd::Cam *>(Rnd::g_manager.Find(HxStr(kCameraName)));
    mUnknown8c = new CreditsRoll(HxStr(kPicturePrefix), HxStr(kTextPrefix), pCam, kFirstCredit);
    mUnknown8c->Build();
}

// 0x00211e40
void MetCreditsScreen::OnUnknownSlot36() {
    mUnknown8c->HideAll();
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
    delete mUnknown8c;
}

// 0x00214e70
void MetCreditsScreen::EnterAndShow() {
    mUnknown8c->Reset();
    SetShowing(1);
    mUnknown08 = 0.0f;
    mUnknown1c = 1;
    mUnknown94 = mUnknown10->mUnknown68;
    OnUnknownSlot33();
}

// 0x00214ee0
void MetCreditsScreen::OnUnknownSlot26(float flTime) {
    const float flFrame = flTime - mUnknown94;
    mUnknown90->SetFrame(flFrame);
    if (mUnknown98 + kExitDelayFrames < flFrame) {
        BeginExit();
    } else {
        mUnknown8c->Update(); // Yes, the binary discards this call's result.
    }
}

// 0x00214f68
void MetCreditsScreen::HandleCommand(const MetScreenCommand *pCommand) {
    if (pCommand->mCommand == kMetScreenCommandBack) {
        BeginExit();
    }
}
