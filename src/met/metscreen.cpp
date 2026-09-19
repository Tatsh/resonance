#include "met/metscreen.h"

#include <map>

#include "app/playsound.h"
#include "met/metrenderer.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "os/mem.h"
#include "rnd/animatable.h"
#include "rnd/asyncloader.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/object.h"
#include "rnd/view.h"

namespace {

// The separator BeginContainerLoad() appends to the directory.
static const char *const kPathSeparator = "/";
// Appended to the container name to form the scene root's registry key.
static const char *const kViewSuffix = ".view";
// Appended to the container name to form the archive the loader requests.
static const char *const kContainerSuffix = ".rnd";
// The two animation view names, formatted from the screen name.
static const char *const kEnterAnimationFormat = "%s_EE.anim";
static const char *const kExitAnimationFormat = "%s_BF.anim";

// The six sounds the front end plays as the user navigates.
static const char *const kSlideSound = "SND_MET_SLIDE";
static const char *const kLeaveSound = "SND_MET_LEAVE";
static const char *const kCycleLeftSound = "SND_MET_CYCLE_L";
static const char *const kCycleRightSound = "SND_MET_CYCLE_R";
static const char *const kHighSound = "SND_MET_HIGH";
static const char *const kErrorSound = "SND_MET_ERROR";

} // namespace

MetScreen::MetScreen(MetRenderer *pRenderer,
                     int nPriority,
                     const HxStr &name,
                     const HxStr &directory,
                     const HxStr &file)
    : mUnknown08(0.0f), mUnknown0c(0.0f), mUnknown10(pRenderer), mUnknown14(nullptr), mUnknown18(2),
      mUnknown1c(0), mUnknown20(name), mUnknown30(nullptr), mUnknown34(nullptr), mUnknown48(1),
      mUnknown4c(0), mUnknown50(0), mUnknown54(0), mUnknown58(1.0f), mUnknown5c(1), mUnknown60(1),
      mUnknown64(0.0f), mUnknown68(nullptr), mUnknown6c(0), mUnknown78(0), mUnknown7c(0),
      mUnknown80(file), mUnknown88(nPriority) {
    mUnknown28 = file + kContainerSuffix;
    mUnknown10->AddSink(this);
    if (directory != "" && file != "") {
        // Yes, the binary calls the virtual directly rather than through slot 18, which is what a
        // virtual call from a constructor compiles to.
        MetScreen::BeginContainerLoad(directory, file);
    }
}

MetScreen::~MetScreen() {
    mUnknown10->RemoveScreen(this);
    OnDestroying();
    mUnknown10->RemoveSink(this);
}

std::map<HxStr, MetContainerLoad *> &MetScreen::ContainerLoaderMap() {
    static std::map<HxStr, MetContainerLoad *> theMap;
    return theMap;
}

std::map<HxStr, MetScreen *> &MetScreen::ScreenRegistry() {
    static std::map<HxStr, MetScreen *> theMap;
    return theMap;
}

MetScreen *MetScreen::FindScreenByName(const HxStr &name) {
    MetScreen *pScreen = nullptr;
    std::map<HxStr, MetScreen *>::iterator it = ScreenRegistry().find(name);
    if (it != ScreenRegistry().end()) {
        pScreen = (*it).second;
    }
    return pScreen;
}

void MetScreen::BeginContainerLoad(const HxStr &directory, const HxStr &file) {
    HxStr dir = directory + kPathSeparator;
    if (ContainerLoaderMap()[mUnknown28] == nullptr) {
        MetContainerLoad *pLoad = new MetContainerLoad;
        pLoad->mLoader = new RndAsyncLoader(dir, mUnknown28, mUnknown88);
        pLoad->mUnknown08 = 1;
        pLoad->mUnknown04 = 0;
        ContainerLoaderMap()[mUnknown28] = pLoad;
    }
    ContainerLoaderMap()[mUnknown28]->mUnknown04 = 0;
    // Yes, the binary sets mUnknown08 and then immediately tests it, so the branch is always taken.
    ContainerLoaderMap()[mUnknown28]->mUnknown08 = 1;
    if (ContainerLoaderMap()[mUnknown28]->mUnknown08 != 0) {
        ContainerLoaderMap()[mUnknown28]->mUnknown08 = 0;
        ContainerLoaderMap()[mUnknown28]->mLoader->Enqueue();
    }
}

