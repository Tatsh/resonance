#include "met/metsolostatsscreen.h"

#include "app/application.h"
#include "game/freqappearance.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "game/gamestats.h"
#include "met/metfrontendstate.h"
#include "met/metpersonadata.h"
#include "met/metremixmanager.h"
#include "met/metremixrecord.h"
#include "met/metsonglists.h"
#include "met/metstrings.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/object.h"
#include "rnd/text.h"
#include "script/configquery.h"

namespace {

// The screen name, the directory the container loads from, and the container name.
static const char *const kScreenName = "egs";
static const char *const kDirectory = "metagame/_Solo";
static const char *const kContainerName = "end_game_stats";

// The heading texts ResolveContainerViews() labels, each beside the key its label is read under.
static const char *const kPanelHeading = "egs_stats_pan.txt";
static const char *const kPanelLabel = "egs_panel_label";
static const char *const kScoreHeading = "egs_score_head.txt";
static const char *const kScoreLabel = "egs_score_label";
static const char *const kSongHeading = "egs_song_head.txt";
static const char *const kSongLabel = "egs_song_label";
static const char *const kSkillHeading = "egs_skill_head.txt";
static const char *const kSkillLabel = "egs_skill_label";
static const char *const kFirstCompleteHeading = "egs_complete_head_01.txt";
static const char *const kFirstCompleteLabel = "egs_complete1_label";
static const char *const kSecondCompleteHeading = "egs_complete_head_02.txt";
static const char *const kSecondCompleteLabel = "egs_complete2_label";
static const char *const kFirstPhraseHeading = "egs_phrase_head_01.txt";
static const char *const kFirstPhraseLabel = "egs_phrase1_label";
static const char *const kSecondPhraseHeading = "egs_phrase_head_02.txt";
static const char *const kSecondPhraseLabel = "egs_phrase2_label";
static const char *const kHotHeading = "egs_hot_head.txt";
static const char *const kHotLabel = "egs_hottest_label";

// The objects ResolveContainerViews() stores.
static const char *const kFreqMat = "freq.mat";
static const char *const kFreqNameText = "egs_freqname.txt";
static const char *const kRankMat = "egs_rank.mat";
static const char *const kScoreText = "egs_score_val.txt";
static const char *const kSongText = "egs_song_val.txt";
static const char *const kSkillText = "egs_skill_val.txt";
static const char *const kCompleteText = "egs_complete_val.txt";
static const char *const kPhraseText = "egs_phrase_val.txt";
static const char *const kHotText = "egs_hot_val.txt";

// Configuration codes the headings and the song title are read under.
constexpr int kLabelConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x325;
constexpr int kShortTitleConfigCode = 0x327;

static const char *const kCountFormat = "%d";
static const char *const kPercentFormat = "%3d%%";
constexpr float kPercentScale = 100.0f;

// The player whose statistics the screen shows, the persona burn texture it takes, and the freq
// material stage that texture is shown on.
constexpr int kFirstPlayer = 0;
constexpr int kFirstBurnTexture = 0;
constexpr int kFreqBurnStage = 1;

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

// Labels one heading text from configuration code 0x258, or from the current language in the
// European release. The lookup is not tested for null.
inline void LabelHeading(const char *pszTextName, MetStringId nLabelId, const char *pszLabelKey) {
    Rnd::Text *pText = FindObject<Rnd::Text>(pszTextName);
    HxStr label = MetConfigText(nLabelId, kLabelConfigCode, pszLabelKey);
    pText->SetText(label);
}

} // namespace

MetSoloStatsScreen::MetSoloStatsScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
}

MetSoloStatsScreen::~MetSoloStatsScreen() {
}

MetSoloStatsScreen *MetSoloStatsScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetSoloStatsScreen(pRenderer, nPriority);
}

void MetSoloStatsScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();

    LabelHeading(kPanelHeading, kMetStrEgsPanelLabel, kPanelLabel);
    LabelHeading(kScoreHeading, kMetStrEgsScoreLabel, kScoreLabel);
    LabelHeading(kSongHeading, kMetStrEgsSongLabel, kSongLabel);
    LabelHeading(kSkillHeading, kMetStrEgsSkillLabel, kSkillLabel);
    LabelHeading(kFirstCompleteHeading, kMetStrEgsComplete1Label, kFirstCompleteLabel);
    LabelHeading(kSecondCompleteHeading, kMetStrEgsComplete2Label, kSecondCompleteLabel);
    LabelHeading(kFirstPhraseHeading, kMetStrEgsPhrase1Label, kFirstPhraseLabel);
    LabelHeading(kSecondPhraseHeading, kMetStrEgsPhrase2Label, kSecondPhraseLabel);
    LabelHeading(kHotHeading, kMetStrEgsHottestLabel, kHotLabel);

    mFreqMat = FindObject<Rnd::Mat>(kFreqMat);
    mBurnTex = FreqAppearance::FindPersonaBurnTexture(kFirstBurnTexture);
    mFreqNameText = FindObject<Rnd::Text>(kFreqNameText);
    mRankMat = FindObject<Rnd::Mat>(kRankMat);
    mScoreText = FindObject<Rnd::Text>(kScoreText);
    mSongText = FindObject<Rnd::Text>(kSongText);
    mSkillText = FindObject<Rnd::Text>(kSkillText);
    mCompleteText = FindObject<Rnd::Text>(kCompleteText);
    mPhraseText = FindObject<Rnd::Text>(kPhraseText);
    mHotText = FindObject<Rnd::Text>(kHotText);
}

void MetSoloStatsScreen::EnterAndShow() {
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    GameStats *pStats = Application::shared()->GetGameManager()->GetStats();
    MetPersonaData *pPersona = MetFrontEndState::shared()->GetFirstPersona();

    HxStr name;
    name = pPersona->mAppearance.mUserName;
    mFreqNameText->SetText(name);
    pPersona->AttachToBurnSlot(kFirstBurnTexture);
    mFreqMat->mStages[kFreqBurnStage].SetTex(mBurnTex);

    HxStr song;
    if (params.mLoadingGame != 0) {
        MetRemixRecord record(*MetRemixManager::shared()->GetRecord());
        song = record.name;
    } else {
        HxStr title = QueryConfigString(kTitleConfigCode, TextOf(params.mLevelName));
        song = title;
    }
    const float flWrapWidth = mSongText->mWrapWidth;
    if (flWrapWidth < mSongText->GetFontWidth(TextOf(song), song.mLen)) {
        HxStr shorter = QueryConfigString(kShortTitleConfigCode, TextOf(params.mLevelName));
        song = shorter;
    }
    mSongText->SetText(song);

    mSkillText->SetText(DifficultyName(params.mDifficulty));
    mScoreText->SetText(HxStr(Rnd::MakeString(kCountFormat, pStats->GetScore(kFirstPlayer))));
    mCompleteText->SetText(HxStr(
        Rnd::MakeString(kPercentFormat, static_cast<int>(pStats->GetProgress() * kPercentScale))));
    mPhraseText->SetText(HxStr(Rnd::MakeString(
        kPercentFormat, static_cast<int>(pStats->GetRatio(kFirstPlayer) * kPercentScale))));
    mHotText->SetText(HxStr(Rnd::MakeString(kCountFormat, pStats->GetTally(kFirstPlayer))));

    MetScreen::EnterAndShow();
}
