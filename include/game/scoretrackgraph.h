#pragma once

#include "app/application.h"
#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/mixer.h"
#include "game/phraseplayer.h"
#include "game/quantizer.h"
#include "game/trackdata.h"
#include "gs/phrasemgr.h"
#include "synth/musesynth.h"

class PhraseDatabase;

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
 * The constructor at `0x001cee50` builds the four objects the members below store, each through
 * the tagged allocator under the tag `MsgSink`, and it takes the track description as its only
 * argument. Its body is not written here, because it is outside the assignment this class was
 * recovered under.
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
 * Every data member is protected. The four derived classes read seven of the ten directly and no
 * accessor for any of them exists in the image.
 */
class ScoreTrackGraph {
public:
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
     * Install a word in the mixer and attach the mixer to the synthesiser.
     *
     * Slot 6, pure. All four overrides are the same three instructions: they register the mixer
     * with the synthesiser through MuseSynth::AddMuseSink() and then store the argument in
     * Mixer::mUnknown04. No call site was inspected, so the four-byte argument's type is
     * unconfirmed.
     *
     * @param nValue The word to install.
     */
    virtual void Slot6(int nValue) = 0;

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
     * Install a word in the phrase manager.
     *
     * Slot 8, pure. All four overrides store the argument in PhraseMgr::mUnknown1c and ignore a
     * zero. No call site was inspected, so the four-byte argument's type is unconfirmed.
     *
     * @param nValue The word to install, ignored when zero.
     */
    virtual void Slot8(int nValue) = 0;

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
     * Slot 10. The default body is empty and does not write the return register, so its value is
     * indeterminate and the default is not meant to be called. CatchingSTG is the only class that
     * overrides it, and its override casts the catcher to SingleCatcher and forwards both
     * arguments to it.
     *
     * @param nFirst The first word, forwarded unchanged.
     * @param nSecond The second word, forwarded unchanged.
     * @return Whatever the override computes.
     * @ghidraAddress 0x001cf760
     */
    virtual int Slot10(int nFirst, int nSecond);

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

protected:
    int mUnknown00;              // +0x00, copied from TrackData::mUnknown04
    const TrackData *mTrackData; // +0x04, the constructor's argument
    PhraseMgr *mPhraseMgr;       // +0x08, 0x60 bytes, built at 0x001ba0d0
    PhrasePlayer *mPhrasePlayer; // +0x0c, 0x30 bytes, built at 0x001c17d8
    Quantizer *mQuantizer;       // +0x10, four bytes, built at 0x001ce670
    MuseSynth *mMuseSynth;       // +0x14, 0x30 bytes, built at 0x001aa4d8
    int mUnknown18;              // +0x18
    Mixer *mMixer;               // +0x1c, 0x58 bytes, built at 0x001a7110
    Application *mApplication;   // +0x20, from Application::shared()
    int mUnknown24;              // +0x24
};
