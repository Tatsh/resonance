#include "met/albumcache.h"

#include <algorithm>
#include <map>
#include <vector>

#include "os/hxstr.h"
#include "script/configquery.h"

namespace {

// Configuration codes the caches are filled from.
constexpr int kLevelStageConfigCode = 0x25d;
constexpr int kLevelValueConfigCode = 0x27d;
constexpr int kJukeboxConfigCode = 0x514;

// A cache word that has not been read yet.
constexpr int kNotCached = -2;

constexpr int kDifficultyCount = 3;
constexpr int kAlbumLevelCount = 10;

// 0x006db590
std::map<HxStr, int> g_albumLevelStages;

// 0x006db5a0
std::vector<int> g_albumLevelValues(kAlbumLevelCount *kDifficultyCount, kNotCached);

// 0x006db5ac
int g_nAlbumJukeboxValue = kNotCached;

// 0x003f6760. The one caller is GetAlbumLevelStage().
int &LevelStageEntry(const HxStr &name) {
    std::map<HxStr, int>::iterator it = g_albumLevelStages.find(name);
    if (it == g_albumLevelStages.end()) {
        int nStage = QueryConfigValue(kLevelStageConfigCode,
                                      name.mStr != nullptr ? name.mStr : g_szEmptyString);
        it = g_albumLevelStages.insert(std::pair<const HxStr, int>(name, nStage)).first;
    }
    return it->second;
}

} // namespace

// 0x003f7a30
int IsAlbumLevel([[maybe_unused]] const HxStr &name) {
    return 1;
}

// 0x003f7a38
int GetAlbumLevelStage(const HxStr &name) {
    return LevelStageEntry(name);
}

// 0x003f7a58
int GetAlbumLevelValue(int nLevel, int nDifficulty) {
    int &value = g_albumLevelValues[nLevel * kDifficultyCount + nDifficulty];
    if (value == kNotCached) {
        value = QueryConfigValue(kLevelValueConfigCode, nLevel, nDifficulty);
    }
    return g_albumLevelValues[nLevel * kDifficultyCount + nDifficulty];
}

// 0x003f7ad8
int GetAlbumJukeboxValue() {
    if (g_nAlbumJukeboxValue == kNotCached) {
        g_nAlbumJukeboxValue = QueryConfigValue(kJukeboxConfigCode);
    }
    return g_nAlbumJukeboxValue;
}

// 0x003f79a8
void ClearAlbumCache() {
    g_albumLevelStages.clear();
    std::fill(g_albumLevelValues.begin(), g_albumLevelValues.end(), kNotCached);
    g_nAlbumJukeboxValue = kNotCached;
}
