#include "met/metarenasscreen.h"

#include <algorithm>

#include "app/application.h"
#include "game/campaignstats.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "math/color.h"
#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metpersonadata.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "met/metsonglists.h"
#include "met/metstrings.h"
#include "met/texturepairrecord.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/button.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/text.h"
#include "rnd/view.h"
#include "script/configquery.h"

#ifdef VIDEO_STANDARD_PAL
#include <libscf.h>

#include "os/hostmode.h"
#endif

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "as";
// The directory the container loads from.
static const char *const kDirectory = "metagame/_Solo";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "arena_sel";

// The extra container object the constructor asks MetScreen to resolve.
static const char *const kArenasObject = "arenas";

// The objects ResolveContainerViews() resolves.
static const char *const kUnlockedButton = "as_01.but";
static const char *const kLockedButton = "as_02.but";
static const char *const kButtonFormat = "as_0%d.but";
static const char *const kScreenshotMaterial = "as_screenshot.mat";
static const char *const kButtonView = "arena_butts.view";
static const char *const kButtonViewFormat = "arena_%d_buts.view";
static const char *const kArenaPanelMesh = "as_arena_panel.mesh";

// The two screenshot textures.
static const char *const kFirstScreenshot = "gArena1.tex";
static const char *const kSecondScreenshot = "gArena2.tex";

// The keys of configuration code kTitleQuery.
static const char *const kSoloKey = "solo";
static const char *const kMultiKey = "multi";
static const char *const kGameKey = "game";
static const char *const kRemixKey = "remix";
static const char *const kArenasKey = "arenas";

static const char *const kStandardTitleLayout = "standard_title";
static const char *const kArenaNoneKey = "arena_none";
static const char *const kNoName = "";

// The screens this one exits or goes on to.
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kThisScreen = "MetArenasScreen";
static const char *const kRemixLoadScreen = "MetRemixLoadScreen";
static const char *const kRemixDataScreen = "MetRemixDataScreen";
static const char *const kSoloStagesScreen = "MetSoloStagesScreen";
static const char *const kLoadGameScreen = "MetLoadGameScreen";

// Configuration codes this screen reads.
constexpr int kTitleQuery = 0x269;
constexpr int kArenaNameQuery = 0x326;
constexpr int kArenaNoneQuery = 0x258;

// The buttons the ring holds, as_01.but through as_09.but.
constexpr int kFirstButton = 1;
constexpr int kLastButton = 9;

// The palette entries ResolveContainerViews() copies out of each template button.
constexpr int kPaletteSize = 4;

// GameManagerImpl::GetGameMode() and GameParams::mPlayMode values the title reads.
constexpr int kSoloGameMode = 1;
constexpr int kGamePlayMode = 1;

// The game mode in which every arena is unlocked.
constexpr int kUnlockAllGameMode = 2;

// MetScreen::mExitChoice values the departure records.
constexpr int kExitBack = 0;
constexpr int kExitToArena = 2;

// The sentinel MetButtonList::SetSelected() takes for no selection.
constexpr int kNoSelection = -1;

// Rnd::Button states.
constexpr int kButtonStateNormal = 0;
constexpr int kButtonStateDisabled = 3;

// Rnd::Drawable::SetShowing() values.
constexpr int kHidden = 0;
constexpr int kShown = 1;

// The alternation command 5 starts on the selected button.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

// The screenshot colours for a selectable entry and for a disabled one.
constexpr float kScreenshotFull = 1.0f;
constexpr float kScreenshotDimmed = 0.3f;

// Concatenates a string and one character, the inlined shape every call site of the character
// operator+=() shows.
inline HxStr Concatenate(const HxStr &left, char ch) {
    HxStr result(left);
    result += ch;
    return result;
}

} // namespace

// NTSC-U/C: 0x001f65a0, PAL: 0x001fcfc8
MetArenasScreen::MetArenasScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mArenaButtons(nullptr), mScreenshots(nullptr) {
    mArenaButtons = new MetButtonList();
    mHelpKeys.push_back(MetText(kMetStrHArenas, kArenasObject));
}

