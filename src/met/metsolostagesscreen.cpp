#include "met/metsolostagesscreen.h"

#include <list>
#include <vector>

#include "app/application.h"
#include "game/campaignstats.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "game/globalsettings.h"
#include "math/color.h"
#include "memcard/memcardconnectstate.h"
#include "met/albumcache.h"
#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
#include "met/metglobalsettingssaverscreen.h"
#include "met/methelpscreen.h"
#include "met/metpersonadata.h"
#include "met/metremixmanager.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "met/metsonglists.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/button.h"
#include "rnd/drawable.h"
#include "rnd/font.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/object.h"
#include "rnd/tex.h"
#include "rnd/text.h"
#include "rnd/transanim.h"
#include "rnd/view.h"
#include "script/configquery.h"
#include "script/scripthost.h"

namespace {

// The screen name, the directory the container loads from, and the container name.
static const char *const kScreenName = "ss";
static const char *const kDirectory = "metagame/_Solo";
static const char *const kContainerName = "stage_sel";

// The texture pairs the constructor builds, and the prompt key it appends.
static const char *const kFirstLogoTex = "gSongLogo1.tex";
static const char *const kSecondLogoTex = "gSongLogo2.tex";
static const char *const kFirstLabelTex = "gSongLabel1.tex";
static const char *const kSecondLabelTex = "gSongLabel2.tex";
static const char *const kLevelsPrompt = "levels";

// The objects ResolveContainerViews() stores.
static const char *const kLabelText = "ss_label.txt";
static const char *const kBioText = "ss_bio.txt";
static const char *const kGenreText = "ss_genre_bpm.txt";
static const char *const kScoreText = "ss_score.txt";
static const char *const kWarningText = "ss_warning.txt";
static const char *const kStageBonusGroup = "stage_bonus_group.view";
static const char *const kStageBonusHeading = "ss_stage_bonus.txt";
static const char *const kStageBonusLabel = "stage_score";
static const char *const kStageBeatHeading = "ss_stage_beat.txt";
static const char *const kStageBeatLabel = "stage_beat";
static const char *const kStageBonusValue = "ss_stage_bonus_val.txt";
static const char *const kStageBeatValue = "ss_stage_beat_val.txt";
static const char *const kTvView = "sstv.view";
static const char *const kTvPanelAnim = "stage_sel_tv_panel.tnm";
static const char *const kTvLeftMat = "ss_tv_left.mat";
static const char *const kTvRightMat = "ss_tv_right.mat";
static const char *const kTvLogoLeftMat = "ss_tv_logo_left.mat";
static const char *const kTvLogoRightMat = "ss_tv_logo_right.mat";
static const char *const kStatusLeftMesh = "ss_status_left.mesh";
static const char *const kStatusLeftMat = "ss_tv_status_left.mat";
static const char *const kStatusRightMesh = "ss_status_right.mesh";
static const char *const kStatusRightMat = "ss_tv_status_right.mat";
static const char *const kLockedStateTex = "lvlstate_lock_live.bmp";
static const char *const kOpenStateTex = "lvlstate_unlock.bmp";
static const char *const kWonStateTex = "lvlstate_won_sel.bmp";

// The objects BuildButtons() resolves.
static const char *const kStageButtonsView = "stage_buts.view";
static const char *const kStageButtonFormat = "ss_stage%d.but";
static const char *const kStageLabelFormat = "stage%d";
static const char *const kStageWireFormat = "ss_butt_wire_0%d.mesh";
static const char *const kCustomStageButton = "ss_stage6.but";
static const char *const kCustomStageLabel = "ss_custom";
static const char *const kIndicatorFormat = "ss_ind_0%d.but";
static const char *const kIndicatorWireFormat = "ss_ind_wire_0%d.mesh";
static const char *const kOpenLiveMat = "icon_unlock_live.mat";
static const char *const kOpenSelectedMat = "icon_unlock_sel.mat";
static const char *const kWonLiveMat = "icon_won_live.mat";
static const char *const kWonSelectedMat = "icon_won_sel.mat";
static const char *const kLockedLiveMat = "icon_lock_live.mat";
static const char *const kLockedSelectedMat = "icon_lock_sel.mat";
static const char *const kOpenStyleButton = "ss_stage1.but";
static const char *const kClosedStyleButton = "ss_stage2.but";

// The arrow buttons beside each stage.
static const char *const kLeftArrowFormat = "ss_left_0%d.but";
static const char *const kRightArrowFormat = "ss_right_0%d.but";

// The television meshes slot 26 shows once a texture has loaded.
static const char *const kTvLogoMesh = "sstv_logo right.mesh";
static const char *const kTvLabelMesh = "sstv_label_right.mesh";

// The fonts StyleLevel() applies.
static const char *const kLockedLabelFont = "font1_pinkgrey_3";
static const char *const kClosedTextFont = "font2_bluegrey_1";
static const char *const kLockedTextFont = "font2_gold_1";
static const char *const kOpenLabelFont = "font1_pink_3";
static const char *const kOpenTextFont = "font2_blue_1";

// The warnings ShowWarning() reads.
static const char *const kStage4Warning = "stage4easywarn";
static const char *const kStage5Warning = "stage5easywarn";
static const char *const kCustomWarning = "custom_warning";

// The texts ShowLevelDetails() formats.
static const char *const kLabelFormat = "%s, %s";
static const char *const kBonusMessage = "bonus_msg";
static const char *const kGenreFormat = "%s - %s bpm";
static const char *const kHighScoreFormat = "High Score: %d";
static const char *const kCountFormat = "%d";

// The title parts EnterAndShow() reads, and the two ways it joins them.
static const char *const kSoloCaption = "solo";
static const char *const kMultiCaption = "multi";
static const char *const kGameCaption = "game";
static const char *const kRemixCaption = "remix";
static const char *const kStagesCaption = "stages";
static const char *const kGameTitleFormat = "%s %s %s %s";
static const char *const kRemixTitleFormat = "%s %s %s";
static const char *const kPromptLayout = "standard_title";
static const char *const kSixButtonView = "stage_6buts.view";
static const char *const kFiveButtonView = "stage_5buts.view";

// Screens the class pushes, activates, and exits by registry key.
static const char *const kOwnScreenName = "MetSoloStagesScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kGameSkillScreen = "MetGameSkillScreen";
static const char *const kRemixTypeScreen = "MetRemixTypeScreen";
static const char *const kRemixLoadScreen = "MetRemixLoadScreen";
static const char *const kRemixDataScreen = "MetRemixDataScreen";
static const char *const kLoadGameScreen = "MetLoadGameScreen";
static const char *const kArenasScreen = "MetArenasScreen";
constexpr int kRemixReturnScreenCount = 4;

// The disc entry ListRemixes() lists after the memory card, and its playlist flag.
static const char *const kDiscSlotName = "disc";
constexpr int kDiscSlot = -1;
constexpr int kSkipPlayList = 0;

// The empty literal that clears a text, the panel, and the prompt.
static const char *const kNoText = "";

// The script template a selection runs, and its argument.
constexpr int kSelectTemplate = 0x267;
static const char *const kSelectTemplateArgument = "0";

// Configuration codes.
constexpr int kLabelConfigCode = 0x258;
constexpr int kCaptionConfigCode = 0x269;
constexpr int kArtistConfigCode = 0x320;
constexpr int kGenreConfigCode = 0x321;
constexpr int kBpmConfigCode = 0x322;
constexpr int kBioConfigCode = 0x323;
constexpr int kTitleConfigCode = 0x325;

// Stage counts. The five numbered stages come from MetSongLists stages 1 through 5, stage 6 adds
// the secret levels, and the sixth button is the custom stage.
constexpr int kNumberedStageCount = 5;
constexpr int kStageButtonCount = 6;
constexpr int kSecretLevelStage = 6;
constexpr int kCustomStageIndex = 5;
constexpr int kIndicatorCount = 7;
constexpr int kLevelWireCount = 5;
constexpr int kStage4 = 4;
constexpr int kStage5 = 5;

// A level list with the two secret levels has six or more entries. Its fifth level is the last
// regular one, and the sixth and seventh indicators are the two secret levels.
constexpr int kFullLevelListCount = 6;
constexpr int kLastRegularLevel = 4;
constexpr int kFirstSecretIndicator = 5;
constexpr int kSecondSecretIndicator = 6;
constexpr int kSecondSecretLevelCount = 2;
constexpr int kEasyStageCount = 3;
constexpr int kNormalStageCount = 4;
constexpr int kLoadGamePersonaCount = 3;

// Difficulties, from GameParams::mDifficulty.
constexpr int kDifficultyEasy = 0;
constexpr int kDifficultyNormal = 1;
constexpr int kDifficultyCount = 3;

// Rnd::Button states.
constexpr int kButtonStateNormal = 0;
constexpr int kButtonStateHighlighted = 1;
constexpr int kButtonStateDisabled = 3;
constexpr int kButtonStyleCount = 4;

// The level states that index mLevelStateTexs.
constexpr int kLevelStateLocked = 0;
constexpr int kLevelStateOpen = 1;
constexpr int kLevelStateWon = 2;

// Values of mScrollDirection.
constexpr int kScrollNone = 0;
constexpr int kScrollLeft = 1;
constexpr int kScrollRight = 2;

// The two television screens, and the material stage their textures go to.
constexpr int kTvLeft = 0;
constexpr int kTvRight = 1;
constexpr int kTexStage = 0;

// The panel animation's frames, and the scroll length.
constexpr float kPanelShownFrame = 0.0f;
constexpr float kPanelHiddenFrame = 100.0f;
constexpr float kScrollFrames = 100.0f;

// The arrow and selection alternations.
constexpr float kArrowAlternateInterval = 15.0f;
constexpr int kArrowAlternateCycles = 1;
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

// The mUnknown18 values slot 30 and slot 36 test.
constexpr int kExitBack = 0;
constexpr int kExitSelected = 2;

// The colours StyleLevel() gives the television.
constexpr float kDimComponent = 0.25f;
constexpr float kFullComponent = 1.0f;

constexpr int kNoSelection = -1;
constexpr int kNotFound = -1;

// Reports the text of a string, or the shared empty string when it has no buffer.
inline const char *TextOf(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// Resolves a registry key to an object of type T, or null.
template <typename T>
T *FindObject(const HxStr &name) {
    Rnd::Object *pObject = Rnd::g_manager.Find(name);
    return pObject != nullptr ? dynamic_cast<T *>(pObject) : nullptr;
}

template <typename T>
T *FindObject(const char *pszName) {
    return FindObject<T>(HxStr(pszName));
}

// Reads one string from configuration.
inline HxStr ConfigText(int nCode, const char *pszKey) {
    HxStr text;
    QueryConfigString(&text, nCode, pszKey);
    return text;
}

// Points a material's first stage at a texture.
inline void ShowTex(Rnd::Mat *pMat, Rnd::Tex *pTex) {
    pMat->mStages[kTexStage].SetTex(pTex);
}

// Appends the live and selected indicator materials in the order the four button states use.
inline void
AppendStateMats(std::vector<Rnd::Mat *> &mats, const char *pszLiveMat, const char *pszSelectedMat) {
    mats.push_back(FindObject<Rnd::Mat>(pszLiveMat));
    mats.push_back(FindObject<Rnd::Mat>(pszSelectedMat));
    mats.push_back(FindObject<Rnd::Mat>(pszSelectedMat));
    mats.push_back(FindObject<Rnd::Mat>(pszLiveMat));
}

// Appends the first four materials and fonts of a stage button.
inline void AppendStyles(std::vector<MetSoloStagesScreen::ButtonStyle> &styles,
                         const char *pszButton) {
    Rnd::Button *pButton = FindObject<Rnd::Button>(pszButton);
    for (int i = 0; i < kButtonStyleCount; ++i) {
        MetSoloStagesScreen::ButtonStyle style{pButton->mMats[i], pButton->mFonts[i]};
        styles.push_back(style);
    }
}

// Applies a material to every state of a button, and returns it to its first state.
inline void ApplyStateMats(Rnd::Button *pButton, const std::vector<Rnd::Mat *> &mats) {
    for (unsigned i = 0; i < mats.size(); ++i) {
        pButton->SetMat(i, mats[i]);
    }
    pButton->SetState(kButtonStateNormal);
}

// Reports the first persona's campaign statistics.
inline CampaignStats &FirstPersonaStats() {
    return MetFrontEndState::shared()->GetFirstPersona()->mStats;
}

// Reports a copy of the game manager's parameters.
inline GameParams CurrentParams() {
    return GameParams(*Application::shared()->GetGameManager()->GetParams());
}

// Adds the first memory-card slot when a card is in use.
inline void AddCardSlot(std::vector<MemcardConnectState> &slots) {
    if (MetFrontEndState::shared()->mUnknown0c != 0) {
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        slots.push_back(GlobalSettings::shared()->mCardSlots[0]);
    }
}

} // namespace

// 0x0039e308
MetSoloStagesScreen::MetSoloStagesScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mLeftArrow(nullptr), mRightArrow(nullptr), mScrollLeftTime(0.0f), mScrollRightTime(0.0f),
      mScrollDirection(kScrollNone), mUnlockAll(0), mSecretUnlocked(0), mSuperSecretUnlocked(0),
      mLogoPair(HxStr(kFirstLogoTex), HxStr(kSecondLogoTex)),
      mLabelPair(HxStr(kFirstLabelTex), HxStr(kSecondLabelTex)) {
    mStageList = new MetButtonList();
    mIndicatorList = new MetButtonList();
    mUnknown38.push_back(HxStr(kLevelsPrompt));
}

// 0x0039ef80
MetSoloStagesScreen::~MetSoloStagesScreen() {
    delete mStageList;
    delete mIndicatorList;
    ClearLevelLists();
}

// 0x003aeb28
MetSoloStagesScreen *MetSoloStagesScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetSoloStagesScreen(pRenderer, nPriority);
}

