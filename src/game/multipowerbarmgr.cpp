#include "game/multipowerbarmgr.h"

namespace {

// The arguments MultiPowerbarMgr fixes for GamePowerbarMgr.
constexpr int kRandomKind = 0;
constexpr int kMultiplayer = 1;
constexpr int kMinGap = 6;
constexpr int kMaxGap = 12;

} // namespace

// NTSC-U/C: 0x001c62c0, PAL: 0x001cc108
MultiPowerbarMgr::MultiPowerbarMgr(PlayMap *pMap,
                                   PhraseDatabase *pDatabase,
                                   const TrackData *pTrackData,
                                   int nTrack)
    : GamePowerbarMgr(
          pMap, pDatabase, pTrackData, kRandomKind, nTrack, kMultiplayer, kMinGap, kMaxGap) {
}
