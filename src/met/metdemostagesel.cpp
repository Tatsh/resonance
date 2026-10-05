#include "met/metdemostagesel.h"

#ifdef VIDEO_STANDARD_PAL
#include <list>

#include <libscf.h>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "met/metsonglists.h"
#include "met/metstrings.h"
#include "met/texturepairrecord.h"
#include "os/formatstring.h"
#include "os/hostmode.h"
#include "os/hxstr.h"
#include "rnd/button.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/object.h"
#include "rnd/tex.h"
#include "rnd/text.h"
#include "rnd/view.h"
#include "script/configquery.h"
#include "script/scripthost.h"

namespace {

// The screen name, the directory the container loads from, and the container name.
static const char *const kScreenName = "ss";
static const char *const kDirectory = "metagame/demo";
static const char *const kContainerName = "stage_sel_demo";

// The texture pairs and the objects ResolveContainerViews() stores.
static const char *const kFirstLogoTex = "gSongLogo1.tex";
static const char *const kSecondLogoTex = "gSongLogo2.tex";
static const char *const kFirstLabelTex = "gSongLabel1.tex";
static const char *const kSecondLabelTex = "gSongLabel2.tex";
static const char *const kGenreText = "ss_genre_bpm.txt";
static const char *const kTvLabelMesh = "sstv_label_right.mesh";
static const char *const kTvLogoMesh = "sstv_logo right.mesh";
static const char *const kBioText = "ss_bio.txt";
static const char *const kLabelText = "ss_label.txt";

// The view EnterAndShow() fills, the two button views, and the buttons in list order.
static const char *const kStageButtonsView = "stage_buts.view";
static const char *const kRemixButtonView = "stage_3buts.view";
static const char *const kGameButtonView = "stage_5buts.view";
static const char *const kStageButtons[] = {
    "ss_stage1.but", "ss_stage2.but", "ss_stage3.but", "ss_stage4.but", "ss_stage5.but"};
constexpr int kRemixButtonCount = 3;
constexpr int kGameButtonCount = 5;

// The button EnterAndShow() selects, the tutorial button, and the first of the two songs played at
// the normal difficulty and in the third arena.
constexpr int kFirstSongButton = 1;
constexpr int kTutorialButton = 0;
constexpr int kFirstAdvancedButton = 3;

// The tutorial level, the suffix of each translated tutorial, and the empty literal.
static const char *const kTutorialLevel = "tutorial";
static const char *const kTutorialRemixLevel = "tutorialrmx";
static const char *const kGermanSuffix = "_ger";
static const char *const kFrenchSuffix = "_fre";
static const char *const kItalianSuffix = "_ita";
static const char *const kSpanishSuffix = "_spa";
static const char *const kNoText = "";

// Arenas of the GetArenaList() table.
constexpr int kFirstArena = 0;
constexpr int kThirdArena = 2;

// Difficulties, from GameParams::mDifficulty.
constexpr int kDifficultyEasy = 0;
constexpr int kDifficultyNormal = 1;

// The script template a song selection runs, and its argument.
constexpr int kSelectTemplate = 615;
static const char *const kSelectTemplateArgument = "0";

// The mExitChoice values HandleCommand() writes and BeginExit() and OnExitFinished() test.
constexpr int kExitBack = 0;
constexpr int kExitSelected = 2;

// Screens the class pushes, activates, exits, and records by registry key.
static const char *const kOwnScreenName = "MetDEMOStageSel";
static const char *const kTutorialScreen = "MetTutorialScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kModeScreen = "MetModeScreen";
static const char *const kLoadGameScreen = "MetLoadGameScreen";

// The material stage the television textures go to.
constexpr int kTexStage = 0;

// The texts ShowSelectedSong() formats, and the configuration codes it reads.
static const char *const kGenreSeparator = " --- ";
static const char *const kBpmSuffix = " bpm";
static const char *const kLabelFormat = "%s, %s";
constexpr int kArtistConfigCode = 800;
constexpr int kGenreConfigCode = 801;
constexpr int kBpmConfigCode = 802;
constexpr int kBioConfigCode = 803;
constexpr int kTitleConfigCode = 805;

// Reports the text of a string, or the shared empty string when it has no buffer.
inline const char *TextOf(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// Resolves a registry key to an object of type T, or null.
template <typename T>
T *FindObject(const char *pszName) {
    Rnd::Object *pObject = Rnd::TheManager.Find(HxStr(pszName));
    return pObject != nullptr ? dynamic_cast<T *>(pObject) : nullptr;
}

// Reports a copy of the game manager's parameters.
inline GameParams CurrentParams() {
    return GameParams(*Application::shared()->GetGameManager()->GetParams());
}

// Replaces the contents of `stage_buts.view` with one of the two button views.
inline void ShowButtonView(const char *pszButtonView) {
    Rnd::View *pView = FindObject<Rnd::View>(kStageButtonsView);
    pView->RemoveAllAnims();
    pView->RemoveAllDraws();
    pView->RemoveAllTranses();
    Rnd::View *pButtons = FindObject<Rnd::View>(pszButtonView);
    pView->AddAnim(pButtons);
    pView->AddTrans(pButtons);
    std::list<Rnd::Drawable *> &draws = pView->GetDraws();
    pView->AddDraw(pButtons, draws.empty() ? nullptr : draws.front());
}

// Shows a television texture on a mesh.
inline void ShowTvTexture(Rnd::Mesh *pMesh, Rnd::Tex *pTex) {
    pMesh->SetShowing(1);
    pMesh->mMat->mStages[kTexStage].SetTex(pTex);
}

} // namespace

MetDEMOStageSel::MetDEMOStageSel(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mLogoPair(nullptr), mLabelPair(nullptr), mLoadPending(0), mStageList(nullptr),
      mGenreText(nullptr), mTvLogoMesh(nullptr), mTvLabelMesh(nullptr), mBioText(nullptr),
      mLabelText(nullptr) {
}

MetDEMOStageSel::~MetDEMOStageSel() {
    delete mLogoPair;
    delete mLabelPair;
    delete mStageList;
}

MetDEMOStageSel *MetDEMOStageSel::New(MetRenderer *pRenderer, int nPriority) {
    return new MetDEMOStageSel(pRenderer, nPriority);
}

void MetDEMOStageSel::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mLogoPair = new TexturePairRecord(HxStr(kFirstLogoTex), HxStr(kSecondLogoTex));
    mLabelPair = new TexturePairRecord(HxStr(kFirstLabelTex), HxStr(kSecondLabelTex));
    mStageList = new MetButtonList();
    mGenreText = FindObject<Rnd::Text>(kGenreText);
    mTvLabelMesh = FindObject<Rnd::Mesh>(kTvLabelMesh);
    mTvLogoMesh = FindObject<Rnd::Mesh>(kTvLogoMesh);
    mBioText = FindObject<Rnd::Text>(kBioText);
    mLabelText = FindObject<Rnd::Text>(kLabelText);
}

void MetDEMOStageSel::EnterAndShow() {
    MetScreen::EnterAndShow();
    mLabelPair->invalidate();
    mLogoPair->invalidate();
    mTvLabelMesh->SetShowing(0);
    mTvLogoMesh->SetShowing(0);
    mHelpKeys.clear();

    const GameParams params(CurrentParams());
    const bool bRemix = params.mPlayMode == kPlayModeJam;
    const int nButtonCount = bRemix ? kRemixButtonCount : kGameButtonCount;
    ShowButtonView(bRemix ? kRemixButtonView : kGameButtonView);
    mStageList->Clear();
    for (int i = 0; i < nButtonCount; ++i) {
        mStageList->Add(HxStr(kStageButtons[i]), HxStr(kNoText));
    }
    mStageList->SetSelected(kFirstSongButton);

    if (bRemix) {
        mHelpKeys.push_back(GetMetString(kMetStrHTutR));
        mHelpKeys.push_back(GetMetString(kMetStrHDEMORemixSelectButton1));
        mHelpKeys.push_back(GetMetString(kMetStrHDEMORemixSelectButton2));
        for (int i = 0; i < kRemixButtonCount; ++i) {
            mStageList->GetButton(i)->mText->SetText(
                GetMetString(kMetStrDEMORemixSelectButton0Text + i));
        }
        MetScreenTitleScreen::SetTitle(GetMetString(kMetStrTDEMORemixTitleText));
    } else {
        mHelpKeys.push_back(GetMetString(kMetStrHTutG));
        mHelpKeys.push_back(GetMetString(kMetStrHDEMOGameSelectButton1));
        mHelpKeys.push_back(GetMetString(kMetStrHDEMOGameSelectButton2));
        mHelpKeys.push_back(GetMetString(kMetStrHDEMOGameSelectButton3));
        mHelpKeys.push_back(GetMetString(kMetStrHDEMOGameSelectButton4));
        for (int i = 0; i < kGameButtonCount; ++i) {
            mStageList->GetButton(i)->mText->SetText(
                GetMetString(kMetStrDEMOGameSelectButton0Text + i));
        }
        MetScreenTitleScreen::SetTitle(GetMetString(kMetStrTDEMOGameTitleText));
    }
    ShowSelectedSong();
}

void MetDEMOStageSel::BeginExit() {
    MetScreen::BeginExit();
    ExitScreenByName(HxStr(kTitleScreen));
    if (mExitChoice == kExitSelected) {
        ExitScreenByName(HxStr(kHelpScreen));
    }
}

void MetDEMOStageSel::OnExitFinished() {
    if (mExitChoice == kExitBack) {
        PushNamedScreen(HxStr(kTitleScreen));
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kModeScreen));
        ActivateNamedPanel(HxStr(kModeScreen));
    } else if (mExitChoice == kExitSelected) {
        PushNamedScreen(HxStr(kLoadGameScreen));
        ActivateNamedPanel(HxStr(kLoadGameScreen));
    }
}

