#pragma once

#include "gs/multimuse.h"
#include "gs/museplayer.h"
#include "gs/musesynth.h"
#include "mid/tickobj.h"
#include "sch/sequencer.h"

/**
 * Player that turns one MultiMuse into messages over time.
 *
 * `15MultiMusePlayer` in the RTTI descriptor at `0x00901c50`, with MuseSynth at offset 0 and
 * MusePlayer at offset 48. The object is 0x50 bytes, which the allocation in
 * MuseSynth::StartMultiMusePlayer() measures. It has three tables: the primary at `0x007dfd58`,
 * the MuseParent one at `0x007dfd38`, and the MusePlayer one at `0x007dfd08`. The slot indices
 * restart in each, so both MuseParent slot 2 and MusePlayer slot 2 are members of this class and
 * the two share a title only in the program.
 *
 * It is both a player and a MuseSynth. Start() posts a scheduler command over its sequence, and
 * each message that command dispatches arrives back through the MuseSynth half, which creates a
 * player for it. That is why the class inherits MuseSynth::HandleMessage() unchanged rather than
 * overriding it.
 *
 * Both MuseParent overrides forward once up the chain. RetainOnly() reports the request to its own
 * parent the first time and never again, and PlayerFinished() reports itself finished to its own
 * parent once the schedule is exhausted and the last child player is gone.
 */
class MultiMusePlayer : public MuseSynth, public MusePlayer {
public:
    // Both bases declare an allocation pair. The destructor's release branch bills the block to
    // the tagged heap, which is MsgSink's operator rather than MusePlayer's untagged one.
    using MsgSink::operator delete;
    using MsgSink::operator new;

    /**
     * @param pMuse The sequence to play, retained for the lifetime of the player.
     * @param pParent The owner this player reports back to.
     * @param pClock The clock the schedule is posted against.
     * @ghidraAddress 0x001a9fb8
     */
    MultiMusePlayer(MultiMuse *pMuse, MuseParent *pParent, Sch::TickClock *pClock);

    /**
     * @ghidraAddress 0x001aa100
     */
    virtual ~MultiMusePlayer();

    /**
     * @ghidraAddress 0x001a9b48
     */
    virtual void Start(MsgSink *pSink);

    /**
     * @ghidraAddress 0x001aa1b8
     */
    virtual void Stop();

    /**
     * MusePlayer table slot 4. The body is one instruction and returns 1.
     *
     * @return 1.
     * @ghidraAddress 0x001a9ed0
     */
    virtual int Slot4();

    /**
     * @ghidraAddress 0x001aa1f8
     */
    virtual void RetainOnly(MusePlayer *pPlayer);

    /**
     * @ghidraAddress 0x001a9c30
     */
    virtual void PlayerFinished(MusePlayer *pPlayer);

private:
    MultiMuse *mMuse;    // +0x38 retained on construction and released by the destructor
    MuseParent *mParent; // +0x3c
    // The clock the schedule is posted against, the same value MuseSynth stores.
    Sch::TickClock *mClock; // +0x40
    // The sequencer that walks mMuse, created by Start() and withdrawn by Stop(). The constructor
    // does not initialise it.
    Sequencer<const TickObj<MuseMsg *> *> *mSequencer; // +0x44
    // Whether RetainOnly() has already been reported to mParent. It is set once and never cleared.
    int mParentToldToRetain; // +0x48
    // Whether the player is running. Start() sets it, Stop() and PlayerFinished() clear it.
    int mRunning; // +0x4c
};
