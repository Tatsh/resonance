#pragma once

#include <vector>

#include "memcard/memcardconnectstate.h"
#include "os/hxstr.h"

/**
 * One level in a stage's list.
 *
 * RebuildStageLists() builds one per level name, with mOrder from configuration code 0x25e. The
 * record is 12 bytes, the stride every routine over the lists uses. The name is a placeholder,
 * taken from the program's titles for the sort routines that order the lists.
 */
struct StageListEntry {
    HxStr mName; /*!< The level's name. +0x00 */
    int mOrder;  /*!< The position the list is sorted by. +0x08 */
};

/**
 * Order two stage entries by mOrder.
 *
 * Inline. The sort routines at `0x003cec08` and `0x003ced70` compare the two words at `+0x08` with
 * `slt` and call nothing.
 *
 * @param left The first entry.
 * @param right The second entry.
 * @return Whether the first entry sorts first.
 */
inline bool operator<(const StageListEntry &left, const StageListEntry &right) {
    return left.mOrder < right.mOrder;
}

/**
 * One arena in the solo or the local arena table.
 *
 * RebuildArenaLists() builds one per arena name, with mOrder from configuration code 0x25f.
 * MetArenasScreen assigns mName to GameParams::mArenaName, which is what identifies the table. The
 * layout is the same as StageListEntry's, and the type is distinct because the unit includes a
 * second, separate emission of every sort routine for it. The name is a placeholder.
 */
struct ArenaListEntry {
    HxStr mName; /*!< The arena's name. +0x00 */
    int mOrder;  /*!< The position the table is sorted by. +0x08 */
};

/**
 * Order two arena entries by mOrder.
 *
 * Inline, for the same reason as the StageListEntry overload.
 *
 * @param left The first entry.
 * @param right The second entry.
 * @return Whether the first entry sorts first.
 */
inline bool operator<(const ArenaListEntry &left, const ArenaListEntry &right) {
    return left.mOrder < right.mOrder;
}

/**
 * Report the display name of a difficulty.
 *
 * The routine reads configuration code 0x258 with the key `ms_easy`, `ms_normal`, or `ms_expert`
 * for difficulties 0, 1, and 2, and reports an empty string for any other value. The front end's
 * stage and statistics screens call it. Every routine in this unit is a free function over
 * file-scope data, and the unit's name is inferred.
 *
 * @param nDifficulty The difficulty.
 * @return The name.
 * @ghidraAddress 0x003cc498
 */
HxStr DifficultyName(int nDifficulty);

/**
 * Rebuild the per-stage level lists from the configuration.
 *
 * The routine reads the level names under configuration code 0x276 into GetLevelNames(), empties
 * every stage list, appends each level to the list of the stage configuration code 0x25d reports
 * for it, and sorts each list by mOrder. The MetRenderer and MetNullRenderer constructors and
 * MetExpansionPakScreen call it.
 *
 * @ghidraAddress 0x003cc7a0
 */
void RebuildStageLists();

/**
 * Rebuild the solo and local arena tables from the configuration.
 *
 * The routine reads the names under configuration code 0x27a for `solo` and then for `local`,
 * rebuilds each table with mOrder from configuration code 0x25f, and sorts both. The MetRenderer
 * constructor and MetExpansionPakScreen call it.
 *
 * @ghidraAddress 0x003ccab0
 */
void RebuildArenaLists();

/**
 * Report the level names RebuildStageLists() last read.
 *
 * The list is a function-local static of a file-local routine, which this accessor forwards to.
 * CampaignStats rebuilds and merges its level records from it.
 *
 * @return The names.
 * @ghidraAddress 0x003d06b8
 */
std::vector<HxStr> &GetLevelNames();

/**
 * Report one stage's level list.
 *
 * @param nStage The stage, counted from 1.
 * @return The list.
 * @ghidraAddress 0x003d06d8
 */
std::vector<StageListEntry> *GetStageList(int nStage);

/**
 * Report the arena table for the current game mode.
 *
 * The local table in kGameModeLocal and the solo table otherwise. MetArenasScreen, the tutorial,
 * the solo-stages, and the remix-load screens read it.
 *
 * @return The table.
 * @ghidraAddress 0x003d06f8
 */
std::vector<ArenaListEntry> *GetArenaList();

/**
 * Report the memory-card location that follows another.
 *
 * Port 1 without a multitap is followed by port 2, port 2 by port 1, slot 1-A by slot 1-B, and
 * slot 1-B by slot 1-A. Any other location gives the default MemcardConnectState. The memory-card
 * screens call it to cycle the selected card.
 *
 * @param slot The current location.
 * @return The next location.
 * @ghidraAddress 0x003d0a40
 */
MemcardConnectState NextCardSlot(const MemcardConnectState &slot);

/**
 * Report the name of the first memory-card location found, or `1` when there is none.
 *
 * The body reads GlobalSettings::mCardSlots and copies the slot name of its first entry. The front
 * end's character, remix, and Freq-maker screens call it.
 *
 * @return The name.
 * @ghidraAddress 0x003d0b98
 */
HxStr FirstCardSlotName();
