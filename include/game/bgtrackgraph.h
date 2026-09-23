#pragma once

#include <cstddef>

class BarSequencer;
class MidiDisabler;
class Mixer;
class MsgSink;
class MsgSource;
class MuseSynth;
class TrackData;

/**
 * Audio graph of one background track.
 *
 * The class is not polymorphic and has no RTTI. The title comes from the literal `BGTrackGraph`
 * that RndWorld's draw pass allocates the object under at `0x0018cf5c` and that the destructor
 * frees it under. Its unit spans `0x0013fb10` through `0x00140578`, and RndWorld's draw pass
 * constructs one per background track.
 *
 * The track's MIDI runs from the sequencer through a MidiDisabler into a MuseSynth, whose players
 * feed a Mixer. Gamer switches the MidiDisabler bar by bar.
 */
class BGTrackGraph {
public:
    /**
     * Allocate a graph from the tagged heap under the tag "BGTrackGraph".
     *
     * No out-of-line body exists. GrooveWorld::BuildGraphs() at `0x0018cce8` inlines the call.
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     */
    void *operator new(size_t nSize);

    /**
     * Release a graph to the tagged heap.
     *
     * No out-of-line body exists. The destructor's release branch inlines the call.
     *
     * @param pBlock The block.
     */
    void operator delete(void *pBlock);

    /**
     * Build the synthesiser and the MIDI filter, and connect the filter to the synthesiser.
     *
     * The filter starts passing notes. The sequencer and the mixer are created later.
     *
     * @param nTrack The track's index.
     * @param nUnmapped The value the sequencer receives as BarSequencer's nUnmapped.
     * @ghidraAddress 0x0013fb10
     */
    BGTrackGraph(int nTrack, int nUnmapped);

    /**
     * Delete the sequencer, the filter, the synthesiser, and the mixer.
     *
     * @ghidraAddress 0x00140338
     */
    ~BGTrackGraph();

    /**
     * Build the track's sequencer and start it.
     *
     * Replays the controller state every bar of the track's MIDI leaves behind into the
     * synthesiser through a MidiChase, then creates a BarSequencer on the song clock that sends
     * through the filter and starts it. The title is inferred.
     *
     * @ghidraAddress 0x0013fc48
     */
    void BuildSequencer();

    /**
     * Record the track and build its mixer on the track's MIDI channel.
     *
     * @param pTrack The track.
     * @ghidraAddress 0x001403e0
     */
    void CreateMixer(TrackData *pTrack);

    /**
     * Register the mixer as a sink of a source.
     *
     * @param pSource The source.
     * @ghidraAddress 0x00140468
     */
    void AttachMixerToSource(MsgSource *pSource);

    /**
     * Feed the synthesiser's output to the mixer and the mixer's output to a synthesiser sink.
     *
     * @param pSynth The sink the mixer emits to.
     * @ghidraAddress 0x00140498
     */
    void AttachMixerToSynth(MsgSink *pSynth);

    /**
     * Register a sink with the synthesiser.
     *
     * @param pSink The sink.
     * @ghidraAddress 0x001404d8
     */
    void AddSynthSink(MsgSink *pSink);

    /**
     * Delete the sequencer, if any.
     *
     * @ghidraAddress 0x001404f8
     */
    void DeleteSequencer();

    /**
     * Let the track's notes reach the synthesiser.
     *
     * Calls MidiDisabler::Enable(). Gamer's per-bar update calls it when its background enable
     * manager reports the bar for the track, and DisableMidi() otherwise.
     *
     * @ghidraAddress 0x00140540
     */
    void EnableMidi();

    /**
     * Stop the track's notes reaching the synthesiser.
     *
     * Calls MidiDisabler::Disable(), which also silences the notes already sounding.
     *
     * @ghidraAddress 0x00140560
     */
    void DisableMidi();

    /**
     * Run BuildSequencer() and report zero.
     *
     * Inline. GrooveWorld::StartSequencers() reaches it through a pointer to member, and the
     * address is its uncalled out-of-line copy in the GrooveWorld unit.
     *
     * @return Always 0.
     * @ghidraAddress 0x00193ff8
     */
    int CallBuildSequencer();

    /**
     * Run DeleteSequencer() and report zero.
     *
     * Inline. GrooveWorld::StartSequencers() and FinishSong() reach it through a pointer to member,
     * and the address is its uncalled out-of-line copy in the GrooveWorld unit.
     *
     * @return Always 0.
     * @ghidraAddress 0x00194018
     */
    int CallDeleteSequencer();

    /**
     * Delete a graph, which may be null.
     *
     * Inline. GrooveWorld::DestroyGraphs() passes it to std::for_each, and the out-of-line copy
     * sits in the GrooveWorld unit.
     *
     * @param pGraph The graph.
     * @ghidraAddress 0x00194038
     */
    static void Delete(BGTrackGraph *pGraph);

private:
    BarSequencer *mSequencer; // +0x00
    unsigned char mUnknown04; // +0x04 0xff at construction
    unsigned char mUnknown05; // +0x05 0xff at construction
    int mTrack;               // +0x08
    TrackData *mTrackData;    // +0x0c
    int mUnknown10;           // +0x10 not written by the constructor
    int mUnmapped;            // +0x14
    MuseSynth *mMuseSynth;    // +0x18
    MidiDisabler *mDisabler;  // +0x1c
    Mixer *mMixer;            // +0x20
};

// 0x00193ff8
// The out-of-line copy.
inline int BGTrackGraph::CallBuildSequencer() {
    BuildSequencer();
    return 0;
}

// 0x00194018
// The out-of-line copy.
inline int BGTrackGraph::CallDeleteSequencer() {
    DeleteSequencer();
    return 0;
}

// 0x00194038
// The out-of-line copy.
inline void BGTrackGraph::Delete(BGTrackGraph *pGraph) {
    delete pGraph;
}
