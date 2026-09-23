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

// GameManagerImpl::GetGameMode() and GameParams::mUnknown1c values the title reads.
constexpr int kSoloGameMode = 1;
constexpr int kGamePlayMode = 1;

// The game mode in which every arena is unlocked.
constexpr int kUnlockAllGameMode = 2;

// MetScreen::mUnknown18 values the departure records.
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

// 0x001f65a0
MetArenasScreen::MetArenasScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknown8c(nullptr), mUnknown90(nullptr) {
    mUnknown8c = new MetButtonList();
    mUnknown38.push_back(HxStr(kArenasObject));
}

// 0x001fc690
MetArenasScreen *MetArenasScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetArenasScreen(pRenderer, nPriority);
}

// 0x001f70f0
MetArenasScreen::~MetArenasScreen() {
    delete mUnknown8c;
    delete mUnknown90;
}

// 0x001f7750
void MetArenasScreen::EnterAndShow() {
    HxStr mode;
    HxStr kind;
    GameManagerImpl *pManager = Application::shared()->GetGameManager();
    GameParams params(*pManager->GetParams());

    {
        const bool bSolo = Application::shared()->GetGameManager()->GetGameMode() == kSoloGameMode;
        HxStr value = QueryConfigString(kTitleQuery, bSolo ? kSoloKey : kMultiKey);
        mode = value;
    }
    {
        HxStr value = QueryConfigString(kTitleQuery,
                                        params.mUnknown1c == kGamePlayMode ? kGameKey : kRemixKey);
        kind = value;
    }

    const HxStr modeSpaced = Concatenate(mode, ' ');
    const HxStr modeKind = modeSpaced + kind;
    HxStr arenas = QueryConfigString(kTitleQuery, kArenasKey);
    MetScreenTitleScreen::SetTitle(modeKind + arenas);

    mUnknown90->invalidate();
    SetupArenaButtons(MetFrontEndState::shared()->mUnknown14);
    MetHelpScreen::SelectPreset(HxStr(kStandardTitleLayout));
    MetScreen::EnterAndShow();
}

// 0x001f75c8
void MetArenasScreen::UpdateScreenshot() {
    (void)GetArenaList(); // Yes, the binary discards this call's result.
    const int nSelected = mUnknown8c->mSelected;
    Color color;
    if (nSelected < mUnknown98 || nSelected == mUnknown94) {
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
    mUnknown9c->SetEmissive(color);
    const HxStr name((*GetArenaList())[nSelected].mName);
    mUnknown90->Load(TexturePairRecord::ArenaPath(name));
}

// 0x001f7d40
void MetArenasScreen::SetupArenaButtons(int bUnlockAll) {
    mUnknown8c->SetSelected(kNoSelection);
    const int nArenaCount = GetArenaList()->size();
    mUnknowna0->ClearDraws();
    mUnknowna0->ClearTransList();

    const HxStr viewName(FormatString(kButtonViewFormat, nArenaCount));
    Rnd::View *pView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(viewName));
    mUnknowna0->AddDraw(pView, nullptr);
    mUnknowna0->AddTrans(pView);

    const int nButtonCount = mUnknown8c->mButtons.size();
    const int nUnlocked = MetFrontEndState::shared()->GetFirstPersona()->mStats.mUnlockLevel;
    mUnknown98 = std::min(nUnlocked, nArenaCount);
    if (bUnlockAll || Application::shared()->GetGameMode() == kUnlockAllGameMode) {
        mUnknown98 = nArenaCount;
    }
    mUnknown94 = GetArenaList()->size() - 1;

    for (int i = 0; i < nButtonCount; ++i) {
        HxStr label;
        Rnd::Button *pButton = mUnknown8c->ButtonAt(i);
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

        const bool bSelectable = i < mUnknown98 || i == mUnknown94;
        const std::vector<Rnd::Mat *> &mats = bSelectable ? mUnknowna4 : mUnknownb0;
        const std::vector<Rnd::Font *> &fonts = bSelectable ? mUnknownbc : mUnknownc8;
        for (unsigned j = 0; j < mats.size(); ++j) {
            pButton->SetMat(j, mats[j]);
            pButton->SetFont(j, fonts[j]);
        }
        pButton->SetState(bArena ? kButtonStateNormal : kButtonStateDisabled);
    }

    Rnd::Button *pNone = mUnknown8c->ButtonAt(mUnknown94);
    pNone->SetState(kButtonStateNormal);
    {
        HxStr text = QueryConfigString(kArenaNoneQuery, kArenaNoneKey);
        pNone->mText->SetText(text);
    }

    if (bUnlockAll || Application::shared()->GetGameMode() == kUnlockAllGameMode) {
        mUnknown8c->SetSelected(0);
    } else {
        mUnknown8c->SetSelected(mUnknown98 - 1);
    }
    UpdateScreenshot();

    Rnd::Mesh *pPanel = dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr(kArenaPanelMesh)));
    pPanel->SetShowing(kHidden);
}