// 0x0039f720
void MetSoloStagesScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    BuildButtons();

    mLabelText = FindObject<Rnd::Text>(kLabelText);
    mBioText = FindObject<Rnd::Text>(kBioText);
    mBioText->SetText(HxStr(kNoText));
    mGenreText = FindObject<Rnd::Text>(kGenreText);
    mScoreText = FindObject<Rnd::Text>(kScoreText);
    mWarningText = FindObject<Rnd::Text>(kWarningText);
    mWarningText->SetText(HxStr(kNoText));
    mStageBonusGroup = FindObject<Rnd::View>(kStageBonusGroup);
    FindObject<Rnd::Text>(kStageBonusHeading)
        ->SetText(ConfigText(kLabelConfigCode, kStageBonusLabel));
    FindObject<Rnd::Text>(kStageBeatHeading)
        ->SetText(ConfigText(kLabelConfigCode, kStageBeatLabel));
    mStageBonusText = FindObject<Rnd::Text>(kStageBonusValue);
    mStageBeatText = FindObject<Rnd::Text>(kStageBeatValue);
    mTvView = FindObject<Rnd::View>(kTvView);
    mTvPanelAnim = FindObject<Rnd::TransAnim>(kTvPanelAnim);
    mTvPanelAnim->SetFrame(kPanelHiddenFrame);

    mTvMats.push_back(FindObject<Rnd::Mat>(kTvLeftMat));
    mTvMats.push_back(FindObject<Rnd::Mat>(kTvRightMat));
    mTvLogoMats.push_back(FindObject<Rnd::Mat>(kTvLogoLeftMat));
    mTvLogoMats.push_back(FindObject<Rnd::Mat>(kTvLogoRightMat));

    mStatusSlots.clear();
    StatusSlot slot;
    slot.mMesh = FindObject<Rnd::Mesh>(kStatusLeftMesh);
    slot.mMat = FindObject<Rnd::Mat>(kStatusLeftMat);
    mStatusSlots.push_back(slot);
    slot.mMesh = FindObject<Rnd::Mesh>(kStatusRightMesh);
    slot.mMat = FindObject<Rnd::Mat>(kStatusRightMat);
    mStatusSlots.push_back(slot);

    mLevelStateTexs.push_back(FindObject<Rnd::Tex>(kLockedStateTex));
    mLevelStateTexs.push_back(FindObject<Rnd::Tex>(kOpenStateTex));
    mLevelStateTexs.push_back(FindObject<Rnd::Tex>(kWonStateTex));

    mUnknownf4 = 0;
    mBlankTex = nullptr;
    mTvLabelTex = nullptr;
    mNextTvLabelTex = nullptr;
    mTvLogoTex = nullptr;
    mNextTvLogoTex = nullptr;
    mUnknownf0 = 0;
}