void MetDEMOStageSel::UpdateIdle(float) {
    if (mLoadPending == 0) {
        return;
    }
    bool bLogoShown = false;
    bool bLabelShown = false;
    if (mLogoPair->Advance() != 0) {
        mTvLogoMesh->SetShowing(0);
        Rnd::Tex *pTex = mLogoPair->Current();
        if (pTex != nullptr) {
            bLogoShown = true;
            ShowTvTexture(mTvLogoMesh, pTex);
        }
    }
    if (mLabelPair->Advance() != 0) {
        mTvLabelMesh->SetShowing(0);
        Rnd::Tex *pTex = mLabelPair->Current();
        if (pTex != nullptr) {
            bLabelShown = true;
            ShowTvTexture(mTvLabelMesh, pTex);
        }
    }
    if (bLabelShown && bLogoShown) {
        mLoadPending = 0;
    }
}

void MetDEMOStageSel::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
    case kMetScreenCommandNext:
        if (pCommand->mCommand == kMetScreenCommandPrevious) {
            mStageList->SelectPrevious();
        } else {
            mStageList->SelectNext();
        }
        MetHelpScreen::SetText(mHelpKeys[mStageList->mSelected], mRenderer->mAnimationFrame);
        ShowSelectedSong();
        break;

    case kMetScreenCommandSelect: {
        ActivateNamedPanel(HxStr(kNoText));
        MetHelpScreen::SetText(HxStr(kNoText), mRenderer->mAnimationFrame);
        const int nButton = mStageList->mSelected;
        if (nButton == kTutorialButton) {
            Application::shared()->GetGameManager()->SetGameMode(kGameModeSolo);
            GameParams params(CurrentParams());
            HxStr level;
            HxStr suffix(kNoText);
            switch (GetLanguage()) {
            case SCE_GERMAN_LANGUAGE:
                suffix = kGermanSuffix;
                break;
            case SCE_FRENCH_LANGUAGE:
                suffix = kFrenchSuffix;
                break;
            case SCE_ITALIAN_LANGUAGE:
                suffix = kItalianSuffix;
                break;
            case SCE_SPANISH_LANGUAGE:
                suffix = kSpanishSuffix;
                break;
            default:
                suffix = kNoText;
                break;
            }
            level = (params.mPlayMode == kPlayModeGame) ? kTutorialLevel : kTutorialRemixLevel;
            level += suffix;
            params.mLevelName = TextOf(level);
            params.mArenaName = (*GetArenaList())[kFirstArena].mName;
            params.mDifficulty = kDifficultyEasy;
            Application::shared()->GetGameManager()->SetParams(params);
            MetFrontEndState::shared()->mReturnScreen = HxStr(kTutorialScreen);
        } else {
            const HxStr level(GetMetString(kMetStrDEMOLevelSelection1 - 1 + nButton));
            GameParams params(CurrentParams());
            params.mLoadingGame = 0;
            params.mLevelName = level;
            const bool bAdvanced = !(nButton < kFirstAdvancedButton);
            params.mArenaName = (*GetArenaList())[bAdvanced ? kThirdArena : kFirstArena].mName;
            params.mDifficulty = bAdvanced ? kDifficultyNormal : kDifficultyEasy;
            CallScriptTemplate(kSelectTemplate, kSelectTemplateArgument);
            Application::shared()->GetGameManager()->SetParams(params);
            MetFrontEndState::shared()->mReturnScreen = HxStr(kOwnScreenName);
        }
        mExitChoice = kExitSelected;
        BeginExit();
        break;
    }

    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoText), mRenderer->mAnimationFrame);
        ActivateNamedPanel(HxStr(kNoText));
        mExitChoice = kExitBack;
        BeginExit();
        break;

    default:
        break;
    }
}

