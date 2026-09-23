#include "met/metrenderer.h"

#include <algorithm>
#include <list>

#include "app/application.h"
#include "app/watchdog.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "game/gamestats.h"
#include "met/metcommandmap.h"
#include "met/metcommandrepeater.h"
#include "met/metlogoscreen.h"
#include "met/metscreen.h"
#include "msg/metcontrollerreading.h"
#include "msg/metunlockstagesmsg.h"
#include "msg/rawcontrollermsg.h"
#include "os/hxstr.h"
#include "os/zone.h"
#include "profile/profiler.h"
#include "rnd/animatable.h"
#include "rnd/asyncloader.h"
#include "rnd/drawable.h"
#include "rnd/transformable.h"
#include "script/configquery.h"

namespace {

constexpr long long kNanosecondsPerMillisecond = 1000000;

// Half a millisecond, added before the division that turns a nanosecond interval into
// milliseconds so that the quotient rounds rather than truncates.
constexpr long long kHalfMillisecondNs = 500000;

constexpr float kMillisecondsPerSecond = 1000.0f;

// Frame position the front end rewinds to when it starts running.
constexpr float kFirstFrame = 1.0f;

// Configuration code the start-up value is read under.
constexpr int kStartUpConfigCode = 0x193;

// The zone every front-end container loader allocates from.
constexpr char kLoaderZone[] = "rndglobal";

// The metagame, fonts, and shared-texture loaders whose progress PollCommonLoaders() averages.
constexpr float kCommonLoaderCount = 3.0f;

// The tag of a joystick reading, `joy `.
constexpr int kReadingTagJoystick = 0x6a6f7920;

// The translator's codes past kMetScreenCommandBack that OnRawController() also arms for repeat.
// Their meaning is not recovered.
constexpr int kRepeatCommandFirst = 0x10;
constexpr int kRepeatCommandLast = 0x13;

// Whether a scene list holds the matching subobject of a view. The list's element type selects
// the subobject the comparison converts the view to. Emitted out of line for the drawable list at
// 0x00370ab8, the transformable list at 0x00370b08, and the animatable list at 0x00370b58.
template <class T>
inline bool ContainsRef(const std::list<T *> &list, Rnd::View *const &pView) {
    return std::find(list.begin(), list.end(), pView) != list.end();
}

// Reading of the frame clock in nanoseconds, measured from the origin the watchdog's clock
// recorded when the run started. MainLoop has its own copy of the same inline.
inline long long FrameClockNs(Watchdog *pWatchdog) {
    return (ProfileClockMilliseconds() - pWatchdog->mClock.mOriginMs) * kNanosecondsPerMillisecond;
}

// Milliseconds between two frame-clock readings, rounded rather than truncated.
inline int FrameIntervalMs(long long nNowNs, long long nThenNs) {
    return static_cast<int>((nNowNs - nThenNs + kHalfMillisecondNs) / kNanosecondsPerMillisecond);
}

} // namespace

// 0x006c3598
MetRenderer *MetRenderer::sInstance;

// 0x006c3600
RndAsyncLoader *MetRenderer::sMetagameLoader;

// 0x006c3608
RndAsyncLoader *MetRenderer::sFontsLoader;

// 0x006c360c
RndAsyncLoader *MetRenderer::sSharedTexLoader;

// 0x006c3610
RndAsyncLoader *MetRenderer::sArenaLoader;

void MetRenderer::OnUnknownSlot4() {
    if (mUnknown94 != nullptr) {
        mUnknown94->Reset();
    }

    mUnknowna8 = 1;
    mUnknown68 = kFirstFrame;
    mUnknown70 = FrameClockNs(Application::shared()->GetWatchdog());
    QueryConfigValue(kStartUpConfigCode); // Yes, the binary discards the result.
}

void MetRenderer::OnUnknownSlot9() {
    if (mUnknowna8 == 0) {
        return;
    }

    const long long nNowNs = FrameClockNs(Application::shared()->GetWatchdog());
    const int nIntervalMs = FrameIntervalMs(nNowNs, mUnknown70);

    mUnknown70 = nNowNs;
    mUnknown68 += mUnknown64 * static_cast<float>(nIntervalMs) / kMillisecondsPerSecond;

    for (std::vector<MetScreen *>::iterator it = mUnknown84.begin(); it != mUnknown84.end(); ++it) {
        (*it)->UpdateAnimationFrame(mUnknown68);

        if (mUnknowna8 == 0) {
            return;
        }

        // A screen that pushed or popped another one invalidated the iterator, so the walk is
        // abandoned rather than restarted. The screens past the change are not advanced this
        // frame.
        if (mUnknown98 != 0) {
            mUnknown98 = 0;
            break;
        }
    }

    mUnknown9c->SetFrame(mUnknown68);
    mUnknown9c->UpdateWorldXfm(nullptr, 0); // Yes, the binary discards the result.
}