// 0x003a09d0
void MetSoloStagesScreen::BuildButtons() {
    mStageButtonsView = FindObject<Rnd::View>(kStageButtonsView);

    for (int i = 0; i < kNumberedStageCount; ++i) {
        const int nStage = i + 1;
        HxStr name(FormatString(kStageButtonFormat, nStage));
        mStageList->Add(name,
                        ConfigText(kLabelConfigCode, FormatString(kStageLabelFormat, nStage)));
        Rnd::Mesh *pWire = FindObject<Rnd::Mesh>(FormatString(kStageWireFormat, nStage));
        mStageWires.push_back(pWire);
        pWire->SetShowing(0);
    }
    mStageList->Add(HxStr(kCustomStageButton), ConfigText(kLabelConfigCode, kCustomStageLabel));

    for (int i = 0; i < static_cast<int>(mStageList->mButtons.size()); ++i) {
        mSelectedLevel[i] = 0;
        mStageLocked[i] = 0;
    }

    for (int i = 0; i < kIndicatorCount; ++i) {
        const int nIndicator = i + 1;
        mIndicatorList->Add(HxStr(FormatString(kIndicatorFormat, nIndicator)), HxStr(kNoText));
        Rnd::Mesh *pWire = FindObject<Rnd::Mesh>(FormatString(kIndicatorWireFormat, nIndicator));
        mIndicatorWires.push_back(pWire);
        pWire->SetShowing(0);
    }
    mIndicatorList->ButtonAt(kFirstSecretIndicator)->SetState(kButtonStateDisabled);
    mIndicatorList->ButtonAt(kSecondSecretIndicator)->SetState(kButtonStateDisabled);

    AppendStateMats(mOpenMats, kOpenLiveMat, kOpenSelectedMat);
    AppendStateMats(mWonMats, kWonLiveMat, kWonSelectedMat);
    AppendStateMats(mLockedMats, kLockedLiveMat, kLockedSelectedMat);

    AppendStyles(mOpenStyles, kOpenStyleButton);
    AppendStyles(mClosedStyles, kClosedStyleButton);
}

// 0x003a1c88
void MetSoloStagesScreen::ClearLevelLists() {
    for (int i = 0; i < kNumberedStageCount; ++i) {
        std::vector<StageListEntry>::iterator it = mStageLevels[i].begin();
        while (it != mStageLevels[i].end()) {
            it = mStageLevels[i].erase(it);
        }
        mStageLevels[i].clear();
    }
    std::vector<StageListEntry>::iterator it = mCustomLevels.begin();
    while (it != mCustomLevels.end()) {
        it = mCustomLevels.erase(it);
    }
    mCustomLevels.clear();
}

// 0x003a75a8
void MetSoloStagesScreen::RebuildLevelLists() {
    ClearLevelLists();
    for (int i = 0; i < kNumberedStageCount; ++i) {
        mStageLevels[i] = *GetStageList(i + 1);
    }
    const std::vector<StageListEntry> secretLevels(*GetStageList(kSecretLevelStage));
    for (unsigned i = 0; i < secretLevels.size(); ++i) {
        mStageLevels[kNumberedStageCount - 1].push_back(secretLevels[i]);
    }
}

// 0x003a79f0
void MetSoloStagesScreen::UpdateSecretLevels(int bUnlockAll) {
    const std::vector<StageListEntry> secretLevels(*GetStageList(kSecretLevelStage));
    for (unsigned i = 0; i < secretLevels.size(); ++i) {
        // Yes, the binary runs this loop with an empty body.
    }

    if (bUnlockAll == 1) {
        if (secretLevels.size() == 0) {
            mSuperSecretUnlocked = 0;
            mSecretUnlocked = 0;
            return;
        }
        mSecretUnlocked = 1;
        mIndicatorList->ButtonAt(kFirstSecretIndicator)->SetState(kButtonStateNormal);
        mIndicatorList->ButtonAt(kFirstSecretIndicator)->SetShowing(0);
        if (secretLevels.size() >= kSecondSecretLevelCount) {
            mSuperSecretUnlocked = 1;
            mIndicatorList->ButtonAt(kSecondSecretIndicator)->SetState(kButtonStateNormal);
            mIndicatorList->ButtonAt(kSecondSecretIndicator)->SetShowing(0);
        }
    } else if (Application::shared()->GetGameMode() == kGameModeSolo) {
        CampaignStats &stats = FirstPersonaStats();
        mSecretUnlocked = stats.IsSecretUnlocked();
        mSuperSecretUnlocked = stats.IsSuperSecretUnlocked();
    } else {
        mSecretUnlocked = 0;
        mSuperSecretUnlocked = 0;
        const std::vector<MetPersonaData *> &personas = MetFrontEndState::shared()->mUnknown00;
        for (unsigned i = 0; i < MetFrontEndState::shared()->mUnknown00.size(); ++i) {
            CampaignStats &stats = personas[i]->mStats;
            mSecretUnlocked = (mSecretUnlocked != 0) || (stats.IsSecretUnlocked() != 0);
            mSuperSecretUnlocked =
                (mSuperSecretUnlocked != 0) || (stats.IsSuperSecretUnlocked() != 0);
        }
    }
}