void MetDEMOStageSel::OnRepeatingSoundFinished(Rnd::Button *) {
}

void MetDEMOStageSel::OnEnterFinished() {
    MetHelpScreen::SetText(mHelpKeys[mStageList->mSelected], mRenderer->mAnimationFrame);
}

void MetDEMOStageSel::ShowSelectedSong() {
    HxStr level;
    const int nButton = mStageList->mSelected;
    if (nButton == kTutorialButton) {
        if (Application::shared()->GetGameManager()->GetPlayMode() == kPlayModeGame) {
            level = GetMetString(kMetStrDEMOLevelSelectionGame);
        } else {
            level = GetMetString(kMetStrDEMOLevelSelectionRemix);
        }
    } else {
        level = GetMetString(kMetStrDEMOLevelSelection1 - 1 + nButton);
    }
    mLabelPair->Load(TexturePairRecord::PicturePath(level));
    mLogoPair->Load(TexturePairRecord::LogoPath(level));

    const HxStr genre(QueryConfigString(kGenreConfigCode, TextOf(level)));
    const HxStr bpm(QueryConfigString(kBpmConfigCode, TextOf(level)));
    const HxStr genreLine(genre + kGenreSeparator + bpm + kBpmSuffix);
    mGenreText->SetText(HxStr(TextOf(genreLine)));
    mBioText->SetText(QueryConfigString(kBioConfigCode, TextOf(level)));

    const HxStr artist(QueryConfigString(kArtistConfigCode, TextOf(level)));
    const HxStr title(QueryConfigString(kTitleConfigCode, TextOf(level)));
    mLabelText->SetText(HxStr(Rnd::MakeString(kLabelFormat, TextOf(artist), TextOf(title))));
    mLoadPending = 1;
}
#endif
