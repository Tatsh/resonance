#include "met/metscreen.h"

#include <map>

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

std::map<HxStr, MetScreenEntry> &MetScreen::ScreenRegistry() {
    static std::map<HxStr, MetScreenEntry> theMap;
    return theMap;
}

MetScreen *MetScreen::FindScreenByName(const HxStr &name) {
    MetScreen *pScreen = nullptr;
    std::map<HxStr, MetScreenEntry>::iterator it = ScreenRegistry().find(name);
    if (it != ScreenRegistry().end()) {
        pScreen = (*it).second.mScreen;
    }
    return pScreen;
}

// 0x00390000
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

// 0x00390498
void MetScreen::StartRepeatingSound(float flStartTime,
                                    float flInterval,
                                    Rnd::Button *pButton,
                                    int nCycles) {
    if (pButton == nullptr) {
        return;
    }
    mUnknown64 = flStartTime;
    mUnknown70 = nCycles * kStepsPerCycle;
    mUnknown74 = flInterval;
    mUnknown6c = 0;
    mUnknown68 = pButton;
    pButton->SetState(kAlternateState);
}

// 0x003904e0
void MetScreen::UpdateRepeatingSound(float flTime) {
    if (mUnknown64 == 0.0f) {
        return;
    }
    if (!((mUnknown64 + mUnknown74) < flTime)) {
        return;
    }
    ++mUnknown6c;
    mUnknown68->SetState((mUnknown6c & 1) != 0 ? kRestState : kAlternateState);
    if (mUnknown6c < mUnknown70) {
        mUnknown64 = flTime + mUnknown74;
        return;
    }
    mUnknown68->SetState(kRestState);
    OnUnknownSlot30(mUnknown68);
    mUnknown68 = nullptr;
    mUnknown64 = 0.0f;
    mUnknown6c = 0;
}

void MetScreen::OnUnknownSlot30([[maybe_unused]] Rnd::Button *pButton) {
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

// 0x003822c8
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

// 0x00382978
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

// 0x00383020
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

// 0x00383700
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
    ScreenRegistry()[HxStr(kSonyScreenKey)] = MetScreenEntry(MetSonyScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kMemDetectStartupKey)] =
        MetScreenEntry(MetMemDetectStartup::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kMsgScreenKey)] = MetScreenEntry(MetMsgScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kLogoScreenKey)] = MetScreenEntry(MetLogoScreen::New(pRenderer, nZone));
}

// 0x003848e0
void MetScreen::CreateMainMenuScreens(MetRenderer *pRenderer) {
    int nZone = FindZoneByName(kGlobalZone);
    ScreenRegistry()[HxStr(kMainScreenKey)] = MetScreenEntry(MetMainScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kTopLogoScreenKey)] =
        MetScreenEntry(new MetTopLogoScreen(pRenderer, nZone));
    ScreenRegistry()[HxStr(kLeftGizmoSmallScreenKey)] =
        MetScreenEntry(MetLeftGizmoSmallScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kLeftGizmoScreenKey)] =
        MetScreenEntry(MetLeftGizmoScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kHelpScreenKey)] = MetScreenEntry(MetHelpScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kScreenTitleScreenKey)] =
        MetScreenEntry(MetScreenTitleScreen::New(pRenderer, nZone));
}

// 0x00385180
void MetScreen::CreateFrontEndScreens(MetRenderer *pRenderer) {
    if (ScreenRegistry()[HxStr(kLoadGameScreenKey)].mScreen != nullptr) {
        return;
    }

    // Five of the classes have no factory of their own, so their `new` expressions stand here,
    // and each one is emitted out of line in its class's unit.
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
        MetScreenEntry(new MetLocNumPlayScreen(pRenderer, nZone));
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
        MetScreenEntry(new MetTutorialScreen(pRenderer, nZone));
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
        MetScreenEntry(new MetStageFinishScreen(pRenderer, nZone));
    ScreenRegistry()[HxStr(kMultiStatsScreenKey)] =
        MetScreenEntry(MetMultiStatsScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kSaveRemixScreenKey)] =
        MetScreenEntry(MetSaveRemixScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kMultiSaveRemixScreenKey)] =
        MetScreenEntry(MetMultiSaveRemixScreen::New(pRenderer, nZone));
    ScreenRegistry()[HxStr(kEndGameGizmoScreenKey)] =
        MetScreenEntry(MetEndGameGizmoScreen::New(pRenderer, nZone));
}