// 0x003a2190
int MetSoloStagesScreen::IsStageUnavailable(int nStage) {
    const GameParams params(CurrentParams());
    if (params.mUnknown1c == kPlayModeJam) {
        return 0;
    }
    if (((params.mDifficulty == kDifficultyEasy) && (nStage >= kEasyStageCount)) ||
        ((params.mDifficulty == kDifficultyNormal) && (nStage >= kNormalStageCount)) ||
        (nStage == kCustomStageIndex)) {
        return 1;
    }
    return 0;
}

// 0x003aed00
inline int MetSoloStagesScreen::IsStageSelectable(int nStage) {
    return (mStageLocked[nStage] == 0) && (IsLevelLocked(mSelectedLevel[nStage]) == 0);
}

// 0x003aebb0
inline int MetSoloStagesScreen::IsStageBeaten(CampaignStats &stats, int nDifficulty, int nStage) {
    if (Application::shared()->GetPlayMode() == kPlayModeGame) {
        return stats.GetStageScoreBeaten(nDifficulty, nStage);
    }
    for (int i = 0; i < kDifficultyCount; ++i) {
        if (stats.GetStageScoreBeaten(i, nStage) != 0) {
            return 1;
        }
    }
    return 0;
}

// 0x003a7f38
void MetSoloStagesScreen::SetUpStages(int bUnlockAll) {
    mStageList->SetSelected(kNoSelection);
    const GameParams params(CurrentParams());

    mStageLocked[0] = 0;
    int nStagesComplete = 0;
    for (int i = 1; i < kNumberedStageCount; ++i) {
        if ((params.mDifficulty == kDifficultyEasy) && (i >= kEasyStageCount) &&
            (params.mUnknown1c == kPlayModeGame)) {
            mStageLocked[i] = 1;
            continue;
        }
        if ((params.mDifficulty == kDifficultyNormal) && (i >= kNormalStageCount) &&
            (params.mUnknown1c == kPlayModeGame)) {
            mStageLocked[i] = 1;
            continue;
        }
        mStageLocked[i] = 0;
        if (bUnlockAll != 0) {
            continue;
        }

        bool bComplete = false;
        if (Application::shared()->GetGameMode() == kGameModeSolo) {
            CampaignStats &stats = FirstPersonaStats();
            bComplete = (params.mUnknown1c == kPlayModeGame) ?
                            (stats.IsStageComplete(params.mDifficulty, i) != 0) :
                            (stats.IsStageCompleteAtAnyDifficulty(i) != 0);
        } else {
            for (unsigned j = 0; j < MetFrontEndState::shared()->mUnknown00.size(); ++j) {
                CampaignStats &stats = MetFrontEndState::shared()->mUnknown00[j]->mStats;
                const int nComplete = (params.mUnknown1c == kPlayModeGame) ?
                                          stats.IsStageComplete(params.mDifficulty, i) :
                                          stats.IsStageCompleteAtAnyDifficulty(i);
                bComplete = bComplete | (nComplete != 0);
            }
        }
        if (bComplete) {
            ++nStagesComplete;
        } else {
            mStageLocked[i] = 1;
        }
    }
    mStageLocked[kCustomStageIndex] = 0;

    UpdateSecretLevels(bUnlockAll);
    ApplyStageStyles();

    int nStage = nStagesComplete;
    if (mStageLocked[nStage] != 0) {
        nStage = nStagesComplete - 1;
    }
    if ((params.mUnknown1c == kPlayModeGame) &&
        (Application::shared()->GetGameMode() == kGameModeSolo)) {
        CampaignStats &stats = FirstPersonaStats();
        if ((stats.IsDifficultyComplete(params.mDifficulty) == 0) &&
            (stats.IsStageComplete(params.mDifficulty, nStage + 1) != 0)) {
            int nFirstUnbeaten = kNotFound;
            int nFirstBelowScore = kNotFound;
            for (int i = 0; i <= nStage; ++i) {
                const HxStr bonusLevel(stats.GetBonusLevelName(params.mDifficulty, i + 1));
                if (bonusLevel != kNoText) {
                    const int nBeaten = stats.GetLevelBeaten(params.mDifficulty, bonusLevel);
                    if (stats.GetStageScoreBeaten(params.mDifficulty, i + 1) == 0) {
                        if (nFirstBelowScore == kNotFound) {
                            nFirstBelowScore = i;
                        }
                    } else if (nBeaten == 0) {
                        nFirstUnbeaten = i;
                        break;
                    }
                }
            }
            if (nFirstUnbeaten != kNotFound) {
                nStage = nFirstUnbeaten;
            } else if (nFirstBelowScore != kNotFound) {
                nStage = nFirstBelowScore;
            }
        }
    }

    mStageList->SetSelected(nStage);
    RefreshStage();
    UpdateArrows();

    unsigned nLevel = 0;
    if ((params.mUnknown1c == kPlayModeGame) &&
        (Application::shared()->GetGameMode() == kGameModeSolo)) {
        for (; nLevel < mCurrentLevels->size(); ++nLevel) {
            const HxStr name((*mCurrentLevels)[nLevel].mName);
            if (FirstPersonaStats().GetLevelBeaten(params.mDifficulty, name) == 0) {
                break;
            }
        }
        if (!(nLevel < mCurrentLevels->size())) {
            nLevel = mCurrentLevels->size() - 1;
        }
        if (IsLevelLocked(nLevel) != 0) {
            --nLevel;
        }
    }
    mSelectedLevel[mStageList->mSelected] = nLevel;
    mIndicatorList->SetSelected(nLevel);
    ShowLevelDetails();
}

// 0x003a26c0
void MetSoloStagesScreen::ApplyStageStyles() {
    for (int i = 0; i < static_cast<int>(mStageList->mButtons.size()); ++i) {
        Rnd::Button *pButton = mStageList->ButtonAt(i);
        const std::vector<ButtonStyle> &styles =
            (mStageLocked[i] != 0) ? mClosedStyles : mOpenStyles;
        for (unsigned j = 0; j < styles.size(); ++j) {
            pButton->SetMat(j, styles[j].mMat);
            pButton->SetFont(j, styles[j].mFont);
        }
        pButton->SetState(kButtonStateNormal);
    }
}

// 0x003a1f38
void MetSoloStagesScreen::RefreshStage() {
    mIndicatorList->SetSelected(kNoSelection);
    const int nStage = mStageList->mSelected;
    mCurrentLevels = (nStage == static_cast<int>(mStageList->mButtons.size()) - 1) ?
                         &mCustomLevels :
                         &mStageLevels[nStage];
    unsigned nLevel = mSelectedLevel[nStage];
    if (!(nLevel < mCurrentLevels->size())) {
        nLevel = mCurrentLevels->size() - 1;
    }

    for (int i = 0; i < kLevelWireCount; ++i) {
        mIndicatorWires[i]->SetShowing(0);
    }
    for (int i = 0; i < kLevelWireCount; ++i) {
        mStageWires[i]->SetShowing(0);
    }

    if (IsStageUnavailable(nStage) != 0) {
        mLogoPair.CancelLoad();
        mLabelPair.CancelLoad();
        ShowIndicators(0);
        ShowLevelTexts(0);
        ShowWarning(true);
        mStageBonusGroup->SetShowing(0);
        return;
    }

    ShowIndicators(1);
    ShowWarning(false);
    mStageWires[nStage]->SetShowing(1);
    if (static_cast<int>(nLevel) < kLevelWireCount) {
        mIndicatorWires[nLevel]->SetShowing(1);
    }
    RefreshIndicators(mStageLocked[nStage]);
    mIndicatorList->SetSelected(mSelectedLevel[nStage]);
    ShowStageBonus(nStage + 1);
}

