#include "met/metsonyscreen.h"

#include <string.h>

#include "app/application.h"
#include "app/cutscene.h"
#include "app/mainloop.h"
#include "gfx/gfxdevice.h"
#include "met/metfade.h"
#include "met/metfreqmakerassetmanager.h"
#include "met/metfrontendstate.h"
#include "met/metrenderer.h"
#include "msg/metfreqendedmsg.h"
#include "os/async.h"
#include "os/hostmode.h"
#include "os/hxstr.h"
#include "os/loadfile.h"
#include "synth/midi_main.h"
#include "synth/ps2hardsynth.h"

namespace {

// The screen name, the directory the container loads from, and the container name. The European
// release appends LocalizedAssetSuffix() to the container name.
#ifdef VIDEO_STANDARD_PAL
static const char *const kScreenName = "esony";
static const char *const kContainerName = "esony_pres";
#else
static const char *const kScreenName = "sony";
static const char *const kContainerName = "sony_pres";
#endif
static const char *const kDirectory = "metagame/Shared";

#ifdef VIDEO_STANDARD_PAL
// PAL: 0x003eeb40
inline HxStr ContainerName() {
    HxStr name(kContainerName);
    HxStr suffix;
    suffix = LocalizedAssetSuffix();
    name += suffix;
    return name;
}
#else
inline HxStr ContainerName() {
    return HxStr(kContainerName);
}
#endif

// This screen's registry key, and the screen whose container slot 26 waits on.
static const char *const kOwnScreenName = "MetSonyScreen";
static const char *const kGizmoScreenName = "MetEndGameGizmoScreen";

// The intro movie and the two device prefixes it is read through.
static const char *const kIntroMovieName = "ps2intro.pss";
static const char *const kHostPrefix = "host0:";
static const char *const kDiscPrefix = "cdrom0:";
constexpr int kIntroPathLength = 64;
constexpr int kPlayWithAudio = 1;

// Frames the presentation shows for, and frames each fade takes.
constexpr float kPresentationFrames = 480.0f;
constexpr float kFadeFrames = 360.0f;
constexpr int kReleaseView = 0;

// What MetRenderer::PollArenaLoader() reports once the arena load is complete.
constexpr int kLoadComplete = 1;

// The payload the finishing message carries.
constexpr int kFreqEndedPayload = 1;

// AsyncCheck() waits for the pending operation.
constexpr int kBlockingCheck = 1;

// NTSC-U/C: 0x003bd5f0, PAL: 0x003f2328
// Plays the intro movie from the host link or the disc. Slot 36 expands the same code inline, and
// this out-of-line copy has no caller.
inline void PlayIntroMovie() {
    char szPath[kIntroPathLength];
    AsyncCheck(kBlockingCheck);
    if (GetHostMode() == kHostModeHostOnly) {
        strcpy(szPath, kHostPrefix);
        strcat(szPath, kIntroMovieName);
    } else {
        strcpy(szPath, kDiscPrefix);
        FilenameToISO9660(kIntroMovieName, szPath);
    }
    play_cutscene(szPath, kPlayWithAudio);
}

} // namespace

// NTSC-U/C: 0x006cc138, PAL: 0x0070f3f8
// Set until slot 26 has skipped its first chance to fade back in.
int g_nSonySkipFirstFadeIn = 1;

// NTSC-U/C: 0x006cc13c, PAL: 0x0070f3fc
// Set until slot 36 has played the intro movie.
int g_nSonyIntroPending = 1;

// NTSC-U/C: 0x003ba0f8, PAL: 0x003eed00
MetSonyScreen::MetSonyScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), ContainerName()),
      mEnterTime(0.0f), mFadeOutDoneTime(0.0f), mFade(nullptr), mHidden(0) {
    mFade = new MetFade(pRenderer);
}

// NTSC-U/C: 0x003bd7a8, PAL: 0x003f24e0
MetSonyScreen::~MetSonyScreen() {
    delete mFade;
}

// NTSC-U/C: 0x003bd720, PAL: 0x003f2458
MetSonyScreen *MetSonyScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetSonyScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x003bd6f8, PAL: 0x003f2430
void MetSonyScreen::PlaySlideSound([[maybe_unused]] int nSelector) {
}

