#include "met/metscreen.h"

#include <map>

#include "app/playsound.h"
#include "met/metlogoscreen.h"
#include "met/metmemdetectstartup.h"
#include "met/metmsgscreen.h"
#include "met/metrenderer.h"
#include "met/metsonyscreen.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "os/mem.h"
#include "os/zone.h"
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

// What RndAsyncLoader::Poll() reports once a load is finished, which PollContainerLoads() also
// records in MetContainerLoad::mUnknown04.
constexpr int kLoadComplete = 1;

// The zone the start-up screens load into.
static const char *const kGlobalZone = "rndglobal";
// The registry keys CreateStartupScreens() writes.
static const char *const kSonyScreenKey = "MetSonyScreen";
static const char *const kMemDetectStartupKey = "MetMemDetectStartup";
static const char *const kMsgScreenKey = "MetMsgScreen";
static const char *const kLogoScreenKey = "MetLogoScreen";

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

// 0x00390000
MetScreen *MetScreen::FindEndScreen([[maybe_unused]] MetRenderer *pRenderer, const HxStr &name) {
    MetScreen *pScreen = nullptr;
    std::map<HxStr, MetScreen *>::iterator it = ScreenRegistry().find(name);
    if (it != ScreenRegistry().end()) {
        pScreen = (*it).second;
    }
    if (pScreen == nullptr) {
        Fatal("PROBLEM end screen is not found!\n");
        return nullptr; // Yes, the binary keeps a return after the call that does not return.
    }
    return pScreen;
}

void MetScreen::BeginContainerLoad(const HxStr &directory, [[maybe_unused]] const HxStr &file) {
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

void MetScreen::SetShowing(int nShowing) {
    // Yes, the view is dereferenced without a null check, and ResolveContainerViews() calls this
    // straight after a failed resolution leaves it null.
    mUnknown14->Drawable::SetShowing(nShowing);
    if (mUnknown60 == 0) {
        return;
    }
    std::list<Rnd::Drawable *> draws(ContainerLoaderMap()[mUnknown28]->mLoader->mDrawables);
    for (std::list<Rnd::Drawable *>::iterator it = draws.begin(); it != draws.end(); ++it) {
        (*it)->SetShowing(nShowing);
    }
}

// 0x00390200
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

// 0x003900a8
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

// 0x003902d0
void MetScreen::ExitScreenByName(const HxStr &name) {
    MemLogWrite(
        FormatString("Exiting screen: %s\n", name.mStr != nullptr ? name.mStr : g_szEmptyString));
    // The binary neither checks the result nor recovers from a key nothing registered under.
    FindScreenByName(name)->BeginExit();
}

// 0x00390100
void MetScreen::BeginExit() {
    StartExitAnimation(mUnknown10->mUnknown68);
}

// 0x003905c0
void MetScreen::StartEnterAnimation(float flTime) {
    mUnknown08 = flTime;
    mUnknown0c = 0.0f;
    if (mUnknown30 != nullptr) {
        mUnknown30->SetFrame(mUnknown04);
    }
}

// 0x003905f0
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

// 0x00390130
void MetScreen::OnUnknownSlot10() {
}

// 0x00390138
void MetScreen::OnKeyboardDismissed() {
}

void MetScreen::OnDrawPass() {
}

// 0x003900a0
void MetScreen::OnDestroying() {
}

void MetScreen::OnMsgScreenDismissed([[maybe_unused]] const HxStr &name,
                                     [[maybe_unused]] int nChoice) {
}

void MetScreen::OnMsgScreenShown([[maybe_unused]] const HxStr &name) {
}

void MetScreen::HandleCommand([[maybe_unused]] const MetScreenCommand *pCommand) {
}

void MetScreen::OnUnknownSlot26([[maybe_unused]] float flTime) {
}

void MetScreen::UpdateIdleAnimation([[maybe_unused]] float flTime) {
}

void MetScreen::OnUnknownSlot30([[maybe_unused]] Rnd::Object *pObject) {
}

void MetScreen::OnUnknownSlot33() {
}

void MetScreen::OnUnknownSlot36() {
}

// 0x003907a8
void MetScreen::HandleMessage([[maybe_unused]] Message *pMsg) {
}

// 0x00390140
void MetScreen::PlaySlideSound([[maybe_unused]] int nSelector) {
    PlaySoundByName(kSlideSound);
}

// 0x00390160
void MetScreen::PlayLeaveSound([[maybe_unused]] int nSelector) {
    PlaySoundByName(kLeaveSound);
}

// 0x003901c0
void MetScreen::PlayHighSound([[maybe_unused]] int nSelector) {
    PlaySoundByName(kHighSound);
}

// 0x00390180
void MetScreen::PlayCycleLeftSound([[maybe_unused]] int nSelector) {
    PlaySoundByName(kCycleLeftSound);
}

// 0x003901a0
void MetScreen::PlayCycleRightSound([[maybe_unused]] int nSelector) {
    PlaySoundByName(kCycleRightSound);
}

// 0x003901e0
void MetScreen::PlayErrorSound([[maybe_unused]] int nSelector) {
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
            PlayLeaveSound(pCommand->mPadIndex);
            break;
        default:
            break;
        }
    }
    HandleCommand(pCommand);
}