// 0x001f7310
void MetArenasScreen::HandleCommand(const MetScreenCommand *pCommand) {
    const int nSelected = mUnknown8c->mSelected;
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mUnknown8c->OnUnknownSlot2();
        UpdateScreenshot();
        break;

    case kMetScreenCommandNext:
        mUnknown8c->OnUnknownSlot3();
        UpdateScreenshot();
        break;

    case kMetScreenCommandSelect:
        if (!(nSelected < mUnknown98) && nSelected != mUnknown94) {
            return;
        }
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        ActivateNamedPanel(HxStr(kNoName));
        StartRepeatingSound(mUnknown10->mUnknown68,
                            kSelectAlternateInterval,
                            mUnknown8c->mUnknown00,
                            kSelectAlternateCycles);
        break;

    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        mUnknown18 = kExitBack;
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// 0x001fc718
void MetArenasScreen::PlaySlideSound(int nSelector) {
    if (mUnknown8c->mSelected < mUnknown98 || mUnknown8c->mSelected == mUnknown94) {
        MetScreen::PlaySlideSound(nSelector);
    }
}

// 0x001fc680
void MetArenasScreen::PlayCycleLeftSound(int) {
}

// 0x001fc688
void MetArenasScreen::PlayCycleRightSound(int) {
}

// 0x001f8af0
void MetArenasScreen::OnUnknownSlot26(float) {
    mUnknown90->Advance(); // Yes, the binary discards whether the pair advanced.
    Rnd::Mesh *pPanel = dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr(kArenaPanelMesh)));
    pPanel->SetShowing(kHidden);
    if (mUnknown90->Current() == nullptr) {
        return;
    }
    pPanel->SetShowing(kShown);
    mUnknown9c->mStages[0].SetTex(mUnknown90->Current());
}

// 0x001f8378
void MetArenasScreen::OnUnknownSlot30(Rnd::Button *) {
    const int nSelected = mUnknown8c->mSelected;
    (void)GetArenaList(); // Yes, the binary discards this call's result.
    if (!(nSelected < mUnknown98) && nSelected != mUnknown94) {
        ActivateNamedPanel(HxStr(kThisScreen));
        return;
    }
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    params.mArenaName = (*GetArenaList())[nSelected].mName;
    Application::shared()->GetGameManager()->SetParams(params);
    mUnknown18 = kExitToArena;
    ExitScreenByName(HxStr(kHelpScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    BeginExit();
}

// 0x001fc758
void MetArenasScreen::OnUnknownSlot33() {
    MetHelpScreen::SetText(mUnknown38[0], mUnknown10->mUnknown68);
}

// 0x001f8658
void MetArenasScreen::OnUnknownSlot36() {
    if (mUnknown18 == kExitBack) {
        if (MetFrontEndState::shared()->mUnknown24 == kRemixLoadScreen) {
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
        MetFrontEndState::shared()->mUnknown24 = HxStr(kThisScreen);
        PushNamedScreen(HxStr(kLoadGameScreen));
        ActivateNamedPanel(HxStr(kLoadGameScreen));
    }
    mUnknown8c->SetSelected(kNoSelection);
}

// 0x001f6a08
void MetArenasScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    // Yes, the binary empties mUnknownbc twice and never empties mUnknowna4.
    mUnknownbc.erase(mUnknownbc.begin(), mUnknownbc.end());
    mUnknownbc.erase(mUnknownbc.begin(), mUnknownbc.end());
    mUnknownb0.erase(mUnknownb0.begin(), mUnknownb0.end());
    mUnknownc8.erase(mUnknownc8.begin(), mUnknownc8.end());

    Rnd::Button *pUnlocked =
        dynamic_cast<Rnd::Button *>(Rnd::g_manager.Find(HxStr(kUnlockedButton)));
    for (int i = 0; i < kPaletteSize; ++i) {
        mUnknowna4.push_back(pUnlocked->mMats[i]);
        mUnknownbc.push_back(pUnlocked->mFonts[i]);
    }
    Rnd::Button *pLocked = dynamic_cast<Rnd::Button *>(Rnd::g_manager.Find(HxStr(kLockedButton)));
    for (int i = 0; i < kPaletteSize; ++i) {
        mUnknownb0.push_back(pLocked->mMats[i]);
        mUnknownc8.push_back(pLocked->mFonts[i]);
    }

    for (int i = kFirstButton; i <= kLastButton; ++i) {
        mUnknown8c->Add(HxStr(FormatString(kButtonFormat, i)), HxStr(kNoName));
    }

    mUnknown9c = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr(kScreenshotMaterial)));
    mUnknowna0 = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kButtonView)));
    mUnknown90 = new TexturePairRecord(HxStr(kFirstScreenshot), HxStr(kSecondScreenshot));
}
