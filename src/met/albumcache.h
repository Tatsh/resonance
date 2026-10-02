#pragma once

#include "os/hxstr.h"

/**
 * Report whether a level counts toward the campaign album.
 *
 * The body returns 1 whatever the name. CampaignStats calls it for every level name while it
 * rebuilds and resets its level records, and skips a level on 0, a branch the shipped body never
 * takes. Every routine in this unit is a free function over file-scope caches, and the unit's name
 * is inferred from the script function `clear_album_cache` that empties them. The title is
 * inferred.
 *
 * @param name The level's name, which the body does not read.
 * @return 1.
 * @ghidraAddress 0x003f7a30
 */
int IsAlbumLevel(const HxStr &name);

/**
 * Report the stage a level belongs to, reading it once and caching it.
 *
 * The first request for a name reads configuration code 0x25d for it and stores the result in a
 * file-scope `std::map<HxStr, int>`; later requests read the map. CampaignStats is the caller. The
 * title is inferred.
 *
 * @param name The level's name.
 * @return The stage.
 * @ghidraAddress 0x003f7a38
 */
int GetAlbumLevelStage(const HxStr &name);

/**
 * Report one value of configuration code 0x27d for a level and difficulty, cached.
 *
 * The cache is a file-scope vector of 30 words, three difficulties for each of ten levels, filled
 * with -2 until an entry is first read. CampaignStats is the caller. The title is inferred.
 *
 * @param nLevel The level, counted from 0.
 * @param nDifficulty The difficulty, 0 through 2.
 * @return The value.
 * @ghidraAddress 0x003f7a58
 */
int GetAlbumLevelValue(int nLevel, int nDifficulty);

/**
 * Report the value of configuration code 0x514, cached.
 *
 * MetJukeboxBaseScreen is the caller. The title is inferred.
 *
 * @return The value.
 * @ghidraAddress 0x003f7ad8
 */
int GetAlbumJukeboxValue();

/**
 * Empty all three caches.
 *
 * The map of level stages is emptied, every word of the level-value cache is reset to -2, and the
 * jukebox value is reset to -2. The image lists no direct caller. The script function
 * `clear_album_cache` at `0x003f6860`, which the file-scope ScriptFunc at `0x00892370` registers,
 * repeats the same three steps inline.
 *
 * @ghidraAddress 0x003f79a8
 */
void ClearAlbumCache();