// NTSC-U/C: 0x001fc690, PAL: 0x00203b20
MetArenasScreen *MetArenasScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetArenasScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x001f70f0, PAL: 0x001fdc98
MetArenasScreen::~MetArenasScreen() {
    delete mArenaButtons;
    delete mScreenshots;
}

// NTSC-U/C: 0x001f7750, PAL: 0x001fe3b8
void MetArenasScreen::EnterAndShow() {
    HxStr mode;
    HxStr kind;
    GameManagerImpl *pManager = Application::shared()->GetGameManager();
    GameParams params(*pManager->GetParams());

    {
        const bool bSolo = Application::shared()->GetGameManager()->GetGameMode() == kSoloGameMode;
        HxStr value = bSolo ? MetConfigText(kMetStrTSolo, kTitleQuery, kSoloKey) :
                              MetConfigText(kMetStrTMulti, kTitleQuery, kMultiKey);
        mode = value;
    }
    {
        HxStr value = params.mPlayMode == kGamePlayMode ?
                          MetConfigText(kMetStrTGame, kTitleQuery, kGameKey) :
                          MetConfigText(kMetStrTRemix, kTitleQuery, kRemixKey);
        kind = value;
    }

#ifdef VIDEO_STANDARD_PAL
    // Spanish places the kind before the mode.
    const bool bKindFirst = GetLanguage() == SCE_SPANISH_LANGUAGE;
    const HxStr modeSpaced = Concatenate(bKindFirst ? kind : mode, ' ');
    const HxStr modeKind = modeSpaced + (bKindFirst ? mode : kind);
#else
    const HxStr modeSpaced = Concatenate(mode, ' ');
    const HxStr modeKind = modeSpaced + kind;
#endif
    HxStr arenas = MetConfigText(kMetStrTArenas, kTitleQuery, kArenasKey);
    MetScreenTitleScreen::SetTitle(modeKind + arenas);

    mScreenshots->invalidate();
    SetupArenaButtons(MetFrontEndState::shared()->mUnlockAll);
    MetHelpScreen::SelectPreset(MetText(kMetStrHStandardTitle, kStandardTitleLayout));
    MetScreen::EnterAndShow();
}

// NTSC-U/C: 0x001f75c8, PAL: 0x001fe1f0
void MetArenasScreen::UpdateScreenshot() {
    (void)GetArenaList(); // Yes, the binary discards this call's result.
    const int nSelected = mArenaButtons->mSelected;
    Color color;
    if (nSelected < mUnlockedCount || nSelected == mNoArenaIndex) {
        color.r = kScreenshotFull;
        color.g = kScreenshotFull;
        color.b = kScreenshotFull;
        color.a = kScreenshotFull;
    } else {
        color.r = kScreenshotDimmed;
        color.g = kScreenshotDimmed;
        color.b = kScreenshotDimmed;
        color.a = kScreenshotFull;
    }
    mScreenshotMat->SetEmissive(color);
    const HxStr name((*GetArenaList())[nSelected].mName);
    mScreenshots->Load(TexturePairRecord::ArenaPath(name));
}

