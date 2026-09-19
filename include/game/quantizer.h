#pragma once

#include "game/trackdata.h"

/**
 * Holder of the track description the beat-quantising paths consult.
 *
 * The class emits no RTTI, because it is not polymorphic. Its title is attested by the mangled
 * signature of `Catcher::Catcher()`,
 * `__7CatcherP9PhraseMgrP9QuantizerPC9TrackDataPQ23Sch9TickClockiGQ23Sch4Tick`, where `P9Quantizer`
 * is the second parameter.
 *
 * The object is four bytes. `ScoreTrackGraph` builds one with `MemAllocScalar(4)` and the
 * constructor at `0x001ce670` stores its argument and returns, which is the whole of the class's
 * recovered state.
 */
class Quantizer {
public:
    /**
     * Retain the track description.
     *
     * @param pTrackData The track this quantiser works over.
     * @ghidraAddress 0x001ce670
     */
    Quantizer(const TrackData *pTrackData);

private:
    const TrackData *mTrackData; // +0x00
};
