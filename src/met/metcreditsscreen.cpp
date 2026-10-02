#include "met/metcreditsscreen.h"

#include "met/metrenderer.h"
#include "os/hxstr.h"
#include "rnd/cam.h"
#include "rnd/manager.h"
#include "rnd/transanim.h"
#include "rnd/view.h"

#ifdef ENABLE_PATCHES
#include <cmath>

#include "creditsavatar.h"
#endif

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

#ifdef ENABLE_PATCHES
// The roll's animation moves the view with this name.
static const char *const kGroupName = "Group_credit.view";
constexpr int kXfmRowTranslation = 3;
constexpr int kVectorComponents = 3;
#endif

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
#ifdef ENABLE_PATCHES
    AddLeadingCredit();
#endif
}

#ifdef ENABLE_PATCHES
void MetCreditsScreen::AddLeadingCredit() {
    mAnimationEndFrame = mEndFrame;
    mGroup = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kGroupName)));
    const float flDistance = mCreditsRoll->AddLeadingCredit(
        HxStr(RESONANCE_CREDITS_TEXT), kCreditsAvatarTexels, RESONANCE_CREDITS_AVATAR_SIZE);
    if (mGroup == nullptr || flDistance == 0.0f || mAnimationEndFrame <= 0.0f) {
        return;
    }

    // The animation moves the group at a steady speed. The extra distance adds frames at the same
    // speed.
    float afStart[kVectorComponents];
    float flTravelSquared = 0.0f;
    mAnimation->SetFrame(0.0f);
    for (int i = 0; i < kVectorComponents; ++i) {
        afStart[i] = mGroup->mLocalXfm[kXfmRowTranslation][i];
    }
    mAnimation->SetFrame(mAnimationEndFrame);
    for (int i = 0; i < kVectorComponents; ++i) {
        const float flTravel = mGroup->mLocalXfm[kXfmRowTranslation][i] - afStart[i];
        mGroupStep[i] = flTravel / mAnimationEndFrame;
        flTravelSquared += flTravel * flTravel;
    }
    mAnimation->SetFrame(0.0f);
    if (flTravelSquared == 0.0f) {
        return;
    }
    mEndFrame += flDistance / (std::sqrt(flTravelSquared) / mAnimationEndFrame);
}
#endif

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
#ifdef ENABLE_PATCHES
    // Past its last key the animation does not move the group. The group is moved here for the
    // frames the leading credit added.
    if (mGroup != nullptr && mAnimationEndFrame < flFrame) {
        const float flExtra = (flFrame < mEndFrame ? flFrame : mEndFrame) - mAnimationEndFrame;
        for (int i = 0; i < kVectorComponents; ++i) {
            mGroup->mLocalXfm[kXfmRowTranslation][i] += mGroupStep[i] * flExtra;
        }
        mGroup->mDirty = 1;
    }
#endif
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