// 0x00390788
void MetScreen::Draw() {
    mUnknown14->Drawable::Draw();
}

// 0x003906a0
void MetScreen::StartExitAnimation(float flTime) {
    mUnknown0c = flTime;
    mUnknown1c = 0;
    mUnknown08 = 0.0f;
}

// 0x00390380
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

void MetScreen::UpdateFrame(float flTime) {
    if (mUnknown4c != 0) {
        if (ContainerLoaderMap()[mUnknown28]->mUnknown04 != 0) {
            if (mUnknown48 != 0) {
                ResolveContainerViews();
            }
            mUnknown10->AddScreenView(mUnknown14);
            EnterAndShow();
            if (mUnknown50 != 0) {
                mUnknown4c = 0;
                mUnknown10->SetActivePanel(this);
                mUnknown10->mUnknown80 = 1;
                OnUnknownSlot7();
                mUnknown50 = 0;
            }
            return;
        }
        float flProgress;
        if (ContainerLoaderMap()[mUnknown28]->mLoader->Poll(&flProgress) != 1) {
            return;
        }
        ContainerLoaderMap()[mUnknown28]->mUnknown04 = 1;
        return;
    }
    UpdateEnterAnimation(flTime);
    if (mUnknown54 != 0) {
        OnUnknownSlot26(flTime);
    } else if (mUnknown08 == 0.0f && mUnknown0c == 0.0f) {
        OnUnknownSlot26(flTime);
    }
    UpdateRepeatingSound(flTime);
    UpdateExitAnimation(flTime);
}

// 0x003906b0
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

// 0x00383700
void MetScreen::DestroyAllScreens() {
    std::map<HxStr, MetScreen *>::iterator screen = ScreenRegistry().begin();
    while (screen != ScreenRegistry().end()) {
        delete screen->second;
        ScreenRegistry().erase(screen++);
    }
    std::map<HxStr, MetContainerLoad *>::iterator load = ContainerLoaderMap().begin();
    while (load != ContainerLoaderMap().end()) {
        MetContainerLoad *pLoad = load->second;
        if (pLoad->mLoader != nullptr) {
            delete pLoad->mLoader;
            pLoad->mLoader = nullptr;
        }
        // Yes, the binary erases the entry without freeing the record it points at.
        ContainerLoaderMap().erase(load++);
    }
}

// 0x00381ef8
void MetScreen::PollContainerLoads() {
    for (std::map<HxStr, MetContainerLoad *>::iterator it = ContainerLoaderMap().begin();
         it != ContainerLoaderMap().end();
         ++it) {
        MetContainerLoad *pLoad = it->second;
        if (pLoad->mUnknown04 != 0) {
            continue;
        }
        float flProgress;
        if (pLoad->mLoader->Poll(&flProgress) != kLoadComplete) {
            continue;
        }
        pLoad->mUnknown04 = kLoadComplete;
        std::list<Rnd::Drawable *> draws(pLoad->mLoader->mDrawables);
        for (std::list<Rnd::Drawable *>::iterator draw = draws.begin(); draw != draws.end();
             ++draw) {
            (*draw)->SetShowing(0);
        }
    }
}

// 0x00384300
void MetScreen::CreateStartupScreens(MetRenderer *pRenderer) {
    int nZone = FindZoneByName(kGlobalZone);
    ScreenRegistry()[HxStr(kSonyScreenKey)] = MetSonyScreen::New(pRenderer, nZone);
    ScreenRegistry()[HxStr(kMemDetectStartupKey)] = MetMemDetectStartup::New(pRenderer, nZone);
    ScreenRegistry()[HxStr(kMsgScreenKey)] = MetMsgScreen::New(pRenderer, nZone);
    ScreenRegistry()[HxStr(kLogoScreenKey)] = MetLogoScreen::New(pRenderer, nZone);
}
