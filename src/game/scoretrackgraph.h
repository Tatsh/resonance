#pragma once

#include "app/application.h"
#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/phraseplayer.h"
#include "game/quantizer.h"
#include "game/trackdata.h"
#include "gs/mixer.h"
#include "gs/musesynth.h"
#include "gs/phrasemgr.h"

class BarSequencer;
class PhraseDatabase;
class Player;

/**
 * Shared base of the four per-instrument gameplay stages.
 *
 * `15ScoreTrackGraph` in the RTTI descriptor at `0x0086f640`, built through the built-in descriptor
 * constructor with no base list. Four classes derive from it and the RTTI records each with this
 * class at offset 0: AxingSTG at `0x008efe70`, CatchingSTG at `0x00902230`, PitchingSTG at
 * `0x00902240`, and VoxingSTG at `0x008efe80`. The shared base is recovered from those four
 * descriptors and from the table walk rather than inferred from the names.
 *
 * Its table is at `0x007e5148` and runs thirteen entries to the zero terminator, and all four
 * derived tables run thirteen as well, so no subclass introduces a virtual of its own. Slots 4, 6,
 * 7, and 8 address the shared pure-virtual stub at `0x005381a8`, which makes this class abstract.
 * Slots 5, 9, 10, 11, and 12 are inert defaults with real bodies here.
 *
 * The object is 0x2c bytes with the vptr at `+0x28`, which is where this toolchain places it for a
 * class with no base. Derived members therefore start at `+0x2c`.
 *
 * The constructor builds the five objects the members below store. The phrase manager, phrase
 * player, mixer, and synthesiser go through the tagged allocator under the tag `MsgSink`, and the
 * quantiser through the plain allocator.
 *
 * An earlier pass titled the five default bodies for AxingSTG, which owns none of them. The table
 * diff against each derived table is what corrects the attribution: `0x001cf750`, `0x001cf758`,
 * `0x001cf760`, `0x001cf768`, and `0x001cf778` sit in this class's own table at slots 5, 9, 10,
 * 11, and 12, and AxingSTG, PitchingSTG, and VoxingSTG inherit all five.
 *
 * Each slot's comment records its table index. Start() and Stop() (slots 2 and 3) are the start
 * and the stop of a stage, as the four overrides establish between them. Each Start() override
 * sends controller 0x52 with value 0x7f on the track's channel and each Stop() override sends the
 * same controller with value zero, and every other pairing in the four classes is symmetric in the
 * same way.
 *
 * Every data member apart from mTrackData is protected. The four derived classes read seven of
 * the ten directly and no accessor for any of them exists in the image.
 */
class ScoreTrackGraph {
public:
    /**
     * Build the stage's quantiser, phrase manager, phrase player, mixer, and synthesiser.
     *
     * The phrase manager takes the song clock, a bar of 1920 ticks, the play map, and
     * configuration code 0x2be. The phrase player is then installed in the phrase manager.
     *
     * @param pTrackData The track description.
     * @ghidraAddress 0x001cee50
     */
    explicit ScoreTrackGraph(TrackData *pTrackData);

    /**
     * @ghidraAddress 0x001cf840
     */
    virtual ~ScoreTrackGraph();

    /**
     * Report the phrase database of this stage's phrase manager.
     *
     * GrooveWorld's phrase save and load paths call it for every stage.
     *
     * @return The phrase manager's database.
     * @ghidraAddress 0x001cf978
     */
    PhraseDatabase *GetPhraseDatabase();

    /**
     * Start the stage.
     *
     * Slot 2. Starts the phrase manager's commands, chases the MIDI of every bar of the play map
     * into the synthesiser so controllers and programs are current, and then builds and starts a
     * BarSequencer at the song start that plays the track into the synthesiser.
     *
     * @ghidraAddress 0x001cf088
     */
    virtual void Start();

    /**
     * Stop the stage.
     *
     * Slot 3. Withdraws the phrase manager's commands and deletes the sequencer Start() built.
     *
     * @ghidraAddress 0x001cf918
     */
    virtual void Stop();

    /**
     * Wire the stage's objects to the sources that drive it.
     *
     * Slot 4, pure. Each override registers the stage's own objects with the first and third
     * sources, and with the second only when it is supplied. The second source is the only
     * argument any override guards against a null pointer.
     *
     * @param pPrimary The source every override registers with first.
     * @param pOptional A further source, ignored when null.
     * @param pSecondary The source every override registers the phrase manager with.
     */
    virtual void
    ConnectSources(MsgSource *pPrimary, MsgSource *pOptional, MsgSource *pSecondary) = 0;

    /**
     * Register the mixer with one source.
     *
     * Slot 5. The default body is empty. CatchingSTG is the only class that overrides it, and its
     * override registers the mixer.
     *
     * @param pSource The source to register with.
     * @ghidraAddress 0x001cf750
     */
    virtual void AddMixerToSource(MsgSource *pSource);

    /**
     * Attach the mixer to the synthesiser and give it its output sink.
     *
     * Slot 6, pure. All four overrides are the same three instructions: they register the mixer
     * with the synthesiser through MuseSynth::AddSink() and then store the argument in
     * Mixer::mOutput, which types it as a sink.
     *
     * @param pOutput The sink the mixer sends to.
     */
    virtual void SetMixerOutput(MsgSink *pOutput) = 0;

