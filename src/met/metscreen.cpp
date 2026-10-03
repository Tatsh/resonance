#include "met/metscreen.h"

#include <map>
#include <stdio.h>

#include "app/playsound.h"
#include "met/metarenasscreen.h"
#include "met/metconfigcontrollerscreen.h"
#include "met/metconfiggameoptionsscreen.h"
#include "met/metconfigoptionsbuttonsscreen.h"
#include "met/metcreditsscreen.h"
#include "met/metendgamegizmoscreen.h"
#include "met/metexpansionpakscreen.h"
#include "met/metfreqcreatescreen.h"
#include "met/metfreqmakerbuttonsscreen.h"
#include "met/metfreqmakercanvasscreen.h"
#include "met/metfreqmakerdirectionsscreen.h"
#include "met/metfreqmakerinventoryscreen.h"
#include "met/metgameskillscreen.h"
#include "met/metglobalsettingssaverscreen.h"
#include "met/methelpscreen.h"
#include "met/metjukeboxcustomremixesscreen.h"
#include "met/metjukeboxeditplaylistscreen.h"
#include "met/metjukeboxeditplaylistscreendone.h"
#include "met/metjukeboxeditplaylistscreenlowerleft.h"
#include "met/metjukeboxfactoryremixesscreen.h"
#include "met/metjukeboxtopbuttonsscreen.h"
#include "met/metkeyboardscreen.h"
#include "met/metleftgizmoscreen.h"
#include "met/metleftgizmosmallscreen.h"
#include "met/metloadfreqscreen.h"
#include "met/metloadgamescreen.h"
#include "met/metloadnewfreqscreen.h"
#include "met/metloadprefabscreen.h"
#include "met/metlocnumplayscreen.h"
#include "met/metlocpickcharscreen.h"
#include "met/metlogoscreen.h"
#include "met/metmainscreen.h"
#include "met/metmcfreqdelscreen.h"
#include "met/metmemcardloadscreen.h"
#include "met/metmemcardtypescreen.h"
#include "met/metmemdetectstartup.h"
#include "met/metmodescreen.h"
#include "met/metmsgscreen.h"
#include "met/metmultiendremixscreen.h"
#include "met/metmultiendscreen.h"
#include "met/metmultisaveremixscreen.h"
#include "met/metmultistatsscreen.h"
#include "met/metmultitips1screen.h"
#include "met/metmultitips2screen.h"
#include "met/metmultitips3screen.h"
#include "met/metmultitips4screen.h"
#include "met/metmultitips5screen.h"
#include "met/metpausegamescreen.h"
#include "met/metpausemultiremixscreen.h"
#include "met/metpausesologamescreen.h"
#include "met/metpausesoloremixscreen.h"
#include "met/metpersonasaverscreen.h"
#include "met/metremixdatascreen.h"
#include "met/metremixdelscreen.h"
#include "met/metremixloadscreen.h"
#include "met/metremixmanager.h"
#include "met/metremixtypescreen.h"
#include "met/metrenderer.h"
#include "met/metrightgizmoscreen.h"
#include "met/metsaveremixscreen.h"
#include "met/metscreentitlescreen.h"
#include "met/metsoloendremixscreen.h"
#include "met/metsololosescreen.h"
#include "met/metsolostagesscreen.h"
#include "met/metsolostatsscreen.h"
#include "met/metsolowinscreen.h"
#include "met/metsonyscreen.h"
#include "met/metstagefinishscreen.h"
#include "met/mettoplogoscreen.h"
#include "met/mettutorialscreen.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "os/mem.h"
#include "os/zone.h"
#include "rnd/animatable.h"
#include "rnd/asyncloader.h"
#include "rnd/button.h"
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

// The two button states the repeating alternation switches between, and the steps in one cycle.
constexpr int kRestState = 1;
constexpr int kAlternateState = 2;
constexpr int kStepsPerCycle = 2;

// The stack buffer ExitScreenByName() formats its log line into.
constexpr int kExitLogBufferSize = 128;

// What RndAsyncLoader::PollAsyncLoads() reports once a load is finished. PollContainerLoads() also
// records it in MetContainerLoad::mFinished.
constexpr int kLoadComplete = 1;

// The zone the start-up screens load into.
static const char *const kGlobalZone = "rndglobal";
// The registry keys CreateStartupScreens() writes.
static const char *const kSonyScreenKey = "MetSonyScreen";
static const char *const kMemDetectStartupKey = "MetMemDetectStartup";
static const char *const kMsgScreenKey = "MetMsgScreen";
static const char *const kLogoScreenKey = "MetLogoScreen";

// The zone DestroyNonDefaultScreens() rewinds.
static const char *const kLocAndNetZone = "rndMetLocAndNet";

// The categories the two single-category walkers select.
constexpr int kCategory1 = 1;
constexpr int kCategory2 = 2;

