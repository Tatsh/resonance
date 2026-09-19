#pragma once

#include "game/phrasemgr.h"
#include "game/pitcher.h"
#include "game/quantizer.h"
#include "game/trackdata.h"
#include "msg/message.h"
#include "sch/tickclock.h"

/**
 * Pitch producer for a vocal track.
 *
 * `5Voxer` in the RTTI descriptor at `0x008f1d00`, with Pitcher as its only base at offset 0. The
 * object is 0x70 bytes, which VoxingSTG's tagged allocation measures. VoxingSTG builds exactly one
 * of these and branches on nothing to choose it.
 *
 * The class is not reconstructed. Only the surface VoxingSTG uses is declared, so that it compiles
 * against the real type.
 */
class Voxer : public Pitcher {
public:
    /**
     * @param pPhraseMgr The phrase manager for the track.
     * @param pQuantizer The quantiser for the track.
     * @param pClock The clock the producer schedules against.
     * @param pTrackData The track description.
     * @ghidraAddress 0x001d81b8
     */
    Voxer(PhraseMgr *pPhraseMgr,
          Quantizer *pQuantizer,
          Sch::TickClock *pClock,
          const TrackData *pTrackData);

    virtual ~Voxer();

    /**
     * Act on a message.
     *
     * @param pMsg The message.
     */
    virtual void HandleMessage(Message *pMsg);
};
