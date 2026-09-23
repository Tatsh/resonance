#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/jameffectsmgr.h"
#include "game/phrase.h"
#include "game/quantizer.h"
#include "game/trackdata.h"
#include "mid/mbt.h"
#include "msg/message.h"

class PhraseMgr;

/**
 * Player of the phrases one track is divided into.
 *
 * `12PhrasePlayer` in the RTTI descriptor at `0x008eff30`, over MsgSource at offset 0 and MsgSink
 * at offset 20. The primary table is at `0x007e3120` with four entries and the MsgSink subobject
 * table at `0x007e30f8`. MsgSource coming first is why every upcast to MsgSink in the stage classes
 * adjusts by `+0x14` and guards the adjustment against a null pointer. The object is 0x30 bytes,
 * which ScoreTrackGraph's constructor measures at `0x001cef40`.
 *
 * Each bar sends the bar's phrase to the sinks as one MultiMuseMsg. How the sequence is built
 * depends on the track mode: an axe or vocal track sends the phrase's own sequence, a riff or
 * scratch track the riff of every phrase gem, and a catch track the riff of every gem of the bar.
 *
 * The destructor at `0x001c2658` is implicitly declared. It restores the base tables, frees
 * MsgSource's vector, and releases the object under MsgSink's tag.
 */
class PhrasePlayer : public MsgSource, public MsgSink {
public:
    /**
     * @param pPhraseMgr The phrase manager of the track.
     * @param pQuantizer The quantiser of the track.
     * @param pTrackData The track description. The constructor copies its mode.
     * @ghidraAddress 0x001c17d8
     */
    PhrasePlayer(PhraseMgr *pPhraseMgr, Quantizer *pQuantizer, const TrackData *pTrackData);

    /**
     * Act on a message.
     *
     * Slot 3 of the MsgSink table. The routine asks the message for its identity and discards the
     * answer, which is what remains of a dispatch with no case left in the release build.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001c2928
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Play the phrase of one bar.
     *
     * PhraseMgr::OnCommand() calls it once a bar. The step value of the bar goes to mJamEffects
     * when there is one. A bar after mLastBar with a phrase is then played from its start.
     *
     * @param nBar The bar.
     * @ghidraAddress 0x001c1860
     */
    void PlayBar(int nBar);

    /**
     * Play a bar again from an offset, already some ticks under way.
     *
     * PhraseMgr::ReplayBar() is the recovered caller. A riff track plays the phrase gems and every
     * other mode plays the bar's gems, whatever the phrase of the bar.
     *
     * @param nBar The bar.
     * @param nOffset The offset within the bar, in MIDI ticks. Earlier gems are skipped.
     * @param nElapsed The ticks since the bar started, subtracted from every gem position.
     * @ghidraAddress 0x001c2818
     */
    void PlayBarAt(int nBar, int nOffset, int nElapsed);

    /**
     * Install the jam-effects manager the step values go to.
     *
     * PitchingSTG's and VoxingSTG's constructors call it. The title is inferred.
     *
     * @param pJamEffects The manager, or null.
     * @ghidraAddress 0x001c2810
     */
    void SetJamEffectsMgr(JamEffectsMgr *pJamEffects);

private:
    // Sends the riff of every gem of the bar from nFrom on, positioned nElapsed earlier, as one
    // sequence. A phrase without an owner plays nothing. Records the bar in mLastBar.
    // 0x001c1978
    void PlayBarGems(Phrase *pPhrase, int nBar, Mid::MBT from, Mid::MBT elapsed);

    // Sends the riff of every gem of the phrase from nFrom on as one sequence, transposing each by
    // the gem's transposition. A gem without a riff abandons the bar before anything is sent or
    // mLastBar changes.
    // 0x001c1ba8
    void PlayPhraseGems(Phrase *pPhrase, int nBar, Mid::MBT from, Mid::MBT elapsed);

    // Sends the phrase's own sequence when it has one, and records the bar in mLastBar.
    // 0x001c28a0
    void PlayPhraseMuse(Phrase *pPhrase, int nBar);

    PhraseMgr *mPhraseMgr;       // +0x18
    Quantizer *mQuantizer;       // +0x1c, not read by any recovered routine
    const TrackData *mTrackData; // +0x20
    int mTrackKind;              // +0x24, a TrackMode copied from TrackData::mKind
    JamEffectsMgr *mJamEffects;  // +0x28
    int mLastBar;                // +0x2c, the last bar played, -1 before the first
};