// NTSC-U/C: 0x003bd700, PAL: 0x003f2438
void MetSonyScreen::PlayLeaveSound([[maybe_unused]] int nSelector) {
}

// NTSC-U/C: 0x003bd708, PAL: 0x003f2440
void MetSonyScreen::PlayHighSound([[maybe_unused]] int nSelector) {
}

// NTSC-U/C: 0x003bd710, PAL: 0x003f2448
void MetSonyScreen::PlayCycleLeftSound([[maybe_unused]] int nSelector) {
}

// NTSC-U/C: 0x003bd718, PAL: 0x003f2450
void MetSonyScreen::PlayCycleRightSound([[maybe_unused]] int nSelector) {
}

// NTSC-U/C: 0x003bd8b8, PAL: 0x003f25f0
void MetSonyScreen::OnFadeInDone() {
    SetShowing(0);
    BeginExit();
}

// NTSC-U/C: 0x003bd908, PAL: 0x003f2640
void MetSonyScreen::OnFadeOutDone() {
    MetScreen::CreateFrontEndScreens(MetRenderer::sInstance);
    mFadeOutDoneTime = mRenderer->mAnimationFrame;
}

// NTSC-U/C: 0x003bd868, PAL: 0x003f25a0
void MetSonyScreen::EnterAndShow() {
    SetShowing(0);
    mHidden = 1;
    mEnterTime = mRenderer->mAnimationFrame;
}

// NTSC-U/C: 0x003ba2f0, PAL: 0x003eef50
void MetSonyScreen::UpdateIdle(float flTime) {
    mFade->Update(flTime);
    if ((mEnterTime != 0.0f) && ((mEnterTime + kPresentationFrames) < flTime)) {
        mEnterTime = 0.0f;
        mFade->FadeOut(kFadeFrames, mRenderer->mAnimationFrame, this, kReleaseView);
        if (mHidden != 0) {
            mHidden = 0;
            SetShowing(1);
        }
    }
    if ((mFadeOutDoneTime == 0.0f) || !((mFadeOutDoneTime + kPresentationFrames) < flTime)) {
        return;
    }
    float flProgress;
    if (MetRenderer::PollArenaLoader(&flProgress) != kLoadComplete) {
        return;
    }
    if (!MetFreqMakerAssetManager::shared()->AreIdentitiesLoaded()) {
        return;
    }
    if (MetScreen::FindScreenByName(HxStr(kGizmoScreenName))->PollContainerLoad() == 0) {
        return;
    }
    if (g_nSonySkipFirstFadeIn != 0) {
        g_nSonySkipFirstFadeIn = 0;
        return;
    }
    mFadeOutDoneTime = 0.0f;
    mFade->FadeIn(kFadeFrames, mRenderer->mAnimationFrame, this, kReleaseView);
}

// NTSC-U/C: 0x003ba4c8, PAL: 0x003ef148
void MetSonyScreen::OnExitFinished() {
    if (g_nSonyIntroPending == 0) {
        return;
    }
    if (IntroMovieEnabled() != 0) {
        Rnd::ThePs.ResetVramAndSavePacket();
        PlayIntroMovie();
        Rnd::ThePs.InitDisplayMode();
    }
    g_nSonyIntroPending = 0;
    MetFrontEndState::shared()->mReturnScreen = HxStr(kOwnScreenName);
    Finish();
}

// NTSC-U/C: 0x003bd828, PAL: 0x003f2560
void MetSonyScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    SetShowing(0);
}

// NTSC-U/C: 0x003ba620, PAL: 0x003ef2c0
void MetSonyScreen::Finish() {
    mRenderer->RemoveScreen(this);
    mRenderer->ClearBackgroundScene();
    SetBankLoadProgressHook(MainLoop::KeepAliveDraw);
    Application::shared()->GetSynth()->LoadBankSet4();
    mRenderer->Stop();
    MetFreqEndedMsg msg;
    msg.mStopJukebox = kFreqEndedPayload;
    mRenderer->Dispatch(&msg);
}