// The registry keys CreateMainMenuScreens() writes.
static const char *const kMainScreenKey = "MetMainScreen";
static const char *const kTopLogoScreenKey = "MetTopLogoScreen";
static const char *const kLeftGizmoSmallScreenKey = "MetLeftGizmoSmallScreen";
static const char *const kLeftGizmoScreenKey = "MetLeftGizmoScreen";
static const char *const kHelpScreenKey = "MetHelpScreen";
static const char *const kScreenTitleScreenKey = "MetScreenTitleScreen";

// The registry keys CreateFrontEndScreens() writes, in its order.
static const char *const kLoadPreFabScreenKey = "MetLoadPreFabScreen";
static const char *const kLoadFreqScreenKey = "MetLoadFreqScreen";
static const char *const kFreqCreateScreenKey = "MetFreqCreateScreen";
static const char *const kLoadNewFreqScreenKey = "MetLoadNewFreqScreen";
static const char *const kModeScreenKey = "MetModeScreen";
static const char *const kGameSkillScreenKey = "MetGameSkillScreen";
static const char *const kRemixTypeScreenKey = "MetRemixTypeScreen";
static const char *const kSoloStagesScreenKey = "MetSoloStagesScreen";
static const char *const kArenasScreenKey = "MetArenasScreen";
static const char *const kFreqMakerButtonsScreenKey = "MetFreqMakerButtonsScreen";
static const char *const kFreqMakerCanvasScreenKey = "MetFreqMakerCanvasScreen";
static const char *const kFreqMakerDirectionsScreenKey = "MetFreqMakerDirectionsScreen";
static const char *const kFreqMakerInventoryScreenKey = "MetFreqMakerInventoryScreen";
// MetLocNumPlayScreen registers under this longer spelling.
static const char *const kLocNumPlayersScreenKey = "MetLocNumPlayersScreen";
static const char *const kLocPickCharScreenKey = "MetLocPickCharScreen";
static const char *const kPersonaSaverScreenKey = "MetPersonaSaverScreen";
static const char *const kGlobalSettingsSaverScreenKey = "MetGlobalSettingsSaverScreen";
static const char *const kRemixManagerKey = "MetRemixManager";
static const char *const kKeyboardScreenKey = "MetKeyboardScreen";
static const char *const kConfigControllerScreenKey = "MetConfigControllerScreen";
static const char *const kConfigGameOptionsScreenKey = "MetConfigGameOptionsScreen";
static const char *const kExpansionPakScreenKey = "MetExpansionPakScreen";
static const char *const kRightGizmoScreenKey = "MetRightGizmoScreen";
static const char *const kTutorialScreenKey = "MetTutorialScreen";
static const char *const kMemCardLoadScreenKey = "MetMemCardLoadScreen";
static const char *const kMemCardTypeScreenKey = "MetMemCardTypeScreen";
static const char *const kMCFreqDelScreenKey = "MetMCFreqDelScreen";
static const char *const kRemixDelScreenKey = "MetRemixDelScreen";
static const char *const kRemixLoadScreenKey = "MetRemixLoadScreen";
static const char *const kRemixDataScreenKey = "MetRemixDataScreen";
static const char *const kCreditsScreenKey = "MetCreditsScreen";
static const char *const kJukeboxTopButtonsScreenKey = "MetJukeboxTopButtonsScreen";
static const char *const kJukeboxCustomRemixesScreenKey = "MetJukeboxCustomRemixesScreen";
static const char *const kJukeboxFactoryRemixesScreenKey = "MetJukeboxFactoryRemixesScreen";
static const char *const kJukeboxEditPlaylistScreenKey = "MetJukeboxEditPlaylistScreen";
static const char *const kJukeboxEditPlaylistScreenLowerLeftKey =
    "MetJukeboxEditPlaylistScreenLowerLeft";
static const char *const kJukeboxEditPlaylistScreenDoneKey = "MetJukeboxEditPlaylistScreenDone";
static const char *const kConfigOptionsButtonsScreenKey = "MetConfigOptionsButtonsScreen";
static const char *const kMultiTips1ScreenKey = "MetMultiTips1Screen";
static const char *const kMultiTips2ScreenKey = "MetMultiTips2Screen";
static const char *const kMultiTips3ScreenKey = "MetMultiTips3Screen";
static const char *const kMultiTips4ScreenKey = "MetMultiTips4Screen";
static const char *const kMultiTips5ScreenKey = "MetMultiTips5Screen";
static const char *const kLoadGameScreenKey = "MetLoadGameScreen";
static const char *const kPauseGameScreenKey = "MetPauseGameScreen";
static const char *const kPauseSoloGameScreenKey = "MetPauseSoloGameScreen";
static const char *const kPauseSoloRemixScreenKey = "MetPauseSoloRemixScreen";
static const char *const kPauseMultiRemixScreenKey = "MetPauseMultiRemixScreen";
static const char *const kSoloLoseScreenKey = "MetSoloLoseScreen";
static const char *const kSoloWinScreenKey = "MetSoloWinScreen";
static const char *const kSoloEndRemixScreenKey = "MetSoloEndRemixScreen";
static const char *const kMultiEndScreenKey = "MetMultiEndScreen";
static const char *const kMultiEndRemixScreenKey = "MetMultiEndRemixScreen";
static const char *const kSoloStatsScreenKey = "MetSoloStatsScreen";
static const char *const kStageFinishScreenKey = "MetStageFinishScreen";
static const char *const kMultiStatsScreenKey = "MetMultiStatsScreen";
static const char *const kSaveRemixScreenKey = "MetSaveRemixScreen";
static const char *const kMultiSaveRemixScreenKey = "MetMultiSaveRemixScreen";
static const char *const kEndGameGizmoScreenKey = "MetEndGameGizmoScreen";

} // namespace

