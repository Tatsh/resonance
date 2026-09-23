#include "game/multipowerbarmgr.h"

namespace {

// The arguments MultiPowerbarMgr fixes for GamePowerbarMgr.
constexpr int kRandomKind = 0;
constexpr int kUnknown30 = 1;
constexpr int kMinGap = 6;
constexpr int kMaxGap = 12;

} // namespace

// 0x001c62c0
MultiPowerbarMgr::MultiPowerbarMgr(PlayMap *pMap,
                                   PhraseDatabase *pDatabase,
                                   const TrackData *pTrackData,
                                   int nTrack)
    : GamePowerbarMgr(
          pMap, pDatabase, pTrackData, kRandomKind, nTrack, kUnknown30, kMinGap, kMaxGap) {
}