    /**
     * Register one sink with every source the stage provides.
     *
     * Slot 7, pure. Each override registers the sink with the phrase manager and with each of its
     * own objects that is a source.
     *
     * @param pSink The sink to register.
     */
    virtual void AddSinkToSources(MsgSink *pSink) = 0;

    /**
     * Install the sink the phrase manager reports phrase changes to.
     *
     * Slot 8, pure. All four overrides store the argument in PhraseMgr::mNetSink and ignore a
     * null sink. PhraseMgr sends messages through that member's MsgSink::Handle(), which is what
     * types the argument.
     *
     * @param pSink The sink to install, ignored when null.
     */
    virtual void SetNetSink(MsgSink *pSink) = 0;

    /**
     * Report whether the stage has nothing outstanding.
     *
     * Slot 9. The default reports that nothing is outstanding. CatchingSTG is the only class that
     * overrides it, and its override forwards to Catcher::IsPhraseRunEmpty(). That routine
     * reports whether the catcher's counter at `+0x60` is zero. Gamer ends the game for a player
     * out of juice only once every stage reports non-zero.
     *
     * @return Non-zero when nothing is outstanding.
     * @ghidraAddress 0x001cf758
     */
    virtual int HasNothingPending();

    /**
     * Give every phrase of the track to one player.
     *
     * Slot 10. The default body is empty. CatchingSTG is the only class that overrides it, and its
     * override forwards both arguments to SingleCatcher::ResetOwners(). The Gamer routine at
     * `0x001123b0` calls the slot for every track whose CanGivePhrases() reports non-zero, with
     * the winning player, and discards the return register.
     *
     * @param nTick The song position the caller received. No implementation reads it.
     * @param pPlayer The player the phrases go to.
     * @ghidraAddress 0x001cf760
     */
    virtual void GivePhrases(int nTick, Player *pPlayer);

    /**
     * Report whether GivePhrases() acts on this stage.
     *
     * Slot 11. The default returns zero and CatchingSTG, the one class whose GivePhrases() has a
     * body, returns 1. Gamer tests it before each GivePhrases() call.
     *
     * @return Zero here.
     * @ghidraAddress 0x001cf768
     */
    virtual int CanGivePhrases();

    /**
     * Create the phrase manager's powerbar manager.
     *
     * Slot 12. The default body is empty. CatchingSTG is the only class that overrides it, and its
     * override runs PhraseMgr::CreatePowerbarMgr(). GrooveWorld::BuildGraphs() calls it for each
     * catch track after loading saved phrases.
     *
     * @ghidraAddress 0x001cf778
     */
    virtual void CreatePowerbarMgr();

    /**
     * Run Start() and report zero.
     *
     * Inline. GrooveWorld::StartSequencers() reaches it through a pointer to member, and the
     * address is its uncalled out-of-line copy.
     *
     * @return Always 0.
     * @ghidraAddress 0x001cf6b8
     */
    int CallStart();

    /**
     * Run Stop() and report zero.
     *
     * Inline. GrooveWorld::FinishSong() reaches it through a pointer to member, and the address is
     * its uncalled out-of-line copy.
     *
     * @return Always 0.
     * @ghidraAddress 0x001cf6e8
     */
    int CallStop();

    /**
     * Delete a stage, which may be null.
     *
     * Inline. GrooveWorld::DestroyGraphs() passes it to std::for_each.
     *
     * @param pGraph The stage.
     * @ghidraAddress 0x001cf718
     */
    static void Delete(ScoreTrackGraph *pGraph);

protected:
    int mTrack; // +0x00, the track's index, copied from TrackData::mIndex

public:
    /**
     * The track description, the constructor's argument. +0x04
     *
     * Public because GrooveWorld::BuildGraphs() loads it directly at `0x0018d730` and passes it
     * to TrackData::AddPhrases(), and the image has no accessor.
     */
    TrackData *mTrackData;

protected:
    PhraseMgr *mPhraseMgr;       // +0x08, 0x60 bytes, built at 0x001ba0d0
    PhrasePlayer *mPhrasePlayer; // +0x0c, 0x30 bytes, built at 0x001c17d8
    Quantizer *mQuantizer;       // +0x10, four bytes, built at 0x001ce670
    MuseSynth *mMuseSynth;       // +0x14, 0x30 bytes, built at 0x001aa4d8
    int mUnused;                 // +0x18, cleared by the constructor and never read
    Mixer *mMixer;               // +0x1c, 0x58 bytes, built at 0x001a7110
    Application *mApplication;   // +0x20, from Application::shared()
    BarSequencer *mSequencer;    // +0x24, built by Start() and deleted by Stop()
};

// 0x001cf6b8
// The out-of-line copy.
inline int ScoreTrackGraph::CallStart() {
    Start();
    return 0;
}

// 0x001cf6e8
// The out-of-line copy.
inline int ScoreTrackGraph::CallStop() {
    Stop();
    return 0;
}

// 0x001cf718
// The out-of-line copy.
inline void ScoreTrackGraph::Delete(ScoreTrackGraph *pGraph) {
    delete pGraph;
}