// NTSC-U/C: 0x0038a450, PAL: 0x003bb9a8
MetScreen::MetScreen(MetRenderer *pRenderer,
                     int nPriority,
                     const HxStr &name,
                     const HxStr &directory,
                     const HxStr &file)
    : mEnterStartTime(0.0f), mExitStartTime(0.0f), mRenderer(pRenderer), mView(nullptr),
      mExitChoice(2), mAcceptsCommands(0), mScreenName(name), mEnterAnim(nullptr),
      mBackAnim(nullptr), mViewsUnresolved(1), mEnterPending(0), mActivatePending(0),
      mIdleWhileAnimating(0), mRepeatScale(1.0f), mPlaysCommandSounds(1), mShowsLoadedDrawables(1),
      mRepeatNextTime(0.0f), mRepeatButton(nullptr), mRepeatStep(0), mExitDone(0), mEnterDone(0),
      mContainerName(file), mLoadPriority(nPriority) {
    mContainerFile = file + kContainerSuffix;
    mRenderer->AddSink(this);
    if (directory != "" && file != "") {
        // Yes, the binary calls the virtual directly rather than through slot 18, which is what a
        // virtual call from a constructor compiles to.
        MetScreen::BeginContainerLoad(directory, file);
    }
}

// NTSC-U/C: 0x0038a848, PAL: 0x003bbe10
MetScreen::~MetScreen() {
    mRenderer->RemoveScreen(this);
    OnDestroying();
    mRenderer->RemoveSink(this);
}

// NTSC-U/C: 0x00381e10, PAL: 0x003b2490
std::map<HxStr, MetContainerLoad *> &MetScreen::ContainerLoaderMap() {
    static std::map<HxStr, MetContainerLoad *> theMap;
    return theMap;
}

// NTSC-U/C: 0x003821e0, PAL: 0x003b2860
std::map<HxStr, MetScreenEntry> &MetScreen::ScreenRegistry() {
    static std::map<HxStr, MetScreenEntry> theMap;
    return theMap;
}

// NTSC-U/C: 0x0038ff90, PAL: 0x003c1860
MetScreen *MetScreen::FindScreenByName(const HxStr &name) {
    MetScreen *pScreen = nullptr;
    std::map<HxStr, MetScreenEntry>::iterator it = ScreenRegistry().find(name);
    if (it != ScreenRegistry().end()) {
        pScreen = (*it).second.mScreen;
    }
    return pScreen;
}

// NTSC-U/C: 0x00390000, PAL: 0x003c18d0
MetScreen *MetScreen::FindEndScreen([[maybe_unused]] MetRenderer *pRenderer, const HxStr &name) {
    MetScreen *pScreen = nullptr;
    std::map<HxStr, MetScreenEntry>::iterator it = ScreenRegistry().find(name);
    if (it != ScreenRegistry().end()) {
        pScreen = (*it).second.mScreen;
    }
    if (pScreen == nullptr) {
        Fatal("PROBLEM end screen is not found!\n");
        return nullptr; // Yes, the binary keeps a return after the call that does not return.
    }
    return pScreen;
}

// NTSC-U/C: 0x0038aa00, PAL: 0x003bc008
void MetScreen::BeginContainerLoad(const HxStr &directory, [[maybe_unused]] const HxStr &file) {
    HxStr dir = directory + kPathSeparator;
    // A screen whose container another screen already loads does not touch the existing load.
    if (ContainerLoaderMap()[mContainerFile] != nullptr) {
        return;
    }
    MetContainerLoad *pLoad = new MetContainerLoad;
    pLoad->mLoader = new RndAsyncLoader(dir, mContainerFile, mLoadPriority);
    pLoad->mEnqueuePending = 1;
    pLoad->mFinished = 0;
    ContainerLoaderMap()[mContainerFile] = pLoad;
    ContainerLoaderMap()[mContainerFile]->mFinished = 0;
    // Yes, the binary sets mEnqueuePending and then immediately tests it. The branch is always
    // taken.
    ContainerLoaderMap()[mContainerFile]->mEnqueuePending = 1;
    if (ContainerLoaderMap()[mContainerFile]->mEnqueuePending != 0) {
        ContainerLoaderMap()[mContainerFile]->mEnqueuePending = 0;
        ContainerLoaderMap()[mContainerFile]->mLoader->Enqueue();
    }
}

