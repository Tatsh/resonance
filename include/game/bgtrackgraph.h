#pragma once

/**
 * Audio graph of one background track.
 *
 * The class is not polymorphic and has no RTTI. The title comes from the literal `BGTrackGraph`
 * at `0x007d3a80` that its destructor at `0x00140338` frees the object under. Its unit spans
 * `0x0013fb10` through `0x00140578`, and RndWorld's draw pass constructs one per background track.
 *
 * The constructor at `0x0013fb10` builds a MidiDisabler at `+0x1c`, which filters the track's MIDI
 * on its way to the synthesiser. Only the two members Gamer calls are declared here. Their bodies
 * forward to MidiDisabler, which has no header yet, and the rest of the layout waits for the unit.
 */
class BGTrackGraph {
public:
    /**
     * Stop the track's MIDI reaching the synthesiser.
     *
     * The body calls the MidiDisabler routine at `0x001a6ef8`, which sets its disabled word.
     * Gamer's per-bar update calls it when its second enable manager reports the bar for the
     * track, and EnableMidi() otherwise.
     *
     * @ghidraAddress 0x00140540
     */
    void DisableMidi();

    /**
     * Let the track's MIDI reach the synthesiser again.
     *
     * The body calls the MidiDisabler routine at `0x001a6a58`, which clears its disabled word and
     * sends an AllNotesOffMsg at kMBTInfinity.
     *
     * @ghidraAddress 0x00140560
     */
    void EnableMidi();
};
