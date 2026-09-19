#pragma once

#include "game/phrasemgr.h"
#include "game/pitcher.h"
#include "game/quantizer.h"
#include "game/trackdata.h"
#include "msg/message.h"
#include "sch/tickclock.h"

/**
 * Pitch producer for a turntable track.
 *
 * `9Scratcher` in the RTTI descriptor at `0x008f1cf0`, with Pitcher as its only base at offset 0.
 * The object is 0x90 bytes, which PitchingSTG's tagged allocation measures. PitchingSTG builds one
 * of these when the track's kind word is 3, and a NotePitcher when it is 2.
 *
 * The class is not reconstructed. Only the surface PitchingSTG uses is declared, so that it
 * compiles against the real type.
 */
class Scratcher : public Pitcher {
public:
    /**
     * @param pPhraseMgr The phrase manager for the track.
     * @param pQuantizer The quantiser for the track.
     * @param pClock The clock the producer schedules against.
     * @param pTrackData The track description.
     * @ghidraAddress 0x001cf988
     */
    Scratcher(PhraseMgr *pPhraseMgr,
              Quantizer *pQuantizer,
              Sch::TickClock *pClock,
              const TrackData *pTrackData);

    virtual ~Scratcher();

    /**
     * Act on a message.
     *
     * @param pMsg The message.
     */
    virtual void HandleMessage(Message *pMsg);
};
