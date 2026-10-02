#pragma once

#include "game/gamepowerbarmgr.h"

/**
 * Powerbar source for a single player, dealing one of two powerbars at even odds.
 *
 * `15SoloPowerbarMgr` in the RTTI, with GamePowerbarMgr as its one base and no members of its own.
 * Its table is at `0x007e3978`. PhraseMgr creates one in game mode 1.
 *
 * The deleting destructor at `0x001c6518` is implicitly declared.
 */
class SoloPowerbarMgr : public GamePowerbarMgr {
public:
    /**
     * Deal powerbars at random kinds, 8 to 15 bars apart.
     *
     * @param pMap The play map.
     * @param pDatabase The phrase database of the track.
     * @param pTrackData The track description.
     * @param nTrack The track's index.
     * @ghidraAddress 0x001c6270
     */
    SoloPowerbarMgr(PlayMap *pMap,
                    PhraseDatabase *pDatabase,
                    const TrackData *pTrackData,
                    int nTrack);
};