// NTSC-U/C: 0x0038b338, PAL: 0x003bca70
int MetScreen::PollContainerLoad() {
    MetContainerLoad *pLoad = ContainerLoaderMap()[mContainerFile];
    if (pLoad->mLoader == nullptr) {
        return 0;
    }
    float flProgress;
    if (pLoad->mLoader->Poll(&flProgress) != 1) {
        return 0;
    }
    if (mViewsUnresolved != 0) {
        ResolveContainerViews();
    }
    return 1;
}

// NTSC-U/C: 0x0038bd60, PAL: 0x003bd538
void MetScreen::ResolveAnimationViews() {
    {
        HxStr name(FormatString(kEnterAnimationFormat,
                                mScreenName.mStr != nullptr ? mScreenName.mStr : g_szEmptyString));
        Rnd::Object *pObject = Rnd::g_manager.Find(name);
        mEnterAnim = pObject != nullptr ? dynamic_cast<Rnd::View *>(pObject) : nullptr;
    }
    {
        HxStr name(FormatString(kExitAnimationFormat,
                                mScreenName.mStr != nullptr ? mScreenName.mStr : g_szEmptyString));
        Rnd::Object *pObject = Rnd::g_manager.Find(name);
        mBackAnim = pObject != nullptr ? dynamic_cast<Rnd::View *>(pObject) : nullptr;
    }
    mAnimEndFrame = mEnterAnim != nullptr ? mEnterAnim->EndFrame() : 0.0f;
}

// NTSC-U/C: 0x0038b1b0, PAL: 0x003bc8a8
void MetScreen::ResolveContainerViews() {
    ResolveAnimationViews();
    HxStr name = mContainerName + kViewSuffix;
    Rnd::Object *pObject = Rnd::g_manager.Find(name);
    mView = pObject != nullptr ? dynamic_cast<Rnd::View *>(pObject) : nullptr;
    if (mView != nullptr) {
        mView->ReleaseAnimsRefs();
    } else {
        Fatal(" the screen %s doesn't have a valid view!\n",
              mContainerName.mStr != nullptr ? mContainerName.mStr : g_szEmptyString);
    }
    SetShowing(0);
    mViewsUnresolved = 0;
}

// NTSC-U/C: 0x0038b490, PAL: 0x003bcbe8
void MetScreen::SetShowing(int nShowing) {
    // Yes, the binary dereferences the view without a null check.
    static_cast<Rnd::Drawable *>(mView)->SetShowing(nShowing);
    if (mShowsLoadedDrawables == 0) {
        return;
    }
    std::list<Rnd::Drawable *> draws(ContainerLoaderMap()[mContainerFile]->mLoader->mDrawables);
    for (std::list<Rnd::Drawable *>::iterator it = draws.begin(); it != draws.end(); ++it) {
        (*it)->SetShowing(nShowing);
    }
}

// NTSC-U/C: 0x00390200, PAL: 0x003c1ad0
void MetScreen::PushNamedScreen(const HxStr &name) {
    MetScreen *pScreen = FindScreenByName(name);
    mRenderer->AddScreen(pScreen);
    if (pScreen->PollContainerLoad() != 0) {
        mRenderer->AddScreenView(pScreen->mView);
        pScreen->EnterAndShow();
    } else {
        pScreen->mEnterPending = 1;
    }
}

// NTSC-U/C: 0x003900a8, PAL: 0x003c1978
void MetScreen::EnterAndShow() {
    SetShowing(1);
    StartEnterAnimation(mRenderer->mAnimationFrame);
}

// NTSC-U/C: 0x0038b828, PAL: 0x003bcfa0
void MetScreen::ActivateNamedPanel(const HxStr &name) {
    if (name == "") {
        mRenderer->mPanelActive = 0;
        return;
    }
    MetScreen *pScreen = FindScreenByName(name);
    if (pScreen->PollContainerLoad() != 0) {
        mRenderer->SetActivePanel(pScreen);
        mRenderer->mPanelActive = 1;
        pScreen->OnPanelActivated();
    } else {
        pScreen->mActivatePending = 1;
    }
}

// NTSC-U/C: 0x003902d0, PAL: 0x003c1ba0
void MetScreen::ExitScreenByName(const HxStr &name) {
    char szLine[kExitLogBufferSize];
    sprintf(szLine, "Exiting screen: %s\n", name.mStr != nullptr ? name.mStr : g_szEmptyString);
    MemLogWrite(szLine);
    // The binary neither checks the result nor recovers from a key nothing registered under.
    FindScreenByName(name)->BeginExit();
}

// NTSC-U/C: 0x00390100, PAL: 0x003c19d0
void MetScreen::BeginExit() {
    StartExitAnimation(mRenderer->mAnimationFrame);
}

// NTSC-U/C: 0x003905c0, PAL: 0x003c1e90
void MetScreen::StartEnterAnimation(float flTime) {
    mEnterStartTime = flTime;
    mExitStartTime = 0.0f;
    if (mEnterAnim != nullptr) {
        mEnterAnim->SetFrame(mAnimEndFrame);
    }
}

