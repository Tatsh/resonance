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
#ifdef VIDEO_STANDARD_PAL
// From this many stage 1 levels, completed levels alone can raise the level to
// kStageOneCompleteUnlock and open stage 2.
constexpr int kLongStageOneLevelCount = 6;
#endif

// The text GetBonusLevelName() reports when a stage has no bonus level.
static const char *const kNoName = "";

} // namespace

// NTSC-U/C: 0x00140580, PAL: 0x00140f60
CampaignStats::CampaignStats() {
    ResetCounters();
}

// NTSC-U/C: 0x00140768, PAL: 0x00141148
CampaignStats::~CampaignStats() {
}

// NTSC-U/C: 0x00145110, PAL: 0x00145c28
void CampaignStats::Save(OBStream &stream) {
    int nVersion = kRecordVersion;
    stream.Write(&nVersion, sizeof(nVersion));

    int nLevelCount = mLevels.size();
    stream.Write(&nLevelCount, sizeof(nLevelCount));

    for (std::vector<LevelStats>::iterator it = mLevels.begin(); it != mLevels.end(); ++it) {
        it->Save(stream);
    }
}

// NTSC-U/C: 0x00141ed8, PAL: 0x001429c8
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

// NTSC-U/C: 0x00144f08, PAL: 0x00145a20
int CampaignStats::GetLevelBeaten(int nDifficulty, const HxStr &name) {
    return mLevels[FindLevelIndex(name)].mSkills[nDifficulty].mBeaten;
}

// NTSC-U/C: 0x00144ca8, PAL: 0x001457c0
int CampaignStats::GetLevelHighScore(int nDifficulty, const HxStr &name) {
    return mLevels[FindLevelIndex(name)].mSkills[nDifficulty].mHighScore;
}

// NTSC-U/C: 0x00141798, PAL: 0x00142210
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

// NTSC-U/C: 0x00144d68, PAL: 0x00145880
void CampaignStats::RecordHighScore(int nDifficulty, const HxStr &name, int nScore) {
    SkillStats &skill = mLevels[FindLevelIndex(name)].mSkills[nDifficulty];
    if (skill.mHighScore < nScore) {
        skill.mHighScore = nScore;
        RecountStageScore(nDifficulty, GetAlbumLevelStage(name));
    }
}

// NTSC-U/C: 0x00145028, PAL: 0x00145b40
int CampaignStats::GetStageScoreBeaten(int nDifficulty, int nStage) {
    return mStageScoreBeaten[nDifficulty][nStage - kFirstStage];
}

// NTSC-U/C: 0x00145048, PAL: 0x00145b60
int CampaignStats::GetStageScore(int nDifficulty, int nStage) {
    return mStageScores[nDifficulty][nStage - kFirstStage];
}

// NTSC-U/C: 0x00144e58, PAL: 0x00145970
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

// NTSC-U/C: 0x00140d40, PAL: 0x00141740
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

// NTSC-U/C: 0x00141690, PAL: 0x001420e8
HxStr CampaignStats::GetBonusLevelName(int nDifficulty, int nStage) {
    int nCount = GetStageList(nStage)->size();
    int nBonus = GetAlbumLevelValue(nStage - kFirstStage, nDifficulty);
    if (nCount == 0 || nBonus == 0) {
        return HxStr(kNoName);
    }
    HxStr name((*GetStageList(nStage))[nCount - 1].mName);
    return name;
}

// NTSC-U/C: 0x001410f0, PAL: 0x00141b10
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

// NTSC-U/C: 0x00141b90, PAL: 0x00142648
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

// NTSC-U/C: 0x00141898, PAL: 0x00142310
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

// NTSC-U/C: 0x00141cb8, PAL: 0x00142780
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
#ifdef VIDEO_STANDARD_PAL
        if (mStageLevelCounts[kFirstStageIndex] < kLongStageOneLevelCount) {
            nLevel = std::min(nCompleted, kMaxStageOneUnlock);
        } else {
            nLevel = std::min(nCompleted, kStageOneCompleteUnlock);
        }
#else
        nLevel = std::min(nCompleted, kMaxStageOneUnlock);
#endif
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

// NTSC-U/C: 0x00140a68, PAL: 0x00141448
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

// NTSC-U/C: 0x00145068, PAL: 0x00145b80
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

// NTSC-U/C: 0x00140b40, PAL: 0x00141520
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

// NTSC-U/C: 0x00144c38, PAL: 0x00145750
int CampaignStats::IsLevelListed(const HxStr &name) {
    std::vector<HxStr>::iterator end = GetLevelNames().end();
    for (std::vector<HxStr>::iterator it = GetLevelNames().begin(); it != end; ++it) {
        if (*it == name) {
            return 1;
        }
    }
    return 0;
}

// NTSC-U/C: 0x00140908, PAL: 0x001412e8
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

// NTSC-U/C: 0x00142070, PAL: 0x00142b60
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

// NTSC-U/C: 0x00140ef8, PAL: 0x00141918
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

// NTSC-U/C: 0x00141578, PAL: 0x00141fd0
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

// NTSC-U/C: 0x00142288, PAL: 0x00142d78
void CampaignStats::Assign(const CampaignStats &other) {
    mLevels.clear();
    mLevels.resize(other.mLevels.size(), LevelStats());
    for (unsigned i = 0; i < mLevels.size(); ++i) {
        mLevels[i].Assign(other.mLevels[i]);
    }
    RecountAll();
}

// NTSC-U/C: 0x001451e0, PAL: 0x00145cf8
void CampaignStats::PrintLevels(std::ostream &stream) {
    for (unsigned i = 0; i < mLevels.size(); ++i) {
        std::ostream &line = stream << "levels[" << static_cast<int>(i) << "]";
        mLevels[i].Print(line);
        line << std::endl;
    }
}