// 0x003a2368
void MetSoloStagesScreen::ShowStageBonus(int nStage) {
    const GameParams params(CurrentParams());
    const int nDifficulty = Application::shared()->GetGameManager()->GetParams()->mDifficulty;
    const int nAlbumValue = GetAlbumLevelValue(nStage - 1, nDifficulty);
    CampaignStats &stats = FirstPersonaStats();
    const int nScore = stats.GetStageScore(nDifficulty, nStage);

    if ((params.mUnknown1c == kPlayModeJam) || (nAlbumValue == 0) ||
        (Application::shared()->GetGameMode() == kGameModeLocal) ||
        (stats.GetStageScoreBeaten(nDifficulty, nStage) == 1)) {
        mStageBonusGroup->SetShowing(0);
        return;
    }
    mStageBonusGroup->SetShowing(1);
    mStageBonusText->SetText(HxStr(FormatString(kCountFormat, nScore)));
    mStageBeatText->SetText(HxStr(FormatString(kCountFormat, nAlbumValue)));
}

// 0x003a2878
int MetSoloStagesScreen::IsLevelLocked(int nLevel) {
    if (mUnlockAll != 0) {
        return 0;
    }
    CampaignStats &stats = MetFrontEndState::shared()->GetFirstPersona()->mStats;
    const GameParams params(CurrentParams());
    const int nDifficulty = params.mDifficulty;
    const int nNextStage = mStageList->mSelected + 1;
    if (nNextStage >= kFullLevelListCount) {
        return 0;
    }
    int nLocked = 0;
    if (GetAlbumLevelValue(mStageList->mSelected, nDifficulty) == 0) {
        return 0;
    }

    const int nLevelCount = mCurrentLevels->size();
    if (nLevelCount >= kFullLevelListCount) {
        if (nLevel == kFirstSecretIndicator) {
            return mSecretUnlocked ^ 1;
        }
        if (nLevel == kSecondSecretIndicator) {
            return mSuperSecretUnlocked ^ 1;
        }
    }

    int nBeaten = 0;
    if (Application::shared()->GetGameMode() == kGameModeSolo) {
        nBeaten = IsStageBeaten(stats, nDifficulty, nNextStage);
    } else {
        for (unsigned i = 0; i < MetFrontEndState::shared()->mUnknown00.size(); ++i) {
            MetPersonaData *pPersona = MetFrontEndState::shared()->mUnknown00[i];
            nBeaten =
                (nBeaten != 0) || (IsStageBeaten(pPersona->mStats, nDifficulty, nNextStage) != 0);
        }
    }

    if ((nLevelCount >= kFullLevelListCount) && (nLevel == kLastRegularLevel) && (nBeaten == 0)) {
        nLocked = 1;
    } else if (nLevel == nLevelCount - 1) {
        nLocked = (nBeaten == 0);
    }
    return nLocked;
}

// 0x003a2d58
void MetSoloStagesScreen::RefreshIndicators(int bStageLocked) {
    CampaignStats &stats = FirstPersonaStats();
    const GameParams params(CurrentParams());
    const int nDifficulty = params.mDifficulty;

    for (unsigned i = 0; i < mCurrentLevels->size(); ++i) {
        Rnd::Button *pButton = mIndicatorList->ButtonAt(i);
        pButton->SetShowing(1);
        const HxStr name((*mCurrentLevels)[i].mName);
        const int nLocked = IsLevelLocked(i);
        if ((Application::shared()->GetGameMode() == kGameModeSolo) &&
            (Application::shared()->GetPlayMode() == kPlayModeGame) &&
            (stats.GetLevelBeaten(nDifficulty, name) != 0)) {
            ApplyStateMats(pButton, mWonMats);
        } else if ((bStageLocked != 0) || (nLocked != 0)) {
            ApplyStateMats(pButton, mLockedMats);
        } else {
            ApplyStateMats(pButton, mOpenMats);
        }
    }

    for (int i = mCurrentLevels->size(); i < static_cast<int>(mIndicatorList->mButtons.size());
         ++i) {
        mIndicatorList->ButtonAt(i)->SetState(kButtonStateDisabled);
        mIndicatorList->ButtonAt(i)->SetShowing(0);
    }
    mIndicatorList->ButtonAt(kFirstSecretIndicator)->SetShowing(0);
    mIndicatorList->ButtonAt(kSecondSecretIndicator)->SetShowing(0);
    if (mSecretUnlocked == 0) {
        mIndicatorList->ButtonAt(kFirstSecretIndicator)->SetState(kButtonStateDisabled);
    }
    if (mSuperSecretUnlocked == 0) {
        mIndicatorList->ButtonAt(kSecondSecretIndicator)->SetState(kButtonStateDisabled);
    }
}

// 0x003aec70
void MetSoloStagesScreen::ShowIndicators(int nShowing) {
    for (int i = 0; i < static_cast<int>(mIndicatorList->mButtons.size()); ++i) {
        mIndicatorList->ButtonAt(i)->SetShowing(nShowing);
    }
}

// 0x003a3238
void MetSoloStagesScreen::ShowLevelTexts(int nShowing) {
    mLabelText->SetShowing(nShowing);
    mGenreText->SetShowing(nShowing);
    mBioText->SetShowing(nShowing);
    mScoreText->SetShowing(nShowing);
    mStatusSlots[kTvLeft].mMesh->SetShowing(nShowing);
    mStatusSlots[kTvRight].mMesh->SetShowing(nShowing);
}

// 0x003a3310
void MetSoloStagesScreen::ShowWarning(bool bShow) {
    if (bShow) {
        mWarningText->SetShowing(1);
        const int nStage = mStageList->mSelected + 1;
        if (nStage == kStage4) {
            mWarningText->SetText(ConfigText(kLabelConfigCode, kStage4Warning));
        } else if (nStage == kStage5) {
            mWarningText->SetText(ConfigText(kLabelConfigCode, kStage5Warning));
        } else {
            mWarningText->SetText(ConfigText(kLabelConfigCode, kCustomWarning));
        }
    } else {
        mWarningText->SetShowing(0);
    }
    mTvView->SetShowing(!bShow);
}

