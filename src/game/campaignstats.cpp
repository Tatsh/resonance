#include "game/campaignstats.h"

#include <algorithm>

#include "game/gameparams.h"
#include "met/albumcache.h"
#include "met/metsonglists.h"

namespace {

constexpr int kRecordVersion = 2;

// The difficulties that limit which stages count.
constexpr int kDifficultyEasy = 0;
constexpr int kDifficultyNormal = 1;

// The last stage each difficulty plays. The secret stage follows the last regular stage.
constexpr int kEasyLastStage = 3;
constexpr int kNormalLastStage = 4;
constexpr int kLastRegularStage = 5;

// The stages UpdateUnlockLevel() tests, counted from 1, and the index of the first in the
// per-stage arrays.
constexpr int kFirstStage = 1;
constexpr int kSecondStage = 2;
constexpr int kThirdStage = 3;
constexpr int kFourthStage = 4;
constexpr int kFirstStageIndex = kFirstStage - 1;

// The number of levels the secret stage needs before its first level unlocks the super secret.
constexpr unsigned kSuperSecretMinimumLevels = 2;

// The unlock levels UpdateUnlockLevel() computes. Before any stage is complete, the level starts
// at kFewLevelsUnlock or kManyLevelsUnlock, depending on whether stage 1 has more than
// kManyLevelsThreshold levels, and rises with the completed count up to kMaxStageOneUnlock.
constexpr int kManyLevelsThreshold = 3;
constexpr int kManyLevelsUnlock = 1;
constexpr int kFewLevelsUnlock = 2;
constexpr int kMaxStageOneUnlock = 4;
constexpr int kStageOneCompleteUnlock = 5;

// The text GetBonusLevelName() reports when a stage has no bonus level.
static const char *const kNoName = "";

} // namespace

// 0x00140580
CampaignStats::CampaignStats() {
    ResetCounters();
}

// 0x00140768
CampaignStats::~CampaignStats() {
}

// 0x00145110
void CampaignStats::Save(OBStream &stream) {
    int nVersion = kRecordVersion;
    stream.Write(&nVersion, sizeof(nVersion));

    int nLevelCount = mLevels.size();
    stream.Write(&nLevelCount, sizeof(nLevelCount));

    for (std::vector<LevelStats>::iterator it = mLevels.begin(); it != mLevels.end(); ++it) {
        it->Save(stream);
    }
}

// 0x00141ed8
void CampaignStats::Load(IBStream &stream) {
    stream.Read(&g_nStatsRecordVersion, sizeof(g_nStatsRecordVersion));

    int nLevelCount;
    stream.Read(&nLevelCount, sizeof(nLevelCount));
    mLevels.resize(nLevelCount);

    for (std::vector<LevelStats>::iterator it = mLevels.begin(); it != mLevels.end(); ++it) {
        it->Load(stream);
    }

    MergeLevelList();
}

// 0x00144f08
int CampaignStats::GetLevelBeaten(int nDifficulty, const HxStr &name) {
    return mLevels[FindLevelIndex(name)].mSkills[nDifficulty].mBeaten;
}

// 0x00144ca8
int CampaignStats::GetLevelHighScore(int nDifficulty, const HxStr &name) {
    return mLevels[FindLevelIndex(name)].mSkills[nDifficulty].mHighScore;
}

// 0x00141798
void CampaignStats::RecordLevelBeaten(int nDifficulty, const HxStr &name) {
    int nIndex = FindLevelIndex(name);
    int nStage = GetAlbumLevelStage(name);
    SkillStats &skill = mLevels[nIndex].mSkills[nDifficulty];
    if (skill.mBeaten != 0) {
        return;
    }
    skill.mBeaten = 1;
    RecountStageCompleted(nDifficulty, nStage);
    RecountStageScore(nDifficulty, nStage);
    UpdateUnlockLevel();
}

// 0x00144d68
void CampaignStats::RecordHighScore(int nDifficulty, const HxStr &name, int nScore) {
    SkillStats &skill = mLevels[FindLevelIndex(name)].mSkills[nDifficulty];
    if (skill.mHighScore < nScore) {
        skill.mHighScore = nScore;
        RecountStageScore(nDifficulty, GetAlbumLevelStage(name));
    }
}

