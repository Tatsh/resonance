#include "game/solopowerbarmgr.h"

namespace {

// The arguments SoloPowerbarMgr fixes for GamePowerbarMgr.
constexpr int kRandomKind = 1;
constexpr int kUnknown30 = 0;
constexpr int kMinGap = 8;
constexpr int kMaxGap = 16;

} // namespace

// 0x001c6270
SoloPowerbarMgr::SoloPowerbarMgr(PlayMap *pMap,
                                 PhraseDatabase *pDatabase,
                                 const TrackData *pTrackData,
                                 int nTrack)
    : GamePowerbarMgr(
          pMap, pDatabase, pTrackData, kRandomKind, nTrack, kUnknown30, kMinGap, kMaxGap) {
}