// 0x003a3510
void MetSoloStagesScreen::StyleLevel(int bStageLocked, const HxStr &levelName) {
    const int nLocked = IsLevelLocked(mSelectedLevel[mStageList->mSelected]);
    Color color;
    Rnd::Font *pLabelFont;
    Rnd::Font *pTextFont;
    int nState;
    if (bStageLocked != 0) {
        color = Color{kDimComponent, kDimComponent, kDimComponent, kFullComponent};
        pLabelFont = FindObject<Rnd::Font>(kLockedLabelFont);
        pTextFont = FindObject<Rnd::Font>(kClosedTextFont);
        mScoreText->SetShowing(0);
        nState = kLevelStateLocked;
    } else if (nLocked != 0) {
        color = Color{kDimComponent, kDimComponent, kDimComponent, kFullComponent};
        pLabelFont = FindObject<Rnd::Font>(kLockedLabelFont);
        pTextFont = FindObject<Rnd::Font>(kLockedTextFont);
        mScoreText->SetShowing(0);
        nState = kLevelStateLocked;
    } else {
        color = Color{kFullComponent, kFullComponent, kFullComponent, kFullComponent};
        pLabelFont = FindObject<Rnd::Font>(kOpenLabelFont);
        pTextFont = FindObject<Rnd::Font>(kOpenTextFont);
        CampaignStats &stats = FirstPersonaStats();
        const GameParams params(CurrentParams());
        nState = kLevelStateOpen;
        if ((Application::shared()->GetGameMode() == kGameModeSolo) &&
            (Application::shared()->GetPlayMode() == kPlayModeGame) &&
            (stats.GetLevelBeaten(params.mDifficulty, levelName) != 0)) {
            nState = kLevelStateWon;
        }
        mScoreText->SetShowing(1);
    }

    Rnd::Mat *const mats[] = {
        mTvMats[kTvLeft], mTvMats[kTvRight], mTvLogoMats[kTvLeft], mTvLogoMats[kTvRight]};
    for (Rnd::Mat *pMat : mats) {
        if (pMat->mStages[kTexStage].mTex != nullptr) {
            pMat->SetDiffuse(color);
        }
    }
    mLabelText->SetFont(pLabelFont);
    mGenreText->SetFont(pTextFont);
    mBioText->SetFont(pTextFont);
    ShowTex(mStatusSlots[kTvLeft].mMat, mLevelStateTexs[nState]);
    ShowTex(mStatusSlots[kTvRight].mMat, mLevelStateTexs[nState]);
}

// 0x003a3c68
void MetSoloStagesScreen::ShowLevelDetails() {
    if (mUnknown48 != 0) {
        return;
    }
    const int nStage = mStageList->mSelected;
    if (IsStageUnavailable(nStage) != 0) {
        ShowLevelTexts(0);
        return;
    }
    ShowLevelTexts(1);
    ShowWarning(false);

    unsigned nLevel = mSelectedLevel[nStage];
    if (!(nLevel < mCurrentLevels->size())) {
        mSelectedLevel[nStage] = 0;
        nLevel = 0;
    }
    const HxStr level((*mCurrentLevels)[nLevel].mName);
    StyleLevel(mStageLocked[nStage], level);

    const HxStr artist(ConfigText(kArtistConfigCode, TextOf(level)));
    const HxStr title(ConfigText(kTitleConfigCode, TextOf(level)));
    mLabelText->SetText(HxStr(FormatString(kLabelFormat, TextOf(artist), TextOf(title))));

    if (IsLevelLocked(nLevel) != 0) {
        const HxStr message(ConfigText(kLabelConfigCode, kBonusMessage));
        const int nAlbumValue = GetAlbumLevelValue(
            nStage, Application::shared()->GetGameManager()->GetParams()->mDifficulty);
        const int nStageLevelCount = GetStageList(nStage + 1)->size();
        mBioText->SetText(HxStr(FormatString(TextOf(message), nStageLevelCount - 1, nAlbumValue)));
    } else {
        mBioText->SetText(ConfigText(kBioConfigCode, TextOf(level)));
    }

    const HxStr genre(ConfigText(kGenreConfigCode, TextOf(level)));
    const HxStr bpm(ConfigText(kBpmConfigCode, TextOf(level)));
    mGenreText->SetText(HxStr(FormatString(kGenreFormat, TextOf(genre), TextOf(bpm))));

    if ((mStageLocked[nStage] == 0) && (Application::shared()->GetPlayMode() == kPlayModeGame) &&
        (Application::shared()->GetGameMode() == kGameModeSolo)) {
        const GameParams params(CurrentParams());
        const int nHighScore = FirstPersonaStats().GetLevelHighScore(params.mDifficulty, level);
        mScoreText->SetText(HxStr(FormatString(kHighScoreFormat, nHighScore)));
    } else {
        mScoreText->SetText(HxStr(kNoText));
    }
}

// 0x003a4438
void MetSoloStagesScreen::UpdateArrows() {
    if (mLeftArrow != nullptr) {
        mLeftArrow->SetState(kButtonStateNormal);
    }
    if (mRightArrow != nullptr) {
        mRightArrow->SetState(kButtonStateNormal);
    }
    const int nStage = mStageList->mSelected;
    if (IsStageUnavailable(nStage) != 0) {
        return;
    }
    mLeftArrow = FindObject<Rnd::Button>(FormatString(kLeftArrowFormat, nStage + 1));
    mRightArrow = FindObject<Rnd::Button>(FormatString(kRightArrowFormat, nStage + 1));
    if (mLeftArrow != nullptr) {
        mLeftArrow->SetState(kButtonStateHighlighted);
    }
    if (mRightArrow != nullptr) {
        mRightArrow->SetState(kButtonStateHighlighted);
    }
}

// 0x003a4640
void MetSoloStagesScreen::LoadLevelTextures() {
    const int nStage = mStageList->mSelected;
    if (IsStageUnavailable(nStage) != 0) {
        mLabelPair.CancelLoad();
        mLogoPair.CancelLoad();
        return;
    }
    if (mCurrentLevels->size() == 0) {
        return;
    }
    const HxStr level((*mCurrentLevels)[mSelectedLevel[nStage]].mName);
    mLabelPair.Load(TexturePairRecord::PicturePath(level));
    mLogoPair.Load(TexturePairRecord::LogoPath(level));
    if (mScrollLeftTime != 0.0f) {
        mScrollLeftTime = 0.0f;
    }
    if (mScrollRightTime != 0.0f) {
        mScrollRightTime = 0.0f;
    }
}

// 0x003a52f8
void MetSoloStagesScreen::StartScroll() {
    if (IsStageUnavailable(mStageList->mSelected) != 0) {
        mScrollLeftTime = 0.0f;
        mScrollRightTime = 0.0f;
        ShowLevelDetails();
        ActivateNamedPanel(HxStr(kOwnScreenName));
        return;
    }
    ShowLevelTexts(0);
    if (mScrollRightTime != 0.0f) {
        ShowTex(mTvMats[kTvRight], mTvLabelTex);
        ShowTex(mTvMats[kTvLeft], mNextTvLabelTex);
        mTvPanelAnim->SetFrame(kPanelShownFrame);
        ShowTex(mTvLogoMats[kTvRight], mTvLogoTex);
        ShowTex(mTvLogoMats[kTvLeft], mNextTvLogoTex);
    } else if (mScrollLeftTime != 0.0f) {
        mTvPanelAnim->SetFrame(kPanelHiddenFrame);
        ShowTex(mTvMats[kTvLeft], mTvLabelTex);
        ShowTex(mTvMats[kTvRight], mNextTvLabelTex);
        ShowTex(mTvLogoMats[kTvLeft], mTvLogoTex);
        ShowTex(mTvLogoMats[kTvRight], mNextTvLogoTex);
    }
}

