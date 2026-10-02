#pragma once

#include "game/gamepowerbarmgr.h"

/**
 * Powerbar source for multiplayer, dealing the powerbar the weighted table selects.
 *
 * `16MultiPowerbarMgr` in the RTTI, with GamePowerbarMgr as its one base and no members of its
 * own. Its table is at `0x007e39a8`. PhraseMgr creates one in every game mode other than 1.
 *
 * The deleting destructor at `0x001c6440` is implicitly declared.
 */
class MultiPowerbarMgr : public GamePowerbarMgr {
public:
    /**
     * Deal powerbars from the weighted table, 6 to 11 bars apart.
     *
     * @param pMap The play map.
     * @param pDatabase The phrase database of the track.
     * @param pTrackData The track description.
     * @param nTrack The track's index.
     * @ghidraAddress 0x001c62c0
     */
    MultiPowerbarMgr(PlayMap *pMap,
                     PhraseDatabase *pDatabase,
                     const TrackData *pTrackData,
                     int nTrack);
};