// NTSC-U/C: 0x003905f0, PAL: 0x003c1ec0
void MetScreen::UpdateEnterAnimation(float flTime) {
    if (mEnterDone != 0) {
        mEnterDone = 0;
        mAcceptsCommands = 1;
        mEnterStartTime = 0.0f;
        OnEnterFinished();
    }
    if (mEnterStartTime == 0.0f) {
        return;
    }
    if (mEnterAnim != nullptr) {
        mEnterAnim->SetFrame(mEnterStartTime + mAnimEndFrame - flTime);
    }
    if (mEnterStartTime + mAnimEndFrame < flTime) {
        mEnterDone = 1;
    }
}

// NTSC-U/C: 0x0038fdf8, PAL: 0x003c16c8
void MetScreen::OnPanelActivated() {
}

// NTSC-U/C: 0x00390130, PAL: 0x003c1a00
void MetScreen::OnUnusedHook() {
}

// NTSC-U/C: 0x00390138, PAL: 0x003c1a08
void MetScreen::OnKeyboardDismissed() {
}

// NTSC-U/C: 0x0038fe00, PAL: 0x003c16d0
void MetScreen::OnDrawPass() {
}

// NTSC-U/C: 0x003900a0, PAL: 0x003c1970
void MetScreen::OnDestroying() {
}

// NTSC-U/C: 0x0038fe20, PAL: 0x003c16f0
void MetScreen::OnMsgScreenDismissed([[maybe_unused]] const HxStr &name,
                                     [[maybe_unused]] int nChoice) {
}

// NTSC-U/C: 0x0038fe28, PAL: 0x003c16f8
void MetScreen::OnMsgScreenShown([[maybe_unused]] const HxStr &name) {
}

// NTSC-U/C: 0x0038fe30, PAL: 0x003c1700
void MetScreen::HandleCommand([[maybe_unused]] const MetScreenCommand *pCommand) {
}

// NTSC-U/C: 0x0038fe38, PAL: 0x003c1708
void MetScreen::UpdateIdle([[maybe_unused]] float flTime) {
}

// NTSC-U/C: 0x0038fe40, PAL: 0x003c1710
void MetScreen::UpdateIdleAnimation([[maybe_unused]] float flTime) {
}

// NTSC-U/C: 0x00390498, PAL: 0x003c1d68
void MetScreen::StartRepeatingSound(float flStartTime,
                                    float flInterval,
                                    Rnd::Button *pButton,
                                    int nCycles) {
    if (pButton == nullptr) {
        return;
    }
    mRepeatNextTime = flStartTime;
    mRepeatSteps = nCycles * kStepsPerCycle;
    mRepeatInterval = flInterval;
    mRepeatStep = 0;
    mRepeatButton = pButton;
    pButton->SetState(kAlternateState);
}

// NTSC-U/C: 0x003904e0, PAL: 0x003c1db0
void MetScreen::UpdateRepeatingSound(float flTime) {
    if (mRepeatNextTime == 0.0f) {
        return;
    }
    if (!((mRepeatNextTime + mRepeatInterval) < flTime)) {
        return;
    }
    ++mRepeatStep;
    mRepeatButton->SetState((mRepeatStep & 1) != 0 ? kRestState : kAlternateState);
    if (mRepeatStep < mRepeatSteps) {
        mRepeatNextTime = flTime + mRepeatInterval;
        return;
    }
    mRepeatButton->SetState(kRestState);
    OnRepeatingSoundFinished(mRepeatButton);
    mRepeatButton = nullptr;
    mRepeatNextTime = 0.0f;
    mRepeatStep = 0;
}

// NTSC-U/C: 0x0038fe48, PAL: 0x003c1718
void MetScreen::OnRepeatingSoundFinished([[maybe_unused]] Rnd::Button *pButton) {
}

// NTSC-U/C: 0x0038fe50, PAL: 0x003c1720
void MetScreen::OnEnterFinished() {
}

// NTSC-U/C: 0x0038fe58, PAL: 0x003c1728
void MetScreen::OnExitFinished() {
}

// NTSC-U/C: 0x003907a8, PAL: 0x003c2078
void MetScreen::HandleMessage([[maybe_unused]] Message *pMsg) {
}

// NTSC-U/C: 0x00390140, PAL: 0x003c1a10
void MetScreen::PlaySlideSound([[maybe_unused]] int nSelector) {
    PlaySoundByName(kSlideSound);
}

// NTSC-U/C: 0x00390160, PAL: 0x003c1a30
void MetScreen::PlayLeaveSound([[maybe_unused]] int nSelector) {
    PlaySoundByName(kLeaveSound);
}

// NTSC-U/C: 0x003901c0, PAL: 0x003c1a90
void MetScreen::PlayHighSound([[maybe_unused]] int nSelector) {
    PlaySoundByName(kHighSound);
}