// 0x00145028
int CampaignStats::GetStageScoreBeaten(int nDifficulty, int nStage) {
    return mStageScoreBeaten[nDifficulty][nStage - kFirstStage];
}

// 0x00145048
int CampaignStats::GetStageScore(int nDifficulty, int nStage) {
    return mStageScores[nDifficulty][nStage - kFirstStage];
}

// 0x00144e58
int CampaignStats::IsStageComplete(int nDifficulty, int nStage) {
    if ((nDifficulty == kDifficultyEasy && nStage > kEasyLastStage) ||
        (nDifficulty == kDifficultyNormal && nStage > kNormalLastStage) ||
        nStage > kLastRegularStage) {
        return 0;
    }
    int nRequired = mStageLevelCounts[nStage - kFirstStage];
    if (GetAlbumLevelValue(nStage - kFirstStage, nDifficulty) != 0) {
        --nRequired;
    }
    return mStageCompleted[nDifficulty][nStage - kFirstStage] >= nRequired;
}

// 0x00140d40
int CampaignStats::IsDifficultyComplete(int nDifficulty) {
    int nStageCount = nDifficulty + kEasyLastStage;
    int nComplete = 0;
    for (int nStage = kFirstStage; nStage <= nStageCount; ++nStage) {
        nComplete += IsStageComplete(nDifficulty, nStage);
    }

    int bComplete = 0;
    if (nComplete == nStageCount) {
        bComplete = 1;
        for (int nStage = kFirstStage; nStage <= nStageCount; ++nStage) {
            HxStr name = GetBonusLevelName(nDifficulty, nStage);
            if (name != kNoName) {
                bComplete = bComplete && GetLevelBeaten(nDifficulty, name) != 0;
            }
        }
    }
    return bComplete;
}

// 0x00141690
HxStr CampaignStats::GetBonusLevelName(int nDifficulty, int nStage) {
    int nCount = GetStageList(nStage)->size();
    int nBonus = GetAlbumLevelValue(nStage - kFirstStage, nDifficulty);
    if (nCount == 0 || nBonus == 0) {
        return HxStr(kNoName);
    }
    HxStr name((*GetStageList(nStage))[nCount - 1].mName);
    return name;
}

// 0x001410f0
int CampaignStats::IsSuperSecretUnlocked() {
    int bUnlocked = 0;
    if (!IsSecretUnlocked()) {
        return bUnlocked;
    }
    std::vector<StageListEntry> levels(*GetStageList(kSecretStage));
    HxStr name(levels[0].mName);
    if (mLevels[FindLevelIndex(name)].mSkills[kDifficultyExpert].mBeaten != 0) {
        bUnlocked = levels.size() >= kSuperSecretMinimumLevels;
    }
    return bUnlocked;
}

// 0x00141b90
void CampaignStats::RecountStageCompleted(int nDifficulty, int nStage) {
    std::vector<int> &levels = mStageLevels[nStage - kFirstStage];
    int nCount = levels.size();
    int nBeaten = 0;
    for (int i = 0; i < nCount; ++i) {
        int nIndex = levels[i];
        HxStr name(mLevels[nIndex].mName);
        (void)name; // Yes, the binary copies the name and never reads it.
        if (mLevels[nIndex].mSkills[nDifficulty].mBeaten != 0) {
            ++nBeaten;
        }
    }
    mStageCompleted[nDifficulty][nStage - kFirstStage] = nBeaten;
}

