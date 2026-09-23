#pragma once

#include "game/phrasemaker.h"
#include "game/quantizer.h"
#include "game/trackdata.h"
#include "gs/phrasemgr.h"
#include "msg/message.h"
#include "sch/tickclock.h"

/**
 * Phrase maker for a guitar track.
 *
 * `14AxePhraseMaker` in the RTTI descriptor at `0x008f2a50`, with PhraseMaker as its only base at
 * offset 0. Its primary table is at `0x007ddc50` with six entries and its MsgSource subobject table
 * at `0x007ddc28` with four, so the class introduces two virtuals of its own. The object is 0x50
 * bytes, which AxingSTG's tagged allocation measures.
 *
 * An earlier pass titled this class's constructor `RndSpotShadowMap__Construct`. No descriptor
 * among the 574 in the image bears that title. Slot 0 of the table at `0x007ddc50` addresses the
 * accessor at `0x0019d390`, which guards on the descriptor at `0x008f2a50`, and that is what
 * settles the name.
 *
 * The class is not reconstructed. Only the surface AxingSTG uses is declared, so that it compiles
 * against the real type.
 */
class AxePhraseMaker : public PhraseMaker {
public:
    /**
     * @param pPhraseMgr The phrase manager for the track.
     * @param pQuantizer The quantiser for the track.
     * @param pTrackData The track description.
     * @param pClock The clock the maker schedules against.
     * @ghidraAddress 0x0019b5d0
     */
    AxePhraseMaker(PhraseMgr *pPhraseMgr,
                   Quantizer *pQuantizer,
                   const TrackData *pTrackData,
                   Sch::TickClock *pClock);

    /**
     * @ghidraAddress 0x0019b7f0
     */
    virtual ~AxePhraseMaker();

    /**
     * Act on a message.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x0019c408
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Act on a period that has elapsed. Slot 4.
     *
     * The body is not written. It runs the routine at `0x0019c118` when the period follows the one
     * recorded at `+0x2c`, runs `0x0019c368` with the period, and then, when `+0x48` is set and
     * the track description's test at `0x001d79d0` accepts the period, dispatches slot 11 of the
     * synthesiser Globals::GetSynth() reports with the MIDI channel at `+0x24`.
     *
     * @param nPeriod The index of the period.
     * @ghidraAddress 0x0019d990
     */
    virtual void Slot4(int nPeriod);

    /**
     * Report the song position periods are counted from. Slot 5.
     *
     * @return 6 always, after a discarded finiteness test on the same value.
     * @ghidraAddress 0x0019d438
     */
    virtual int Slot5();
};