// 0x003a5810
void MetSoloStagesScreen::EnterAndShow() {
    SetShowing(0);
    bool bSaveFirst = false;
    if (MetFrontEndState::shared()->mUnknown18 != 0) {
        bSaveFirst = MetFrontEndState::shared()->mUnknown10 != 0;
    }
    if (bSaveFirst) {
        MetFrontEndState::shared()->mUnknown10 = 0;
        std::vector<HxStr> screens(1, HxStr());
        screens[0] = kOwnScreenName;
        MetGlobalSettingsSaverScreen::StartSave(screens);
        mUnknown50 = 0;
        return;
    }

    mUnlockAll = MetFrontEndState::shared()->mUnknown14;
    RebuildLevelLists();
    SetUpStages(MetFrontEndState::shared()->mUnknown14);

    if (MetFrontEndState::shared()->mUnknown18 != 0) {
        MetFrontEndState *pState = MetFrontEndState::shared();
        pState->mUnknown1c = pState->mUnknown18;
        pState->mUnknown18 = 0;
        MetHelpScreen::SelectPreset(HxStr(kPromptLayout));
        PushNamedScreen(HxStr(kHelpScreen));
        mUnknown10->SetActivePanel(this);
        GameParams params(CurrentParams());
        params.mLoadingGame = 0;
        Application::shared()->GetGameManager()->SetParams(params);
        CallScriptTemplate(kSelectTemplate, kSelectTemplateArgument);
    }

    HxStr mode;
    HxStr kind;
    HxStr difficulty;
    HxStr stages;
    HxStr title;
    const GameParams params(CurrentParams());
    if (Application::shared()->GetGameManager()->GetGameMode() == kGameModeSolo) {
        mode = ConfigText(kCaptionConfigCode, kSoloCaption);
    } else {
        mode = ConfigText(kCaptionConfigCode, kMultiCaption);
    }
    if (params.mUnknown1c == kPlayModeGame) {
        kind = ConfigText(kCaptionConfigCode, kGameCaption);
        difficulty = DifficultyName(params.mDifficulty);
    } else {
        kind = ConfigText(kCaptionConfigCode, kRemixCaption);
    }
    stages = ConfigText(kCaptionConfigCode, kStagesCaption);
    if (params.mUnknown1c == kPlayModeGame) {
        title = FormatString(
            kGameTitleFormat, TextOf(mode), TextOf(kind), TextOf(difficulty), TextOf(stages));
    } else {
        title = FormatString(kRemixTitleFormat, TextOf(mode), TextOf(kind), TextOf(stages));
    }

    ShowLevelTexts(0);
    ShowWarning(false);
    MetScreenTitleScreen::SetTitle(title);

    mTvLabelTex = mBlankTex;
    ShowTex(mTvMats[kTvLeft], mTvLabelTex);
    ShowTex(mTvLogoMats[kTvLeft], mTvLogoTex);
    ShowTex(mTvMats[kTvRight], mTvLabelTex);
    ShowTex(mTvLogoMats[kTvRight], mTvLogoTex);
    mTvPanelAnim->SetFrame(kPanelShownFrame);
    mTvView->SetShowing(0);
    mLabelPair.invalidate();
    mLogoPair.invalidate();

    mStageButtonsView->ReleaseAnimsRefs();
    mStageButtonsView->ClearDraws();
    mStageButtonsView->ClearTransList();
    Rnd::View *pButtons;
    if (params.mUnknown1c == kPlayModeGame) {
        pButtons = FindObject<Rnd::View>(kSixButtonView);
        mStageList->ButtonAt(kCustomStageIndex)->SetState(kButtonStateNormal);
    } else {
        pButtons = FindObject<Rnd::View>(kFiveButtonView);
        mStageList->ButtonAt(kCustomStageIndex)->SetState(kButtonStateDisabled);
    }
    mStageButtonsView->AddAnim(pButtons);
    mStageButtonsView->AddTrans(pButtons);
    std::list<Rnd::Drawable *> &draws = mStageButtonsView->GetDraws();
    mStageButtonsView->AddDraw(pButtons, draws.empty() ? nullptr : draws.front());

    for (int i = 0; i < kStageButtonCount; ++i) {
        Rnd::Button *pLeft = FindObject<Rnd::Button>(FormatString(kLeftArrowFormat, i + 1));
        Rnd::Button *pRight = FindObject<Rnd::Button>(FormatString(kRightArrowFormat, i + 1));
        if (IsStageUnavailable(i) != 0) {
            pLeft->SetShowing(0);
            pRight->SetShowing(0);
        } else {
            pLeft->SetShowing(1);
            pRight->SetShowing(1);
        }
    }

    MetHelpScreen::SelectPreset(HxStr(kPromptLayout));
    MetScreen::EnterAndShow();
}

// 0x003a6690
void MetSoloStagesScreen::BeginExit() {
    mTvLabelTex = mBlankTex;
    mTvLogoTex = mBlankTex;
    ShowTex(mTvMats[kTvLeft], mTvLabelTex);
    ShowTex(mTvLogoMats[kTvLeft], mTvLogoTex);
    ShowTex(mTvMats[kTvRight], mTvLabelTex);
    ShowTex(mTvLogoMats[kTvRight], mTvLogoTex);
    mTvView->SetShowing(0);
    MetScreen::BeginExit();
}

// 0x003a4810
void MetSoloStagesScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
    case kMetScreenCommandNext:
        if (pCommand->mCommand == kMetScreenCommandPrevious) {
            mStageList->OnUnknownSlot2();
        } else {
            mStageList->OnUnknownSlot3();
        }
        RefreshStage();
        UpdateArrows();
        LoadLevelTextures();
        mScrollDirection = kScrollNone;
        break;

    case kMetScreenCommandLeft:
        if (IsStageUnavailable(mStageList->mSelected) != 0) {
            return;
        }
        ActivateNamedPanel(HxStr(kNoText));
        mScrollDirection = kScrollLeft;
        StartRepeatingSound(
            mUnknown10->mUnknown68, kArrowAlternateInterval, mLeftArrow, kArrowAlternateCycles);
        break;

    case kMetScreenCommandRight:
        if (IsStageUnavailable(mStageList->mSelected) != 0) {
            return;
        }
        ActivateNamedPanel(HxStr(kNoText));
        mScrollDirection = kScrollRight;
        StartRepeatingSound(
            mUnknown10->mUnknown68, kArrowAlternateInterval, mRightArrow, kArrowAlternateCycles);
        break;

    case kMetScreenCommandSelect: {
        if (IsStageSelectable(mStageList->mSelected) == 0) {
            return;
        }
        ActivateNamedPanel(HxStr(kNoText));
        MetHelpScreen::SetText(HxStr(kNoText), mUnknown10->mUnknown68);
        const int nStage = mStageList->mSelected;
        if (nStage != kCustomStageIndex) {
            const int nLevel = mSelectedLevel[nStage];
            GameParams params(CurrentParams());
            params.mLoadingGame = 0;
            params.mLevelName = (*mCurrentLevels)[nLevel].mName;
            CallScriptTemplate(kSelectTemplate, kSelectTemplateArgument);
            Application::shared()->GetGameManager()->SetParams(params);
        }
        StartRepeatingSound(mUnknown10->mUnknown68,
                            kSelectAlternateInterval,
                            mStageList->mUnknown00,
                            kSelectAlternateCycles);
        break;
    }

    case kMetScreenCommandBack:
        ActivateNamedPanel(HxStr(kNoText));
        MetHelpScreen::SetText(HxStr(kNoText), mUnknown10->mUnknown68);
        mUnknown18 = kExitBack;
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// 0x003aed40
void MetSoloStagesScreen::PlaySlideSound(int nSelector) {
    if (IsStageSelectable(mStageList->mSelected) != 0) {
        MetScreen::PlaySlideSound(nSelector);
    }
}