void MetRenderer::OnFadeOutDone() {
    mUnknownd0 = 0;

    if (mUnknownc8 != 0) {
        return;
    }

    MetScreen *const pPending = mUnknownc4;

    if (mUnknown94 != nullptr) {
        mUnknown94->Reset();
    }

    mUnknown7c = pPending;
    mUnknown80 = 1;
    AddScreen(mUnknown7c);
    mUnknown7c->PollContainerLoad(); // Yes, the binary discards the result.
    mUnknown7c->mUnknown4c = 1;
    mUnknown7c->mUnknown50 = 1;
}

void MetRenderer::OnFadeInDone() {
}

void MetRenderer::SetActivePanel(MetScreen *pScreen) {
    mUnknown7c = pScreen;

    if (mUnknown94 != nullptr) {
        mUnknown94->Reset();
    }
}

void MetRenderer::AddScreen(MetScreen *pScreen) {
    if (std::find(mUnknown84.begin(), mUnknown84.end(), pScreen) != mUnknown84.end()) {
        return;
    }

    mUnknown84.push_back(pScreen);
    mUnknown98 = 1;
}

// 0x00371858
void MetRenderer::RemoveScreenView(Rnd::View *pView) {
    mUnknowna0->RemoveTrans(pView);
    mUnknowna0->RemoveDraw(pView);
    mUnknowna0->RemoveAnim(pView);
}

void MetRenderer::ClearScreenScene() {
    mUnknowna0->ReleaseAnimsRefs();
    mUnknowna0->ClearDraws();
    mUnknowna0->ClearTransList();
}

void MetRenderer::ClearBackgroundScene() {
    mUnknowna4->ReleaseAnimsRefs();
    mUnknowna4->ClearDraws();
    mUnknowna4->ClearTransList();
}

// 0x003714f8
void MetRenderer::ActivatePanel(MetScreen *pScreen) {
    mUnknown7c = pScreen;
    if (mUnknown94 != nullptr) {
        mUnknown94->Reset();
    }
    mUnknown80 = 1;
    AddScreen(mUnknown7c);
    mUnknown7c->PollContainerLoad(); // Yes, the binary discards the result.
    mUnknown7c->mUnknown4c = 1;
    mUnknown7c->mUnknown50 = 1;
}

// 0x00371730
void MetRenderer::MoveScreenViewToFront(Rnd::View *pView) {
    if (!ContainsRef(mUnknowna0->GetDraws(), pView)) {
        AddScreenView(pView);
        return;
    }
    mUnknowna0->RemoveDraw(pView);
    mUnknowna0->AddDraw(pView, nullptr);
}

// 0x0036b8c8
void MetRenderer::UnlockAllStages() {
    if (mUnknown7c != nullptr) {
        MetUnlockStagesMsg msg;
        mUnknown7c->Handle(&msg);
    }
}

// 0x00371b58
int MetRenderer::IsLogoScreenActive() {
    return dynamic_cast<MetLogoScreen *>(mUnknown7c) != nullptr;
}

// 0x00371cb0
void MetRenderer::ForwardToPanel(Message *pMsg) {
    if (mUnknown7c != nullptr) {
        mUnknown7c->Handle(pMsg);
    }
}

// 0x00371cf0
void MetRenderer::ForwardToPanelUnchecked(Message *pMsg) {
    mUnknown7c->Handle(pMsg);
}

// 0x00371ba8
inline void MetRenderer::OnRawController(RawControllerMsg *pMsg) {
    if (mUnknowna8 == 0) {
        return;
    }
    MetControllerReading &reading = pMsg->mReading;
    if (mUnknownd4 < reading.mPadIndex) {
        return;
    }
    MetScreenCommand command;
    if (mUnknown90->Translate(&reading, &command) == 0) {
        return;
    }
    if (reading.mTag == kReadingTagJoystick) {
        switch (command.mCommand) {
        case kMetScreenCommandNone:
        case kMetScreenCommandPrevious:
        case kMetScreenCommandNext:
        case kMetScreenCommandLeft:
        case kMetScreenCommandRight:
        case kRepeatCommandFirst:
        case kRepeatCommandFirst + 1:
        case kRepeatCommandFirst + 2:
        case kRepeatCommandLast:
            mUnknown94->Arm(&command, reading.mButton, command.mPadIndex);
            break;
        default:
            break;
        }
    }
    if (mUnknown7c != nullptr && mUnknown80 != 0 && command.mCommand != kMetScreenCommandNone) {
        mUnknown7c->DeliverCommand(&command);
    }
}