// 0x00141898
void CampaignStats::RecountStageScore(int nDifficulty, int nStage) {
    if (nStage > kLastRegularStage) {
        return;
    }
    int &nTotal = mStageScores[nDifficulty][nStage - kFirstStage];
    nTotal = 0;
    std::vector<int> &levels = mStageLevels[nStage - kFirstStage];
    int nCount = levels.size();
    for (int i = 0; i < nCount; ++i) {
        int nIndex = levels[i];
        HxStr name(mLevels[nIndex].mName);
        if (mLevels[nIndex].mSkills[nDifficulty].mBeaten != 0) {
            nTotal += mLevels[FindLevelIndex(name)].mSkills[nDifficulty].mHighScore;
        }
    }

    int nTarget = GetAlbumLevelValue(nStage - kFirstStage, nDifficulty);
    if (nTarget != 0 && nTotal >= nTarget && IsStageComplete(nDifficulty, nStage)) {
        mStageScoreBeaten[nDifficulty][nStage - kFirstStage] = 1;
    } else {
        mStageScoreBeaten[nDifficulty][nStage - kFirstStage] = 0;
    }
}

// 0x00141cb8
int CampaignStats::UpdateUnlockLevel() {
    int nLevel = mStageLevelCounts[kFirstStageIndex] > kManyLevelsThreshold ? kManyLevelsUnlock :
                                                                              kFewLevelsUnlock;
    if (IsStageComplete(kDifficultyEasy, kFirstStage) ||
        IsStageComplete(kDifficultyNormal, kFirstStage) ||
        IsStageComplete(kDifficultyExpert, kFirstStage)) {
        nLevel = kStageOneCompleteUnlock;
    } else {
        int nCompleted = mStageCompleted[kDifficultyEasy][kFirstStageIndex] +
                         mStageCompleted[kDifficultyNormal][kFirstStageIndex] +
                         mStageCompleted[kDifficultyExpert][kFirstStageIndex] + nLevel;
        nLevel = std::min(nCompleted, kMaxStageOneUnlock);
    }

    // Only the listed difficulties are tested at stages 3 and 4, which matches the binary.
    if (nLevel == kStageOneCompleteUnlock && (IsStageComplete(kDifficultyEasy, kSecondStage) ||
                                              IsStageComplete(kDifficultyNormal, kSecondStage) ||
                                              IsStageComplete(kDifficultyExpert, kSecondStage))) {
        ++nLevel;
        if (IsStageComplete(kDifficultyNormal, kThirdStage) ||
            IsStageComplete(kDifficultyExpert, kThirdStage)) {
            ++nLevel;
            if (IsStageComplete(kDifficultyExpert, kFourthStage)) {
                ++nLevel;
            }
        }
    }
    mUnlockLevel = nLevel;
    return nLevel;
}

// 0x00140a68
void CampaignStats::ResetCounters() {
    for (int i = 0; i < kStageCount; ++i) {
        mStageLevelCounts[i] = 0;
        for (int nDifficulty = 0; nDifficulty < kDifficultyCount; ++nDifficulty) {
            mStageCompleted[nDifficulty][i] = 0;
            mStageScoreBeaten[nDifficulty][i] = 0;
            mStageScores[nDifficulty][i] = 0;
        }
    }

    // Yes, the binary fetches the global level list once for each end of the walk.
    std::vector<HxStr>::iterator end = GetLevelNames().end();
    for (std::vector<HxStr>::iterator it = GetLevelNames().begin(); it != end; ++it) {
        if (IsAlbumLevel(*it)) {
            ++mStageLevelCounts[GetAlbumLevelStage(*it) - kFirstStage];
        }
    }
    RecountAll();
}

// 0x00145068
void CampaignStats::RecountAll() {
    RebuildStageLevels();
    for (int nStage = kFirstStage; nStage <= kLastRegularStage; ++nStage) {
        RecountStageCompleted(kDifficultyEasy, nStage);
        RecountStageCompleted(kDifficultyNormal, nStage);
        RecountStageCompleted(kDifficultyExpert, nStage);
        RecountStageScore(kDifficultyEasy, nStage);
        RecountStageScore(kDifficultyNormal, nStage);
        RecountStageScore(kDifficultyExpert, nStage);
    }
    UpdateUnlockLevel();
}

