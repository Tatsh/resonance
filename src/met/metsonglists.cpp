#include "met/metsonglists.h"

#include <algorithm>
#include <vector>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "os/hxstr.h"
#include "script/configquery.h"

namespace {

// Configuration codes the lists are read under.
constexpr int kStringConfigCode = 0x258;
constexpr int kLevelStageConfigCode = 0x25d;
constexpr int kLevelOrderConfigCode = 0x25e;
constexpr int kArenaOrderConfigCode = 0x25f;
constexpr int kLevelListConfigCode = 0x276;
constexpr int kArenaListConfigCode = 0x27a;

constexpr int kDifficultyEasy = 0;
constexpr int kDifficultyNormal = 1;
constexpr int kDifficultyExpert = 2;

// The static initialiser at 0x003d00f0 builds one empty list per stage.
constexpr int kStageCount = 6;

// mPortSlot values of the four memory-card locations.
constexpr int kCardPort1 = 0;
constexpr int kCardPort1SlotB = 1;
constexpr int kCardPort2 = 0x100;

// 0x006ce610
std::vector<std::vector<StageListEntry>> g_stageLists(kStageCount);

// 0x006ce620
std::vector<ArenaListEntry> g_localArenaList;

// 0x006ce630
std::vector<ArenaListEntry> g_soloArenaList;

// 0x003cc748
// The static's guard is at 0x006ce608 and the list at 0x00892360.
std::vector<HxStr> &LevelNameStorage() {
    static std::vector<HxStr> names;
    return names;
}

} // namespace

// 0x003cc498
HxStr DifficultyName(int nDifficulty) {
    HxStr name("");
    switch (nDifficulty) {
    case kDifficultyEasy: {
        HxStr text;
        QueryConfigString(&text, kStringConfigCode, "ms_easy");
        name = text;
        break;
    }
    case kDifficultyNormal: {
        HxStr text;
        QueryConfigString(&text, kStringConfigCode, "ms_normal");
        name = text;
        break;
    }
    case kDifficultyExpert: {
        HxStr text;
        QueryConfigString(&text, kStringConfigCode, "ms_expert");
        name = text;
        break;
    }
    default:
        name = ""; // Yes, the binary assigns the empty string a second time.
        break;
    }
    return name;
}

// 0x003cc7a0
void RebuildStageLists() {
    QueryConfigStrings(&LevelNameStorage(), kLevelListConfigCode);

    for (std::vector<std::vector<StageListEntry>>::size_type i = 0; i < g_stageLists.size(); ++i) {
        g_stageLists[i].clear();
    }

    // The end is fetched through a second call rather than reused from the first.
    for (std::vector<HxStr>::iterator it = GetLevelNames().begin(), end = GetLevelNames().end();
         it != end;
         ++it) {
        int nStage = QueryConfigValue(kLevelStageConfigCode,
                                      it->mStr != nullptr ? it->mStr : g_szEmptyString);
        StageListEntry entry;
        entry.mName = *it;
        entry.mOrder = QueryConfigValue(kLevelOrderConfigCode,
                                        it->mStr != nullptr ? it->mStr : g_szEmptyString);
        g_stageLists[nStage - 1].push_back(entry);
    }

    for (std::vector<std::vector<StageListEntry>>::size_type i = 0; i < g_stageLists.size(); ++i) {
        std::sort(g_stageLists[i].begin(), g_stageLists[i].end());
    }
}

// 0x003ccab0
void RebuildArenaLists() {
    std::vector<HxStr> soloNames;
    std::vector<HxStr> localNames;
    ArenaListEntry entry;

    g_soloArenaList.clear();
    QueryConfigStrings(&soloNames, kArenaListConfigCode, "solo");
    for (const HxStr &name : soloNames) {
        entry.mName = name;
        entry.mOrder = QueryConfigValue(kArenaOrderConfigCode,
                                        name.mStr != nullptr ? name.mStr : g_szEmptyString);
        g_soloArenaList.push_back(entry);
    }

    g_localArenaList.clear();
    QueryConfigStrings(&localNames, kArenaListConfigCode, "local");
    for (const HxStr &name : localNames) {
        entry.mName = name;
        entry.mOrder = QueryConfigValue(kArenaOrderConfigCode,
                                        name.mStr != nullptr ? name.mStr : g_szEmptyString);
        g_localArenaList.push_back(entry);
    }

    std::sort(g_soloArenaList.begin(), g_soloArenaList.end());
    std::sort(g_localArenaList.begin(), g_localArenaList.end());
}

// 0x003d06b8
std::vector<HxStr> &GetLevelNames() {
    return LevelNameStorage();
}

// 0x003d06d8
std::vector<StageListEntry> *GetStageList(int nStage) {
    return &g_stageLists[nStage - 1];
}

// 0x003d06f8
std::vector<ArenaListEntry> *GetArenaList() {
    if (Application::shared()->GetGameMode() == kGameModeLocal) {
        return &g_localArenaList;
    }
    return &g_soloArenaList;
}

// 0x003d0a40
CardSlot NextCardSlot(const CardSlot &slot) {
    CardSlot next;
    if (slot.mPortSlot == kCardPort1) {
        if (slot.mName == "1") {
            next.mPortSlot = kCardPort2;
            next.mName = "2";
        } else {
            next.mPortSlot = kCardPort1SlotB;
            next.mName = "1-B";
        }
    } else if (slot.mPortSlot == kCardPort2) {
        next.mPortSlot = kCardPort1;
        next.mName = "1";
    } else if (slot.mPortSlot == kCardPort1SlotB) {
        next.mPortSlot = kCardPort1;
        next.mName = "1-A";
    }
    return next;
}