int MetScreen::PollContainerLoad() {
    MetContainerLoad *pLoad = ContainerLoaderMap()[mUnknown28];
    if (pLoad->mLoader == nullptr) {
        return 0;
    }
    float flProgress;
    if (pLoad->mLoader->Poll(&flProgress) != 1) {
        return 0;
    }
    if (mUnknown48 != 0) {
        ResolveContainerViews();
    }
    return 1;
}

void MetScreen::ResolveAnimationViews() {
    {
        HxStr name(FormatString(kEnterAnimationFormat,
                                mUnknown20.mStr != nullptr ? mUnknown20.mStr : g_szEmptyString));
        Rnd::Object *pObject = Rnd::g_manager.Find(name);
        mUnknown30 = pObject != nullptr ? dynamic_cast<Rnd::View *>(pObject) : nullptr;
    }
    {
        HxStr name(FormatString(kExitAnimationFormat,
                                mUnknown20.mStr != nullptr ? mUnknown20.mStr : g_szEmptyString));
        Rnd::Object *pObject = Rnd::g_manager.Find(name);
        mUnknown34 = pObject != nullptr ? dynamic_cast<Rnd::View *>(pObject) : nullptr;
    }
    mUnknown04 = mUnknown30 != nullptr ? mUnknown30->EndFrame() : 0.0f;
}

void MetScreen::ResolveContainerViews() {
    ResolveAnimationViews();
    HxStr name = mUnknown80 + kViewSuffix;
    Rnd::Object *pObject = Rnd::g_manager.Find(name);
    mUnknown14 = pObject != nullptr ? dynamic_cast<Rnd::View *>(pObject) : nullptr;
    if (mUnknown14 != nullptr) {
        mUnknown14->ReleaseAnimsRefs();
    } else {
        LogPrintf(" the screen %s doesn't have a valid view!\n",
                  mUnknown80.mStr != nullptr ? mUnknown80.mStr : g_szEmptyString);
    }
    SetShowing(0);
    mUnknown48 = 0;
}

void MetScreen::PushNamedScreen(const HxStr &name) {
    MetScreen *pScreen = FindScreenByName(name);
    mUnknown10->AddScreen(pScreen);
    if (pScreen->PollContainerLoad() != 0) {
        mUnknown10->AddScreenView(pScreen->mUnknown14);
        pScreen->EnterAndShow();
    } else {
        pScreen->mUnknown4c = 1;
    }
}

void MetScreen::EnterAndShow() {
    SetShowing(1);
    StartEnterAnimation(mUnknown10->mUnknown68);
}

void MetScreen::ActivateNamedPanel(const HxStr &name) {
    if (name == "") {
        mUnknown10->mUnknown80 = 0;
        return;
    }
    MetScreen *pScreen = FindScreenByName(name);
    if (pScreen->PollContainerLoad() != 0) {
        mUnknown10->SetActivePanel(pScreen);
        mUnknown10->mUnknown80 = 1;
        pScreen->OnUnknownSlot7();
    } else {
        pScreen->mUnknown50 = 1;
    }
}

void MetScreen::ExitScreenByName(const HxStr &name) {
    MemLogWrite(
        FormatString("Exiting screen: %s\n", name.mStr != nullptr ? name.mStr : g_szEmptyString));
    // The binary neither checks the result nor recovers from a key nothing registered under.
    FindScreenByName(name)->BeginExit();
}

void MetScreen::BeginExit() {
    StartExitAnimation(mUnknown10->mUnknown68);
}

void MetScreen::StartEnterAnimation(float flTime) {
    mUnknown08 = flTime;
    mUnknown0c = 0.0f;
    if (mUnknown30 != nullptr) {
        mUnknown30->SetFrame(mUnknown04);
    }
}

void MetScreen::UpdateEnterAnimation(float flTime) {
    if (mUnknown7c != 0) {
        mUnknown7c = 0;
        mUnknown1c = 1;
        mUnknown08 = 0.0f;
        OnUnknownSlot33();
    }
    if (mUnknown08 == 0.0f) {
        return;
    }
    if (mUnknown30 != nullptr) {
        mUnknown30->SetFrame(mUnknown08 + mUnknown04 - flTime);
    }
    if (mUnknown08 + mUnknown04 < flTime) {
        mUnknown7c = 1;
    }
}

void MetScreen::OnUnknownSlot7() {
}

void MetScreen::OnUnknownSlot10() {
}

void MetScreen::OnKeyboardDismissed() {
}