// 0x00140b40
void CampaignStats::RebuildStageLevels() {
    for (int i = 0; i < kIndexedStageCount; ++i) {
        mStageLevels[i].erase(mStageLevels[i].begin(), mStageLevels[i].end());
    }

    for (unsigned i = 0; i < mLevels.size(); ++i) {
        HxStr name(mLevels[i].mName);
        const int bAlbum = IsLevelListed(name) ? IsAlbumLevel(name) : 0;
        const int nStageIndex = mLevels[i].mStage - kFirstStage;
        // Yes, the binary tests the global level list a second time.
        if (bAlbum && IsLevelListed(name) && nStageIndex < kIndexedStageCount) {
            mStageLevels[nStageIndex].push_back(static_cast<int>(i));
        }
    }
}

// 0x00144c38
int CampaignStats::IsLevelListed(const HxStr &name) {
    std::vector<HxStr>::iterator end = GetLevelNames().end();
    for (std::vector<HxStr>::iterator it = GetLevelNames().begin(); it != end; ++it) {
        if (*it == name) {
            return 1;
        }
    }
    return 0;
}

// 0x00140908
void CampaignStats::RebuildLevelList() {
    mLevels.clear();
    LevelStats level;
    std::vector<HxStr>::iterator end = GetLevelNames().end();
    for (std::vector<HxStr>::iterator it = GetLevelNames().begin(); it != end; ++it) {
        if (IsAlbumLevel(*it)) {
            level.Reset();
            level.mName = *it;
            level.mStage = GetAlbumLevelStage(*it);
            mLevels.push_back(level);
        }
    }
}

// 0x00142070
void CampaignStats::MergeLevelList() {
    std::vector<HxStr>::iterator end = GetLevelNames().end();
    for (std::vector<HxStr>::iterator it = GetLevelNames().begin(); it != end; ++it) {
        unsigned nIndex = 0;
        while (nIndex < mLevels.size() && !(mLevels[nIndex].mName == *it)) {
            ++nIndex;
        }
        if (nIndex < mLevels.size()) {
            mLevels[nIndex].mStage = GetAlbumLevelStage(*it);
        } else {
            LevelStats level;
            level.Reset();
            level.mName = *it;
            level.mStage = GetAlbumLevelStage(*it);
            mLevels.push_back(level);
        }
    }
    RecountAll();
}

// 0x00140ef8
int CampaignStats::IsStageCompleteAtAnyDifficulty(int nStage) {
    if (nStage < kThirdStage) {
        return IsStageComplete(kDifficultyEasy, nStage) ||
               IsStageComplete(kDifficultyNormal, nStage) ||
               IsStageComplete(kDifficultyExpert, nStage);
    }
    if (nStage == kThirdStage) {
        return IsStageComplete(kDifficultyNormal, nStage) ||
               IsStageComplete(kDifficultyExpert, nStage);
    }
    if (nStage <= kLastRegularStage) {
        return IsStageComplete(kDifficultyExpert, nStage);
    }
    return 0;
}

// 0x00141578
int CampaignStats::IsLastLevelRemaining(const GameParams &params) {
    const int nDifficulty = params.mDifficulty;
    if (mLevels[FindLevelIndex(params.mLevelName)].mSkills[nDifficulty].mBeaten != 0) {
        return 0;
    }
    int nLevels = 0;
    int nCompleted = 0;
    for (int i = 0; i < nDifficulty + kEasyLastStage; ++i) {
        nLevels += mStageLevelCounts[i];
        nCompleted += mStageCompleted[nDifficulty][i];
    }
    return nCompleted == nLevels - 1;
}

// 0x00142288
void CampaignStats::Assign(const CampaignStats &other) {
    mLevels.clear();
    mLevels.resize(other.mLevels.size(), LevelStats());
    for (unsigned i = 0; i < mLevels.size(); ++i) {
        mLevels[i].Assign(other.mLevels[i]);
    }
    RecountAll();
}

// 0x001451e0
void CampaignStats::PrintLevels(std::ostream &stream) {
    for (unsigned i = 0; i < mLevels.size(); ++i) {
        std::ostream &line = stream << "levels[" << static_cast<int>(i) << "]";
        mLevels[i].Print(line);
        line << std::endl;
    }
}
