#include "met/albumcache.h"

#include <algorithm>
#include <map>
#include <vector>

#include "os/hxstr.h"
#include "script/configquery.h"

namespace {

// Configuration codes the caches are filled from.
constexpr int kLevelStageConfigCode = 605;
constexpr int kLevelValueConfigCode = 637;
constexpr int kJukeboxConfigCode = 0x514;

// A cache word that has not been read yet.
constexpr int kNotCached = -2;

constexpr int kDifficultyCount = 3;
constexpr int kAlbumLevelCount = 10;

// NTSC-U/C: 0x006db590, PAL: 0x0071ed70
std::map<HxStr, int> g_albumLevelStages;

// NTSC-U/C: 0x006db5a0, PAL: 0x0071ed80
std::vector<int> g_albumLevelValues(kAlbumLevelCount *kDifficultyCount, kNotCached);

// NTSC-U/C: 0x006db5ac, PAL: 0x0071ed8c
int g_nAlbumJukeboxValue = kNotCached;

// NTSC-U/C: 0x003f6760, PAL: 0x0042ef48
// The one caller is GetAlbumLevelStage().
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

// NTSC-U/C: 0x003f7a30, PAL: 0x00430240
int IsAlbumLevel([[maybe_unused]] const HxStr &name) {
    return 1;
}

// NTSC-U/C: 0x003f7a38, PAL: 0x00430248
int GetAlbumLevelStage(const HxStr &name) {
    return LevelStageEntry(name);
}

// NTSC-U/C: 0x003f7a58, PAL: 0x00430268
int GetAlbumLevelValue(int nLevel, int nDifficulty) {
    int &value = g_albumLevelValues[nLevel * kDifficultyCount + nDifficulty];
    if (value == kNotCached) {
        value = QueryConfigValue(kLevelValueConfigCode, nLevel, nDifficulty);
    }
    return g_albumLevelValues[nLevel * kDifficultyCount + nDifficulty];
}

// NTSC-U/C: 0x003f7ad8, PAL: 0x004302e8
int GetAlbumJukeboxValue() {
    if (g_nAlbumJukeboxValue == kNotCached) {
        g_nAlbumJukeboxValue = QueryConfigValue(kJukeboxConfigCode);
    }
    return g_nAlbumJukeboxValue;
}

// NTSC-U/C: 0x003f79a8, PAL: 0x004301b8
void ClearAlbumCache() {
    g_albumLevelStages.clear();
    std::fill(g_albumLevelValues.begin(), g_albumLevelValues.end(), kNotCached);
    g_nAlbumJukeboxValue = kNotCached;
}