// 0x003aedb0
void MetSoloStagesScreen::PlayCycleLeftSound(int nSelector) {
    if (IsStageUnavailable(mStageList->mSelected) == 0) {
        MetScreen::PlayCycleLeftSound(nSelector);
    }
}

// 0x003aee00
void MetSoloStagesScreen::PlayCycleRightSound(int nSelector) {
    if (IsStageUnavailable(mStageList->mSelected) == 0) {
        MetScreen::PlayCycleRightSound(nSelector);
    }
}

// 0x003a4df8
void MetSoloStagesScreen::OnUnknownSlot26(float flTime) {
    const bool bLogoAdvanced = mLogoPair.Advance() != 0;
    const bool bAdvanced = bLogoAdvanced | (mLabelPair.Advance() != 0);

    Rnd::Tex *pLogo = mLogoPair.Current();
    Rnd::Mesh *pLogoMesh = FindObject<Rnd::Mesh>(kTvLogoMesh);
    pLogoMesh->SetShowing(0);
    if (pLogo != nullptr) {
        pLogoMesh->SetShowing(1);
    }
    Rnd::Tex *pLabel = mLabelPair.Current();
    Rnd::Mesh *pLabelMesh = FindObject<Rnd::Mesh>(kTvLabelMesh);
    pLabelMesh->SetShowing(0);
    if (pLabel != nullptr) {
        pLabelMesh->SetShowing(1);
    }

    if (bAdvanced) {
        if (mScrollDirection == kScrollNone) {
            mTvLogoTex = pLogo;
            mTvLabelTex = pLabel;
            mTvPanelAnim->SetFrame(kPanelShownFrame);
            ShowTex(mTvMats[kTvRight], mTvLabelTex);
            ShowTex(mTvLogoMats[kTvRight], mTvLogoTex);
            ShowLevelDetails();
        } else if (mScrollDirection == kScrollLeft) {
            mNextTvLogoTex = pLogo;
            mNextTvLabelTex = pLabel;
            mScrollLeftTime = flTime;
            StartScroll();
        } else if (mScrollDirection == kScrollRight) {
            mNextTvLogoTex = pLogo;
            mNextTvLabelTex = pLabel;
            mScrollRightTime = flTime;
            StartScroll();
        }
    }

    if (mScrollLeftTime != 0.0f) {
        const float flElapsed = flTime - mScrollLeftTime;
        if (flElapsed < kScrollFrames) {
            mTvPanelAnim->SetFrame(kPanelHiddenFrame - flElapsed);
        } else {
            mTvPanelAnim->SetFrame(kPanelHiddenFrame);
            mScrollLeftTime = 0.0f;
            ShowLevelDetails();
            mTvLabelTex = mNextTvLabelTex;
            mTvLogoTex = mNextTvLogoTex;
            mTvPanelAnim->SetFrame(kPanelShownFrame);
            ShowTex(mTvMats[kTvRight], mTvLabelTex);
            ShowTex(mTvLogoMats[kTvRight], mTvLogoTex);
            ActivateNamedPanel(HxStr(kOwnScreenName));
        }
    }

    if (mScrollRightTime != 0.0f) {
        const float flElapsed = flTime - mScrollRightTime;
        if (flElapsed < kScrollFrames) {
            mTvPanelAnim->SetFrame(flElapsed);
        } else {
            mScrollRightTime = 0.0f;
            ShowLevelDetails();
            mTvLabelTex = mNextTvLabelTex;
            mTvLogoTex = mNextTvLogoTex;
            mTvPanelAnim->SetFrame(kPanelShownFrame);
            ShowTex(mTvMats[kTvRight], mTvLabelTex);
            ShowTex(mTvLogoMats[kTvRight], mTvLogoTex);
            ActivateNamedPanel(HxStr(kOwnScreenName));
        }
    }
}

// 0x003a54b8
void MetSoloStagesScreen::OnUnknownSlot30(Rnd::Button *pButton) {
    if ((pButton == mLeftArrow) || (pButton == mRightArrow)) {
        int &nLevel = mSelectedLevel[mStageList->mSelected];
        mIndicatorWires[nLevel]->SetShowing(0);
        if (pButton == mLeftArrow) {
            mIndicatorList->OnUnknownSlot2();
        } else {
            mIndicatorList->OnUnknownSlot3();
        }
        nLevel = mIndicatorList->mSelected;
        if (nLevel < kLevelWireCount) {
            mIndicatorWires[nLevel]->SetShowing(1);
        }
        LoadLevelTextures();
        ActivateNamedPanel(HxStr(kOwnScreenName));
        return;
    }

    if (mStageLocked[mStageList->mSelected] == 0) {
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kHelpScreen));
        BeginExit();
        mUnknown18 = kExitSelected;
    } else {
        ActivateNamedPanel(HxStr(kOwnScreenName));
    }
}

// 0x003aee50
void MetSoloStagesScreen::OnUnknownSlot33() {
    mTvView->SetShowing(1);
    mScrollDirection = kScrollNone;
    LoadLevelTextures();
    MetHelpScreen::SetText(mUnknown38[0], mUnknown10->mUnknown68);
}

// 0x003a6738
void MetSoloStagesScreen::OnUnknownSlot36() {
    if (mUnknown18 == kExitBack) {
        if (Application::shared()->GetGameManager()->GetParams()->mUnknown1c == kPlayModeGame) {
            PushNamedScreen(HxStr(kLeftGizmoScreen));
            PushNamedScreen(HxStr(kGameSkillScreen));
            ActivateNamedPanel(HxStr(kGameSkillScreen));
        } else {
            PushNamedScreen(HxStr(kLeftGizmoScreen));
            PushNamedScreen(HxStr(kRemixTypeScreen));
            ActivateNamedPanel(HxStr(kRemixTypeScreen));
        }
    } else if (mStageList->mSelected == kCustomStageIndex) {
        std::vector<HxStr> screens;
        screens.resize(kRemixReturnScreenCount);
        screens[0] = kRemixLoadScreen;
        screens[1] = kTitleScreen;
        screens[2] = kRemixDataScreen;
        screens[3] = kHelpScreen;
        std::vector<MemcardConnectState> slots;
        MemcardConnectState disc;
        disc.mSlotName = kDiscSlotName;
        disc.mPortSlot = kDiscSlot;
        AddCardSlot(slots);
        slots.push_back(disc);
        MetRemixManager::shared()->ListRemixes(screens, slots, kSkipPlayList);
    } else if (Application::shared()->GetGameManager()->GetPersonas()->size() >=
               kLoadGamePersonaCount) {
        GameParams params(CurrentParams());
        const std::vector<ArenaListEntry> *pArenas = GetArenaList();
        params.mArenaName = (*GetArenaList())[pArenas->size() - 1].mName;
        Application::shared()->GetGameManager()->SetParams(params);
        PushNamedScreen(HxStr(kLoadGameScreen));
        ActivateNamedPanel(HxStr(kLoadGameScreen));
    } else {
        MetFrontEndState::shared()->mUnknown24 = HxStr(kOwnScreenName);
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kArenasScreen));
        ActivateNamedPanel(HxStr(kArenasScreen));
    }
    mWarningText->SetText(HxStr(kNoText));
    mBioText->SetText(HxStr(kNoText));
}