// NTSC-U/C: 0x00390180, PAL: 0x003c1a50
void MetScreen::PlayCycleLeftSound([[maybe_unused]] int nSelector) {
    PlaySoundByName(kCycleLeftSound);
}

// NTSC-U/C: 0x003901a0, PAL: 0x003c1a70
void MetScreen::PlayCycleRightSound([[maybe_unused]] int nSelector) {
    PlaySoundByName(kCycleRightSound);
}

// NTSC-U/C: 0x003901e0, PAL: 0x003c1ab0
void MetScreen::PlayErrorSound([[maybe_unused]] int nSelector) {
    PlaySoundByName(kErrorSound);
}

// NTSC-U/C: 0x0038b730, PAL: 0x003bcea8
void MetScreen::DeliverCommand(const MetScreenCommand *pCommand) {
    if (mAcceptsCommands == 0) {
        return;
    }
    if (mPlaysCommandSounds != 0) {
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

// NTSC-U/C: 0x00390788, PAL: 0x003c2058
void MetScreen::Draw() {
    mView->Drawable::Draw();
}

// NTSC-U/C: 0x003906a0, PAL: 0x003c1f70
void MetScreen::StartExitAnimation(float flTime) {
    mExitStartTime = flTime;
    mAcceptsCommands = 0;
    mEnterStartTime = 0.0f;
}

// NTSC-U/C: 0x00390380, PAL: 0x003c1c50
void MetScreen::UpdateAnimationFrame(float flTime) {
    if (mEnterPending != 0) {
        return;
    }
    if (mEnterStartTime == 0.0f && mExitStartTime == 0.0f) {
        UpdateIdleAnimation(flTime);
    }
    if (mEnterStartTime != 0.0f && mEnterAnim != nullptr) {
        const float flEnd = mEnterStartTime + mAnimEndFrame;
        float flFrame = mEnterStartTime + (mAnimEndFrame - flTime);
        if (flEnd < flFrame) {
            flFrame = flEnd;
        }
        mEnterAnim->SetFrame(flFrame);
    }
    if (mExitStartTime != 0.0f && mEnterAnim != nullptr) {
        float flFrame = flTime - mExitStartTime;
        if (mExitStartTime + mAnimEndFrame < flFrame) {
            // Yes, the clamp restores the start time rather than the end frame.
            flFrame = mExitStartTime;
        }
        mEnterAnim->SetFrame(flFrame);
    }
}

// NTSC-U/C: 0x0038b918, PAL: 0x003bd090
void MetScreen::UpdateFrame(float flTime) {
    if (mEnterPending != 0) {
        if (ContainerLoaderMap()[mContainerFile]->mFinished != 0) {
            if (mViewsUnresolved != 0) {
                ResolveContainerViews();
            }
            mRenderer->AddScreenView(mView);
            EnterAndShow();
            mEnterPending = 0;
            if (mActivatePending != 0) {
                mRenderer->SetActivePanel(this);
                mRenderer->mPanelActive = 1;
                OnPanelActivated();
                mActivatePending = 0;
            }
            return;
        }
        float flProgress;
        if (ContainerLoaderMap()[mContainerFile]->mLoader->Poll(&flProgress) != 1) {
            return;
        }
        ContainerLoaderMap()[mContainerFile]->mFinished = 1;
        return;
    }
    UpdateEnterAnimation(flTime);
    if (mIdleWhileAnimating != 0) {
        UpdateIdle(flTime);
    } else if (mEnterStartTime == 0.0f && mExitStartTime == 0.0f) {
        UpdateIdle(flTime);
    }
    UpdateRepeatingSound(flTime);
    UpdateExitAnimation(flTime);
}

// NTSC-U/C: 0x003906b0, PAL: 0x003c1f80
void MetScreen::UpdateExitAnimation(float flTime) {
    if (mExitDone != 0) {
        mExitStartTime = 0.0f;
        mExitDone = 0;
        // Yes, the binary compares the two start times right after clearing one of them, and the
        // clear also makes everything below this block unreachable on this path.
        if (mEnterStartTime != mExitStartTime) {
            return;
        }
        SetShowing(0);
        mRenderer->RemoveScreen(this);
        OnExitFinished();
    }
    if (mExitStartTime == 0.0f) {
        return;
    }
    if (mEnterAnim != nullptr) {
        // Yes, the exit animation drives the enter view and its end frame, not mBackAnim.
        mEnterAnim->SetFrame(flTime - mExitStartTime);
    }
    if (mExitStartTime + mAnimEndFrame < flTime) {
        mExitDone = 1;
    }
}

// NTSC-U/C: 0x003822c8, PAL: 0x003b2948
void MetScreen::DestroyCategory2Screens() {
    std::map<HxStr, MetScreenEntry>::iterator screen = ScreenRegistry().begin();
    while (screen != ScreenRegistry().end()) {
        const MetScreenEntry entry = screen->second;
        if (entry.mCategory == kCategory2) {
            delete entry.mScreen;
            ScreenRegistry().erase(screen++);
        } else {
            ++screen;
        }
    }
}

// NTSC-U/C: 0x00382978, PAL: 0x003b3010
void MetScreen::DestroyCategory1Screens() {
    std::map<HxStr, MetScreenEntry>::iterator screen = ScreenRegistry().begin();
    while (screen != ScreenRegistry().end()) {
        const MetScreenEntry entry = screen->second;
        if (entry.mCategory == kCategory1) {
            delete entry.mScreen;
            ScreenRegistry().erase(screen++);
        } else {
            ++screen;
        }
    }
}

// NTSC-U/C: 0x00383020, PAL: 0x003b36d8
void MetScreen::DestroyNonDefaultScreens() {
    std::map<HxStr, MetScreenEntry>::iterator screen = ScreenRegistry().begin();
    while (screen != ScreenRegistry().end()) {
        const MetScreenEntry entry = screen->second;
        if (entry.mCategory != MetScreenEntry::kDefaultCategory) {
            delete entry.mScreen;
            ScreenRegistry().erase(screen++);
        } else {
            ++screen;
        }
    }
    ZoneResetZone(FindZoneByName(kLocAndNetZone));
}

// NTSC-U/C: 0x00383700, PAL: 0x003b3dc8
void MetScreen::DestroyAllScreens() {
    std::map<HxStr, MetScreenEntry>::iterator screen = ScreenRegistry().begin();
    while (screen != ScreenRegistry().end()) {
        delete screen->second.mScreen;
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

// NTSC-U/C: 0x00381ef8, PAL: 0x003b2578
void MetScreen::PollContainerLoads() {
    for (std::map<HxStr, MetContainerLoad *>::iterator it = ContainerLoaderMap().begin();
         it != ContainerLoaderMap().end();
         ++it) {
        MetContainerLoad *pLoad = it->second;
        if (pLoad->mFinished != 0) {
            continue;
        }
        float flProgress;
        if (pLoad->mLoader->Poll(&flProgress) != kLoadComplete) {
            continue;
        }
        pLoad->mFinished = kLoadComplete;
        std::list<Rnd::Drawable *> draws(pLoad->mLoader->mDrawables);
        for (std::list<Rnd::Drawable *>::iterator draw = draws.begin(); draw != draws.end();
             ++draw) {
            (*draw)->SetShowing(0);
        }
    }
}

// NTSC-U/C: 0x00384300, PAL: 0x003b49f0
void MetScreen::CreateStartupScreens(MetRenderer *pRenderer) {
    int nZone = FindZoneByName(kGlobalZone);
    ScreenRegistry()[HxStr(kSonyScreenKey)] = MetScreenEntry(MetSonyScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kMemDetectStartupKey)] =
        MetScreenEntry(MetMemDetectStartup::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kMsgScreenKey)] = MetScreenEntry(MetMsgScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kLogoScreenKey)] = MetScreenEntry(MetLogoScreen::New(pRenderer, nZone));
}

// NTSC-U/C: 0x003848e0, PAL: 0x003b50b0
void MetScreen::CreateMainMenuScreens(MetRenderer *pRenderer) {
    int nZone = FindZoneByName(kGlobalZone);
    ScreenRegistry()[HxStr(kMainScreenKey)] = MetScreenEntry(MetMainScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kTopLogoScreenKey)] =
        MetScreenEntry(MetTopLogoScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kLeftGizmoSmallScreenKey)] =
        MetScreenEntry(MetLeftGizmoSmallScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kLeftGizmoScreenKey)] =
        MetScreenEntry(MetLeftGizmoScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kHelpScreenKey)] = MetScreenEntry(MetHelpScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kScreenTitleScreenKey)] =
        MetScreenEntry(MetScreenTitleScreen::New(pRenderer, nZone));
}

