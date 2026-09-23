#include "met/metmultistatsscreen.h"

#include <vector>

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
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/text.h"
#include "script/configquery.h"

namespace {

static const char *const kScreenName = "ems";
static const char *const kDirectory = "metagame/_Local";
static const char *const kContainerName = "end_multi_stats";

static const char *const kSongHeading = "emd_song_head.txt";
static const char *const kSongHeadingKey = "egs_song_label";
static const char *const kSongValue = "emd_song_val.txt";
static const char *const kSkillValue = "emd_skill_val.txt";
static const char *const kGamePanel = "emd_game_data_pan.txt";
static const char *const kGamePanelKey = "egm_game_label";
static const char *const kSkillHeading = "emd_skill_head.txt";
static const char *const kSkillHeadingKey = "egs_skill_label";
static const char *const kScoresPanel = "ems_scores_pan.txt";
static const char *const kScoresPanelKey = "egm_game_player_label";

// Counted from 1.
static const char *const kScoreTextFormat = "ems_score_0%d.txt";
static const char *const kNameTextFormat = "ems_freqname_0%d.txt";
static const char *const kPictureMaterialFormat = "ems_freq_0%d.mat";
static const char *const kMeshFormat = "ems_freq_0%d.mesh";

static const char *const kScoreFormat = "%d";
static const char *const kNoText = "";

constexpr int kPromptConfigCode = 0x258;
constexpr int kSongNameConfigCode = 0x325;
constexpr int kShortSongNameConfigCode = 0x327;

// The four player rows.
constexpr int kRowCount = 4;
// The stage of a row's material that takes the persona picture.
constexpr int kBurnStage = 1;
constexpr int kFirstPlayer = 0;

const Color kPlayerColors[] = {
    {0.0f, 1.0f, 0.0f, 1.0f},
    {0.65f, 0.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 0.0f, 1.0f},
    {1.0f, 0.0f, 0.0f, 1.0f},
};

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

inline Rnd::Text *FindText(const char *pszName) {
    return dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(pszName)));
}

// Yes, the binary does not test the text for null.
inline void FillText(const char *pszText, const char *pszKey) {
    Rnd::Text *pText = FindText(pszText);
    HxStr text = QueryConfigString(kPromptConfigCode, pszKey);
    pText->SetText(text);
}

} // namespace

// 0x002ff228
MetMultiStatsScreen::MetMultiStatsScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
}

// 0x002ff3f8
void MetMultiStatsScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    FillText(kSongHeading, kSongHeadingKey);
    mUnknown8c = FindText(kSongValue);
    mUnknown90 = FindText(kSkillValue);
    FillText(kGamePanel, kGamePanelKey);
    FillText(kSkillHeading, kSkillHeadingKey);
    FillText(kScoresPanel, kScoresPanelKey);

    for (int nRow = 0; nRow < kRowCount; ++nRow) {
        Rnd::Text *pScore = FindText(FormatString(kScoreTextFormat, nRow + 1));
        mUnknown94.push_back(pScore);
        pScore->SetText(HxStr(kNoText));

        Rnd::Text *pName = FindText(FormatString(kNameTextFormat, nRow + 1));
        mUnknowna0.push_back(pName);
        pName->SetText(HxStr(kNoText));

        Rnd::Mat *pMaterial = dynamic_cast<Rnd::Mat *>(
            Rnd::g_manager.Find(HxStr(FormatString(kPictureMaterialFormat, nRow + 1))));
        mUnknownac.push_back(pMaterial);

        Rnd::Mesh *pMesh = dynamic_cast<Rnd::Mesh *>(
            Rnd::g_manager.Find(HxStr(FormatString(kMeshFormat, nRow + 1))));
        mUnknownb8.push_back(pMesh);
    }

    mUnknownd0.resize(kRowCount);
    // The binary expands this loop into one store per component.
    for (int nRow = 0; nRow < kRowCount; ++nRow) {
        mUnknownd0[nRow] = kPlayerColors[nRow];
    }
}

