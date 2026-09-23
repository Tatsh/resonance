#pragma once

#include "game/catcher.h"
#include "game/quantizer.h"
#include "game/trackdata.h"
#include "gs/phrasemgr.h"
#include "sch/tick.h"
#include "sch/tickclock.h"

/**
 * Catcher that scores a whole range of bars at once.
 *
 * `12MultiCatcher` in the RTTI descriptor at `0x008ef518`, with Catcher as its only base at offset
 * 0. Its primary table is at `0x007e0b28` with eleven entries and its MsgSource subobject table at
 * `0x007e0b00` with four. The table is the same length as the base's, so the class introduces no
 * virtual and overrides only the destructor and the base's two pure slots. The object is 0x80
 * bytes, the same as the base, so it adds no data member.
 *
 * The constructor's whole body is the base construction and the two table stores, and it supplies
 * 1 for the base's `int` parameter. The destructor's whole body is the base teardown and the
 * tagged release behind the deleting flag, which is why both definitions are empty of anything a
 * programmer wrote.
 *
 * CatchingSTG builds one of these for every game mode other than 1, and a SingleCatcher for 1.
 */
class MultiCatcher : public Catcher {
public:
    /**
     * @param pPhraseMgr The phrase manager for the track.
     * @param pQuantizer The quantiser for the track.
     * @param pTrackData The track description.
     * @param pClock The clock the scheduled commands run on.
     * @param tick The scheduler time the base retains.
     * @ghidraAddress 0x001b1a60
     */
    MultiCatcher(PhraseMgr *pPhraseMgr,
                 Quantizer *pQuantizer,
                 const TrackData *pTrackData,
                 Sch::TickClock *pClock,
                 Sch::Tick tick);

    /**
     * @ghidraAddress 0x001b0d98
     */
    virtual ~MultiCatcher();

    /**
     * Score a range of bars.
     *
     * Slot 9. The routine records the first argument, decrements the second into a bar count,
     * queries the track for the range, posts a caught-phrase packet for every bar in it, and then
     * sends two messages whose payloads include the range, the bar count, and the player. A third
     * argument of zero replaces the first step with a query on the player instead.
     *
     * The body is not written. It depends on seven routines in the 0x001d7xxx range that are not
     * identified, and on two message classes whose payload words their headers declare private.
     *
     * @param nFirst The first bar of the range.
     * @param nSecond One past the last bar of the range.
     * @param nThird Zero selects the player query rather than the track query.
     * @ghidraAddress 0x001ad8e8
     */
    virtual void Slot9(int nFirst, int nSecond, int nThird);

    /**
     * Slot 10. The body is empty. The base declares the slot pure, so the empty body is a real
     * override rather than an inherited default, and it is what makes this class concrete.
     *
     * @param nBar The caught bar, unread.
     * @ghidraAddress 0x001b0e00
     */
    virtual void Slot10(int nBar);
};