// NTSC-U/C: 0x00385180, PAL: 0x003b5a98
void MetScreen::CreateFrontEndScreens(MetRenderer *pRenderer) {
    if (ScreenRegistry()[HxStr(kLoadGameScreenKey)].mScreen != nullptr) {
        return;
    }

    int nZone = FindZoneByName(kGlobalZone);
    ScreenRegistry()[HxStr(kLoadPreFabScreenKey)] =
        MetScreenEntry(MetLoadPreFabScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kLoadFreqScreenKey)] =
        MetScreenEntry(MetLoadFreqScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kFreqCreateScreenKey)] =
        MetScreenEntry(MetFreqCreateScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kLoadNewFreqScreenKey)] =
        MetScreenEntry(MetLoadNewFreqScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kModeScreenKey)] = MetScreenEntry(MetModeScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kGameSkillScreenKey)] =
        MetScreenEntry(MetGameSkillScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kRemixTypeScreenKey)] =
        MetScreenEntry(MetRemixTypeScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kSoloStagesScreenKey)] =
        MetScreenEntry(MetSoloStagesScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kArenasScreenKey)] =
        MetScreenEntry(MetArenasScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kFreqMakerButtonsScreenKey)] =
        MetScreenEntry(MetFreqMakerButtonsScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kFreqMakerCanvasScreenKey)] =
        MetScreenEntry(MetFreqMakerCanvasScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kFreqMakerDirectionsScreenKey)] =
        MetScreenEntry(MetFreqMakerDirectionsScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kFreqMakerInventoryScreenKey)] =
        MetScreenEntry(MetFreqMakerInventoryScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kLocNumPlayersScreenKey)] =
        MetScreenEntry(MetLocNumPlayScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kLocPickCharScreenKey)] =
        MetScreenEntry(MetLocPickCharScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kPersonaSaverScreenKey)] =
        MetScreenEntry(MetPersonaSaverScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kGlobalSettingsSaverScreenKey)] =
        MetScreenEntry(MetGlobalSettingsSaverScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kRemixManagerKey)] =
        MetScreenEntry(MetRemixManager::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kKeyboardScreenKey)] =
        MetScreenEntry(MetKeyboardScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kConfigControllerScreenKey)] =
        MetScreenEntry(MetConfigControllerScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kConfigGameOptionsScreenKey)] =
        MetScreenEntry(MetConfigGameOptionsScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kExpansionPakScreenKey)] =
        MetScreenEntry(MetExpansionPakScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kRightGizmoScreenKey)] =
        MetScreenEntry(MetRightGizmoScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kTutorialScreenKey)] =
        MetScreenEntry(MetTutorialScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kMemCardLoadScreenKey)] =
        MetScreenEntry(MetMemCardLoadScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kMemCardTypeScreenKey)] =
        MetScreenEntry(MetMemCardTypeScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kMCFreqDelScreenKey)] =
        MetScreenEntry(MetMCFreqDelScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kRemixDelScreenKey)] =
        MetScreenEntry(MetRemixDelScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kRemixLoadScreenKey)] =
        MetScreenEntry(MetRemixLoadScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kRemixDataScreenKey)] =
        MetScreenEntry(MetRemixDataScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kCreditsScreenKey)] =
        MetScreenEntry(MetCreditsScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kJukeboxTopButtonsScreenKey)] =
        MetScreenEntry(MetJukeboxTopButtonsScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kJukeboxCustomRemixesScreenKey)] =
        MetScreenEntry(MetJukeboxCustomRemixesScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kJukeboxFactoryRemixesScreenKey)] =
        MetScreenEntry(MetJukeboxFactoryRemixesScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kJukeboxEditPlaylistScreenKey)] =
        MetScreenEntry(MetJukeboxEditPlaylistScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kJukeboxEditPlaylistScreenLowerLeftKey)] =
        MetScreenEntry(MetJukeboxEditPlaylistScreenLowerLeft::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kJukeboxEditPlaylistScreenDoneKey)] =
        MetScreenEntry(MetJukeboxEditPlaylistScreenDone::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kConfigOptionsButtonsScreenKey)] =
        MetScreenEntry(MetConfigOptionsButtonsScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kMultiTips1ScreenKey)] =
        MetScreenEntry(MetMultiTips1Screen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kMultiTips2ScreenKey)] =
        MetScreenEntry(MetMultiTips2Screen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kMultiTips3ScreenKey)] =
        MetScreenEntry(MetMultiTips3Screen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kMultiTips4ScreenKey)] =
        MetScreenEntry(MetMultiTips4Screen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kMultiTips5ScreenKey)] =
        MetScreenEntry(MetMultiTips5Screen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kLoadGameScreenKey)] =
        MetScreenEntry(MetLoadGameScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kPauseGameScreenKey)] =
        MetScreenEntry(MetPauseGameScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kPauseSoloGameScreenKey)] =
        MetScreenEntry(MetPauseSoloGameScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kPauseSoloRemixScreenKey)] =
        MetScreenEntry(MetPauseSoloRemixScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kPauseMultiRemixScreenKey)] =
        MetScreenEntry(MetPauseMultiRemixScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kSoloLoseScreenKey)] =
        MetScreenEntry(MetSoloLoseScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kSoloWinScreenKey)] =
        MetScreenEntry(MetSoloWinScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kSoloEndRemixScreenKey)] =
        MetScreenEntry(MetSoloEndRemixScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kMultiEndScreenKey)] =
        MetScreenEntry(MetMultiEndScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kMultiEndRemixScreenKey)] =
        MetScreenEntry(MetMultiEndRemixScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kSoloStatsScreenKey)] =
        MetScreenEntry(MetSoloStatsScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kStageFinishScreenKey)] =
        MetScreenEntry(MetStageFinishScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kMultiStatsScreenKey)] =
        MetScreenEntry(MetMultiStatsScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kSaveRemixScreenKey)] =
        MetScreenEntry(MetSaveRemixScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kMultiSaveRemixScreenKey)] =
        MetScreenEntry(MetMultiSaveRemixScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kEndGameGizmoScreenKey)] =
        MetScreenEntry(MetEndGameGizmoScreen::New(pRenderer, nZone));
}
