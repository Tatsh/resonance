#include "game/campaignstats.h"

namespace {

constexpr int kRecordVersion = 2;

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

// 0x00144ba8
int CampaignStats::FindLevelIndex(const HxStr &name) {
    unsigned nIndex = 0;
    while (nIndex < mLevels.size()) {
        if (mLevels[nIndex].mName == name) {
            break;
        }
        ++nIndex;
    }
    return static_cast<int>(nIndex);
}