// NTSC-U/C: 0x001f7d40, PAL: 0x001fee58
void MetArenasScreen::SetupArenaButtons(int bUnlockAll) {
    mArenaButtons->SetSelected(kNoSelection);
    const int nArenaCount = GetArenaList()->size();
    mButtonView->ClearDraws();
    mButtonView->ClearTransList();

    const HxStr viewName(FormatString(kButtonViewFormat, nArenaCount));
    Rnd::View *pView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(viewName));
    mButtonView->AddDraw(pView, nullptr);
    mButtonView->AddTrans(pView);

    const int nButtonCount = mArenaButtons->mButtons.size();
    const int nUnlocked = MetFrontEndState::shared()->GetFirstPersona()->mStats.mUnlockLevel;
    mUnlockedCount = std::min(nUnlocked, nArenaCount);
    if (bUnlockAll || Application::shared()->GetGameMode() == kUnlockAllGameMode) {
        mUnlockedCount = nArenaCount;
    }
    mNoArenaIndex = GetArenaList()->size() - 1;

    for (int i = 0; i < nButtonCount; ++i) {
        HxStr label;
        Rnd::Button *pButton = mArenaButtons->ButtonAt(i);
        const bool bArena = i < nArenaCount;
        if (bArena) {
            const HxStr &arena = (*GetArenaList())[i].mName;
            HxStr value = QueryConfigString(kArenaNameQuery,
                                            arena.mStr != nullptr ? arena.mStr : g_szEmptyString);
            label = value;
        } else {
            label = kNoName;
        }
        pButton->mText->SetText(label);

        const bool bSelectable = i < mUnlockedCount || i == mNoArenaIndex;
        const std::vector<Rnd::Mat *> &mats = bSelectable ? mUnlockedMats : mLockedMats;
        const std::vector<Rnd::Font *> &fonts = bSelectable ? mUnlockedFonts : mLockedFonts;
        for (unsigned j = 0; j < mats.size(); ++j) {
            pButton->SetMat(j, mats[j]);
            pButton->SetFont(j, fonts[j]);
        }
        pButton->SetState(bArena ? kButtonStateNormal : kButtonStateDisabled);
    }

    Rnd::Button *pNone = mArenaButtons->ButtonAt(mNoArenaIndex);
    pNone->SetState(kButtonStateNormal);
    {
        HxStr text = MetConfigText(kMetStrArenaNone, kArenaNoneQuery, kArenaNoneKey);
        pNone->mText->SetText(text);
    }

    if (bUnlockAll || Application::shared()->GetGameMode() == kUnlockAllGameMode) {
        mArenaButtons->SetSelected(0);
    } else {
        mArenaButtons->SetSelected(mUnlockedCount - 1);
    }
    UpdateScreenshot();

    Rnd::Mesh *pPanel = dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr(kArenaPanelMesh)));
    pPanel->SetShowing(kHidden);
}

// NTSC-U/C: 0x001f7310, PAL: 0x001fdeb8
void MetArenasScreen::HandleCommand(const MetScreenCommand *pCommand) {
    const int nSelected = mArenaButtons->mSelected;
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mArenaButtons->SelectPrevious();
        UpdateScreenshot();
        break;

    case kMetScreenCommandNext:
        mArenaButtons->SelectNext();
        UpdateScreenshot();
        break;

    case kMetScreenCommandSelect:
        if (!(nSelected < mUnlockedCount) && nSelected != mNoArenaIndex) {
            return;
        }
        MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
        ActivateNamedPanel(HxStr(kNoName));
        StartRepeatingSound(mRenderer->mAnimationFrame,
                            kSelectAlternateInterval,
                            mArenaButtons->mSelectedButton,
                            kSelectAlternateCycles);
        break;

    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
        mExitChoice = kExitBack;
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// NTSC-U/C: 0x001fc718, PAL: 0x00203ba8
void MetArenasScreen::PlaySlideSound(int nSelector) {
    if (mArenaButtons->mSelected < mUnlockedCount || mArenaButtons->mSelected == mNoArenaIndex) {
        MetScreen::PlaySlideSound(nSelector);
    }
}

// NTSC-U/C: 0x001fc680, PAL: 0x00203b10
void MetArenasScreen::PlayCycleLeftSound(int) {
}

// NTSC-U/C: 0x001fc688, PAL: 0x00203b18
void MetArenasScreen::PlayCycleRightSound(int) {
}

// NTSC-U/C: 0x001f8af0, PAL: 0x001ffe58
void MetArenasScreen::UpdateIdle(float) {
    mScreenshots->Advance(); // Yes, the binary discards whether the pair advanced.
    Rnd::Mesh *pPanel = dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr(kArenaPanelMesh)));
    pPanel->SetShowing(kHidden);
    if (mScreenshots->Current() == nullptr) {
        return;
    }
    pPanel->SetShowing(kShown);
    mScreenshotMat->mStages[0].SetTex(mScreenshots->Current());
}