void MetScreen::OnDrawPass() {
}

void MetScreen::OnDestroying() {
}

void MetScreen::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
}

void MetScreen::OnMsgScreenShown(const HxStr &name) {
}

void MetScreen::HandleCommand(const MetScreenCommand *pCommand) {
}

void MetScreen::OnUnknownSlot26(float flTime) {
}

void MetScreen::UpdateIdleAnimation(float flTime) {
}

void MetScreen::OnUnknownSlot30(Rnd::Object *pObject) {
}

void MetScreen::OnUnknownSlot33() {
}

void MetScreen::OnUnknownSlot36() {
}

void MetScreen::HandleMessage(Message *pMsg) {
}

void MetScreen::PlaySlideSound(int nSelector) {
    PlaySoundByName(kSlideSound);
}

void MetScreen::PlayLeaveSound() {
    PlaySoundByName(kLeaveSound);
}

void MetScreen::PlayHighSound(int nSelector) {
    PlaySoundByName(kHighSound);
}

void MetScreen::PlayCycleLeftSound(int nSelector) {
    PlaySoundByName(kCycleLeftSound);
}

void MetScreen::PlayCycleRightSound(int nSelector) {
    PlaySoundByName(kCycleRightSound);
}

void MetScreen::PlayErrorSound(int nSelector) {
    PlaySoundByName(kErrorSound);
}

void MetScreen::DeliverCommand(const MetScreenCommand *pCommand) {
    if (mUnknown1c == 0) {
        return;
    }
    if (mUnknown5c != 0) {
        switch (pCommand->mCommand) {
        case kMetScreenCommandPrevious:
        case kMetScreenCommandNext:
            PlayHighSound(pCommand->mPadIndex);
            break;
        case kMetScreenCommandLeft:
            PlayCycleLeftSound(pCommand->mPadIndex);
            break;
        case kMetScreenCommandRight:
            PlayCycleRightSound(pCommand->mPadIndex);
            break;
        case kMetScreenCommandSelect:
            PlaySlideSound(pCommand->mPadIndex);
            break;
        case kMetScreenCommandBack:
            // Yes, the binary loads the controller index into a1 here as it does for the other
            // five, and this declaration accepts none.
            PlayLeaveSound();
            break;
        default:
            break;
        }
    }
    HandleCommand(pCommand);
}

void MetScreen::Draw() {
    mUnknown14->Drawable::Draw();
}

void MetScreen::StartExitAnimation(float flTime) {
    mUnknown0c = flTime;
    mUnknown1c = 0;
    mUnknown08 = 0.0f;
}

void MetScreen::UpdateAnimationFrame(float flTime) {
    if (mUnknown4c != 0) {
        return;
    }
    if (mUnknown08 == 0.0f && mUnknown0c == 0.0f) {
        UpdateIdleAnimation(flTime);
    }
    if (mUnknown08 != 0.0f && mUnknown30 != nullptr) {
        const float flEnd = mUnknown08 + mUnknown04;
        float flFrame = mUnknown08 + (mUnknown04 - flTime);
        if (flEnd < flFrame) {
            flFrame = flEnd;
        }
        mUnknown30->SetFrame(flFrame);
    }
    if (mUnknown0c != 0.0f && mUnknown30 != nullptr) {
        float flFrame = flTime - mUnknown0c;
        if (mUnknown0c + mUnknown04 < flFrame) {
            // Yes, the clamp restores the start time rather than the end frame.
            flFrame = mUnknown0c;
        }
        mUnknown30->SetFrame(flFrame);
    }
}

void MetScreen::UpdateExitAnimation(float flTime) {
    if (mUnknown78 != 0) {
        mUnknown0c = 0.0f;
        mUnknown78 = 0;
        // Yes, the binary compares the two start times right after clearing one of them, and the
        // clear also makes everything below this block unreachable on this path.
        if (mUnknown08 != mUnknown0c) {
            return;
        }
        SetShowing(0);
        mUnknown10->RemoveScreen(this);
        OnUnknownSlot36();
    }
    if (mUnknown0c == 0.0f) {
        return;
    }
    if (mUnknown30 != nullptr) {
        // Yes, the exit animation drives the enter view and its end frame, not mUnknown34.
        mUnknown30->SetFrame(flTime - mUnknown0c);
    }
    if (mUnknown0c + mUnknown04 < flTime) {
        mUnknown78 = 1;
    }
}