// 0x002fff90
MetMultiStatsScreen::~MetMultiStatsScreen() {
}

// 0x00300268
void MetMultiStatsScreen::EnterAndShow() {
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    GameStats *pStats = Application::shared()->GetGameManager()->GetStats();

    HxStr songName;
    if (params.mLoadingGame != 0) {
        MetRemixRecord record(*MetRemixManager::shared()->GetRecord());
        songName = record.name;
    } else {
        HxStr text = QueryConfigString(kSongNameConfigCode, TextOrEmpty(params.mLevelName));
        songName = text;
        const float flWrapWidth = mUnknown8c->mWrapWidth;
        if (flWrapWidth < mUnknown8c->MeasureText(TextOrEmpty(songName), songName.mLen)) {
            HxStr shorter =
                QueryConfigString(kShortSongNameConfigCode, TextOrEmpty(params.mLevelName));
            songName = shorter;
        }
    }
    mUnknown8c->SetText(songName);
    mUnknown90->SetText(DifficultyName(params.mDifficulty));

    mUnknownc4.erase(mUnknownc4.begin(), mUnknownc4.end());
    mUnknownc4.insert(mUnknownc4.begin(), kFirstPlayer);
    for (int nPlayer = 1; nPlayer < pStats->mPlayerCount; ++nPlayer) {
        const int nScore = pStats->GetScore(nPlayer);
        for (std::vector<int>::iterator it = mUnknownc4.begin(); it != mUnknownc4.end(); ++it) {
            if (pStats->GetScore(*it) < nScore) {
                mUnknownc4.insert(it, nPlayer);
                break;
            }
            if (it + 1 == mUnknownc4.end()) {
                mUnknownc4.push_back(nPlayer);
                break;
            }
        }
    }

    std::vector<HxStr> names;
    std::vector<Rnd::Tex *> pictures;
    if (params.mUnknown28 == 0) {
        std::vector<MetPersonaData *> personas(MetFrontEndState::shared()->mUnknown00);
        for (std::vector<MetPersonaData *>::size_type i = 0; i < personas.size(); ++i) {
            MetPersonaData *pPersona = personas[i];
            names.push_back(pPersona->mUnknown140.mUnknown00);
            Rnd::Tex *pPicture = FreqAppearance::FindPersonaBurnTexture(i);
            pPersona->AttachToBurnSlot(i);
            pictures.push_back(pPicture);
        }
    }

    for (int nRow = 0; nRow < kRowCount; ++nRow) {
        if (nRow < pStats->mPlayerCount) {
            const int nPlayer = mUnknownc4[nRow];
            if (params.mUnknown1c == kPlayModeGame) {
                mUnknown94[nRow]->SetText(
                    HxStr(FormatString(kScoreFormat, pStats->GetScore(nPlayer))));
            } else {
                mUnknown94[nRow]->SetText(HxStr(kNoText));
            }
            Rnd::Tex *pPicture = pictures[nPlayer];
            mUnknownb8[nRow]->SetShowing(1);
            mUnknownb8[nRow]->SetVertexColor(mUnknownd0[nPlayer]);
            mUnknownac[nRow]->mStages[kBurnStage].SetTex(pPicture);
            mUnknowna0[nRow]->SetText(names[nPlayer]);
        } else {
            mUnknown94[nRow]->SetText(HxStr(kNoText));
            mUnknowna0[nRow]->SetText(HxStr(kNoText));
            mUnknownb8[nRow]->SetShowing(0);
        }
    }

    MetScreen::EnterAndShow();
}

// 0x00306910
MetMultiStatsScreen *MetMultiStatsScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMultiStatsScreen(pRenderer, nPriority);
}

// 0x00306998
void MetMultiStatsScreen::OnUnknownSlot36() {
}
