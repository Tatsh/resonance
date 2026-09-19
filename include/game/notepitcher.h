#pragma once

#include "game/phrasemgr.h"
#include "game/pitcher.h"
#include "game/quantizer.h"
#include "game/trackdata.h"
#include "msg/message.h"
#include "sch/tickclock.h"

/**
 * Pitch producer for a track whose gems are discrete notes.
 *
 * `11NotePitcher` in the RTTI descriptor at `0x008f1ce0`, with Pitcher as its only base at offset
 * 0. The object is 0x70 bytes, which PitchingSTG's tagged allocation measures. PitchingSTG builds
 * one of these when the track's kind word is 2, and a Scratcher when it is 3.
 *
 * The constructor takes seven arguments, arriving in a1 through a3 and t0 through t3, which is the
 * register convention this target uses for arguments five through eight.
 *
 * The class is not reconstructed. Only the surface PitchingSTG uses is declared, so that it
 * compiles against the real type.
 */
class NotePitcher : public Pitcher {
public:
    /**
     * @param pPhraseMgr The phrase manager for the track.
     * @param pQuantizer The quantiser for the track.
     * @param pClock The clock the producer schedules against.
     * @param pTrackData The track description.
     * @param bPlayModeOne Non-zero when the game manager reports play mode 1.
     * @param nUnknown1 PitchingSTG passes 1.
     * @param nUnknown2 PitchingSTG passes zero.
     * @ghidraAddress 0x001b1ce0
     */
    NotePitcher(PhraseMgr *pPhraseMgr,
                Quantizer *pQuantizer,
                Sch::TickClock *pClock,
                const TrackData *pTrackData,
                int bPlayModeOne,
                int nUnknown1,
                int nUnknown2);

    virtual ~NotePitcher();

    /**
     * Act on a message.
     *
     * @param pMsg The message.
     */
    virtual void HandleMessage(Message *pMsg);
};
