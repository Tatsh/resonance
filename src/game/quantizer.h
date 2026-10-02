#pragma once

#include "game/trackdata.h"

/**
 * Holder of the track description the beat-quantising paths consult.
 *
 * The class emits no RTTI, because it is not polymorphic. Its name is attested by the signature of
 * `Catcher::Catcher()` the image records, whose second parameter is `Quantizer *`.
 *
 * The object is four bytes. `ScoreTrackGraph` builds one with the global `operator new(4)`, and the
 * constructor at `0x001ce670` stores its argument and returns.
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

    /**
     * Round a song position to the quantisation of its bar.
     *
     * @param nTick The song position, in MIDI ticks.
     * @return The rounded position.
     * @ghidraAddress 0x001ce680
     */
    int Quantize(int nTick);

    /**
     * Report the quantisation of the bar a song position falls in.
     *
     * @param nTick The song position, in MIDI ticks.
     * @return TrackData::GetQuant() for the bar, in MIDI ticks.
     * @ghidraAddress 0x001ce6b0
     */
    int GetQuantum(int nTick);

    /**
     * Round a song position to the nearest multiple of a quantum.
     *
     * The position is moved eight ticks earlier before rounding. The arithmetic is unsigned, and a
     * zero quantum traps.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param nQuantum The quantum, in MIDI ticks.
     * @return The rounded position.
     * @ghidraAddress 0x001ce710
     */
    static unsigned Round(unsigned nTick, unsigned nQuantum);

private:
    const TrackData *mTrackData; // +0x00
};
