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
 * Every slot whose verb is unrecovered retains its table index as its title, because the index is
 * part of the layout. The comment records the behaviour recovered instead. Slot 2 and slot 3 are
 * the start and the stop of a stage, which the four overrides establish between them: each slot 2
 * override sends controller 0x52 with value 0x7f on the track's channel and each slot 3 override
 * sends the same controller with value zero, and every other pairing in the four classes is
 * symmetric in the same way.
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
     * Slot 2. The body is not written here, for the reason recorded in the class documentation.
     *
     * @ghidraAddress 0x001cf088
     */
    virtual void Slot2();

    /**
     * Stop the stage.
     *
     * Slot 3. The body is not written here, for the reason recorded in the class documentation.
     *
     * @ghidraAddress 0x001cf918
     */
    virtual void Slot3();

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
    virtual void Slot4(MsgSource *pPrimary, MsgSource *pOptional, MsgSource *pSecondary) = 0;

    /**
     * Register the mixer with one source.
     *
     * Slot 5. The default body is empty. CatchingSTG is the only class that overrides it, and its
     * override registers the mixer.
     *
     * @param pSource The source to register with.
     * @ghidraAddress 0x001cf750
     */
    virtual void Slot5(MsgSource *pSource);

    /**
     * Attach the mixer to the synthesiser and give it its output sink.
     *
     * Slot 6, pure. All four overrides are the same three instructions: they register the mixer
     * with the synthesiser through MuseSynth::AddSink() and then store the argument in
     * Mixer::mOutput, which types it as a sink.
     *
     * @param pOutput The sink the mixer sends to.
     */
    virtual void Slot6(MsgSink *pOutput) = 0;

    /**
     * Register one sink with every source the stage provides.
     *
     * Slot 7, pure. Each override registers the sink with the phrase manager and with each of its
     * own objects that is a source.
     *
     * @param pSink The sink to register.
     */
    virtual void Slot7(MsgSink *pSink) = 0;

    /**
     * Install the sink the phrase manager reports phrase changes to.
     *
     * Slot 8, pure. All four overrides store the argument in PhraseMgr::mNetSink and ignore a
     * null sink. PhraseMgr sends messages through that member's MsgSink::Handle(), which is what
     * types the argument.
     *
     * @param pSink The sink to install, ignored when null.
     */
    virtual void Slot8(MsgSink *pSink) = 0;

    /**
     * Report whether the stage has nothing outstanding.
     *
     * Slot 9. The default reports that it has not. CatchingSTG is the only class that overrides
     * it, and its override forwards to Catcher's slot 6, which reports whether the catcher's
     * counter at `+0x60` is zero.
     *
     * @return Non-zero when nothing is outstanding.
     * @ghidraAddress 0x001cf758
     */
    virtual int Slot9();

    /**
     * Give every phrase of the track to one player.
     *
     * Slot 10. The default body is empty. CatchingSTG is the only class that overrides it, and its
     * override forwards both arguments to SingleCatcher::ResetOwners(). The Gamer routine at
     * `0x001123b0` calls the slot for every track whose slot 11 reports non-zero, with the
     * winning player, and discards the return register.
     *
     * @param nTick The song position the caller received. No implementation reads it.
     * @param pPlayer The player the phrases go to.
     * @ghidraAddress 0x001cf760
     */
    virtual void Slot10(int nTick, Player *pPlayer);

    /**
     * Slot 11. The default returns zero and CatchingSTG returns 1, which is the whole of the
     * recovered behaviour on both sides.
     *
     * @return Zero here.
     * @ghidraAddress 0x001cf768
     */
    virtual int Slot11();

    /**
     * Slot 12. The default body is empty. CatchingSTG is the only class that overrides it, and its
     * override calls one routine on the phrase manager.
     *
     * @ghidraAddress 0x001cf778
     */
    virtual void Slot12();

    /**
     * Run Slot2() and report zero.
     *
     * Inline. GrooveWorld::StartSequencers() reaches it through a pointer to member, and the
     * address is its uncalled out-of-line copy.
     *
     * @return Always 0.
     * @ghidraAddress 0x001cf6b8
     */
    int CallSlot2();

    /**
     * Run Slot3() and report zero.
     *
     * Inline. GrooveWorld::FinishSong() reaches it through a pointer to member, and the address is
     * its uncalled out-of-line copy.
     *
     * @return Always 0.
     * @ghidraAddress 0x001cf6e8
     */
    int CallSlot3();

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
    int mUnknown00; // +0x00, copied from TrackData::mUnknown04

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
    int mUnknown18;              // +0x18
    Mixer *mMixer;               // +0x1c, 0x58 bytes, built at 0x001a7110
    Application *mApplication;   // +0x20, from Application::shared()
    int mUnknown24;              // +0x24
};

// 0x001cf6b8, the out-of-line copy.
inline int ScoreTrackGraph::CallSlot2() {
    Slot2();
    return 0;
}

// 0x001cf6e8, the out-of-line copy.
inline int ScoreTrackGraph::CallSlot3() {
    Slot3();
    return 0;
}

// 0x001cf718, the out-of-line copy.
inline void ScoreTrackGraph::Delete(ScoreTrackGraph *pGraph) {
    delete pGraph;
}