// 0x0036aae0
MetScreen *MetRenderer::SelectEndScreen() {
    const GameParams params(*Application::shared()->GetGameManager()->GetParams());
    MetScreen *pScreen;
    if (params.mUnknown1c == kPlayModeJam) {
        // Yes, the binary looks each discarded screen up only for the fatal check.
        MetScreen::FindEndScreen(this, HxStr("MetSaveRemixScreen"));
        if (Application::shared()->GetGameManager()->GetParams()->mUnknown28) {
            pScreen = MetScreen::FindEndScreen(this, HxStr("MetNetEndRemixScreen"));
        } else if (Application::shared()->GetGameManager()->GetGameMode() == kGameModeSolo) {
            pScreen = MetScreen::FindEndScreen(this, HxStr("MetSoloEndRemixScreen"));
        } else {
            MetScreen::FindEndScreen(this, HxStr("MetMultiEndRemixScreen"));
            pScreen = MetScreen::FindEndScreen(this, HxStr("MetMultiSaveRemixScreen"));
        }
    } else if (Application::shared()->GetGameManager()->GetParams()->mUnknown28) {
        MetScreen::FindEndScreen(this, HxStr("MetMultiStatsScreen"));
        pScreen = MetScreen::FindEndScreen(this, HxStr("MetNetEndScreen"));
    } else if (Application::shared()->GetGameManager()->GetGameMode() == kGameModeSolo) {
        GameStats *pStats = Application::shared()->GetGameManager()->GetStats();
        MetScreen::FindEndScreen(this, HxStr("MetSoloStatsScreen"));
        if (pStats->mCompleted != 0 && pStats->mUnknown08 == 0) {
            pScreen = MetScreen::FindEndScreen(this, HxStr("MetStageFinishScreen"));
        } else {
            pScreen = MetScreen::FindEndScreen(this, HxStr("MetSoloLoseScreen"));
        }
    } else {
        MetScreen::FindEndScreen(this, HxStr("MetMultiStatsScreen"));
        pScreen = MetScreen::FindEndScreen(this, HxStr("MetMultiEndScreen"));
    }
    return pScreen;
}

// 0x00369b08
void MetRenderer::CreateCommonLoaders() {
    const int nZone = FindZoneByName(kLoaderZone);
    sMetagameLoader = new RndAsyncLoader(HxStr("MetaGame/"), HxStr("metagame.rnd"), nZone);
    sFontsLoader = new RndAsyncLoader(HxStr("metagame/fonts/"), HxStr("fonts.rnd"), nZone);
    sSharedTexLoader =
        new RndAsyncLoader(HxStr("metagame/shared/"), HxStr("shared_tex.rnd"), nZone);
}

// 0x00369e50
void MetRenderer::CreateArenaLoader() {
    const int nZone = FindZoneByName(kLoaderZone);
    sArenaLoader = new RndAsyncLoader(HxStr("MetaGame/Arena/"), HxStr("meta_arena.rnd"), nZone);
    MetScreen::CreateMainMenuScreens(sInstance);
    StartArenaLoad();
}

// 0x003712d8
void MetRenderer::StartArenaLoad() {
    EnqueueArenaLoader();
}

// 0x00371270
void MetRenderer::EnqueueCommonLoaders() {
    if (sMetagameLoader->mPending != 0) {
        sMetagameLoader->Enqueue();
    }
    if (sFontsLoader->mPending != 0) {
        sFontsLoader->Enqueue();
    }
    if (sSharedTexLoader->mPending != 0) {
        sSharedTexLoader->Enqueue();
    }
}

// 0x00371438
void MetRenderer::EnqueueArenaLoader() {
    if (sArenaLoader->mPending != 0) {
        sArenaLoader->Enqueue();
    }
}

// 0x003712f8
void MetRenderer::UnloadCommonLoaders() {
    sMetagameLoader->Unload();
    sFontsLoader->Unload();
    sSharedTexLoader->Unload();
}

// 0x00371490
void MetRenderer::UnloadArenaLoader() {
    sArenaLoader->Unload();
}

// 0x00371338
int MetRenderer::PollCommonLoaders(float *pfProgress) {
    *pfProgress = 0.0f;
    float flProgress;
    int bDone = sMetagameLoader->Poll(&flProgress);
    *pfProgress += flProgress;
    const int bFonts = sFontsLoader->Poll(&flProgress);
    *pfProgress += flProgress;
    bDone = bDone && bFonts;
    const int bSharedTex = sSharedTexLoader->Poll(&flProgress);
    *pfProgress = (*pfProgress + flProgress) / kCommonLoaderCount;
    return bDone && bSharedTex;
}

// 0x003713f0
int MetRenderer::PollArenaLoader(float *pfProgress) {
    *pfProgress = 0.0f;
    float flProgress;
    const int bDone = sArenaLoader->Poll(&flProgress);
    *pfProgress += flProgress;
    return bDone;
}