// NTSC-U/C: 0x001f8378, PAL: 0x001ff518
void MetArenasScreen::OnRepeatingSoundFinished(Rnd::Button *) {
    const int nSelected = mArenaButtons->mSelected;
    (void)GetArenaList(); // Yes, the binary discards this call's result.
    if (!(nSelected < mUnlockedCount) && nSelected != mNoArenaIndex) {
        ActivateNamedPanel(HxStr(kThisScreen));
        return;
    }
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    params.mArenaName = (*GetArenaList())[nSelected].mName;
    Application::shared()->GetGameManager()->SetParams(params);
    mExitChoice = kExitToArena;
    ExitScreenByName(HxStr(kHelpScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    BeginExit();
}

// NTSC-U/C: 0x001fc758, PAL: 0x00203be8
void MetArenasScreen::OnEnterFinished() {
    MetHelpScreen::SetText(mHelpKeys[0], mRenderer->mAnimationFrame);
}

// NTSC-U/C: 0x001f8658, PAL: 0x001ff8a0
void MetArenasScreen::OnExitFinished() {
    if (mExitChoice == kExitBack) {
        if (MetFrontEndState::shared()->mReturnScreen == kRemixLoadScreen) {
            PushNamedScreen(HxStr(kTitleScreen));
            PushNamedScreen(HxStr(kRemixDataScreen));
            PushNamedScreen(HxStr(kHelpScreen));
            PushNamedScreen(HxStr(kRemixLoadScreen));
            ActivateNamedPanel(HxStr(kRemixLoadScreen));
        } else {
            PushNamedScreen(HxStr(kSoloStagesScreen));
            ActivateNamedPanel(HxStr(kSoloStagesScreen));
        }
    } else {
        MetFrontEndState::shared()->mReturnScreen = HxStr(kThisScreen);
        PushNamedScreen(HxStr(kLoadGameScreen));
        ActivateNamedPanel(HxStr(kLoadGameScreen));
    }
    mArenaButtons->SetSelected(kNoSelection);
}

// NTSC-U/C: 0x001f6a08, PAL: 0x001fd4a8
void MetArenasScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    // Yes, the binary empties mUnlockedFonts twice and never empties mUnlockedMats.
    mUnlockedFonts.erase(mUnlockedFonts.begin(), mUnlockedFonts.end());
    mUnlockedFonts.erase(mUnlockedFonts.begin(), mUnlockedFonts.end());
    mLockedMats.erase(mLockedMats.begin(), mLockedMats.end());
    mLockedFonts.erase(mLockedFonts.begin(), mLockedFonts.end());

    Rnd::Button *pUnlocked =
        dynamic_cast<Rnd::Button *>(Rnd::g_manager.Find(HxStr(kUnlockedButton)));
    for (int i = 0; i < kPaletteSize; ++i) {
        mUnlockedMats.push_back(pUnlocked->mMats[i]);
        mUnlockedFonts.push_back(pUnlocked->mFonts[i]);
    }
    Rnd::Button *pLocked = dynamic_cast<Rnd::Button *>(Rnd::g_manager.Find(HxStr(kLockedButton)));
    for (int i = 0; i < kPaletteSize; ++i) {
        mLockedMats.push_back(pLocked->mMats[i]);
        mLockedFonts.push_back(pLocked->mFonts[i]);
    }

    for (int i = kFirstButton; i <= kLastButton; ++i) {
        mArenaButtons->Add(HxStr(FormatString(kButtonFormat, i)), HxStr(kNoName));
    }

    mScreenshotMat = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr(kScreenshotMaterial)));
    mButtonView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kButtonView)));
    mScreenshots = new TexturePairRecord(HxStr(kFirstScreenshot), HxStr(kSecondScreenshot));
}
