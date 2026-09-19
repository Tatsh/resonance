#include "met/metscreen.h"

#include <map>

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
// The two animation view names, formatted from the screen name.
static const char *const kEnterAnimationFormat = "%s_EE.anim";
static const char *const kExitAnimationFormat = "%s_BF.anim";

} // namespace

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
    HxStr dir(directory);
    dir += kPathSeparator;
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
    HxStr name(mUnknown80);
    name += kViewSuffix;
    Rnd::Object *pObject = Rnd::g_manager.Find(name);
    mUnknown14 = pObject != nullptr ? dynamic_cast<Rnd::View *>(pObject) : nullptr;
    if (mUnknown14 != nullptr) {
        // The binary runs Rnd::Animatable::ReleaseAnimsRefs() at 0x0049a960 on the resolved view
        // here. The call is not written because that member is declared protected, and this class
        // does not derive from Rnd::Animatable.
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
