#pragma once

#include "sch/genericsequencer.h"

class MsgSink;

/**
 * Dispatcher of one range of timed objects against a clock.
 *
 * The one instantiation in the image is `Sequencer<TickObj<MuseMsg *> const *>`, whose descriptor
 * is at `0x008eec48` with GenericSequencer as its one base at offset 0 and whose table is at
 * `0x007cc7e8`. MultiMusePlayer::Start() is the one construction, and it takes 0x2c bytes against
 * the tag at `0x007dfc90`.
 *
 * The two members below are what that construction writes above the base, at `+0x24` and `+0x28`.
 * MultiMusePlayer::PlayerFinished() tests whether the range is exhausted by comparing the base's
 * cursor against mFinish.
 *
 * The instantiation's table addresses a body inside the template's own address range at both
 * slot 1 and slot 2, so the template overrides both of GenericSequencer's virtuals. Neither body
 * is written.
 */
template <typename T>
class Sequencer : public GenericSequencer {
public:
    /**
     * @param begin First object of the range.
     * @param finish One past the last object of the range.
     */
    Sequencer(T begin, T finish);

    /**
     * Slot 1, verb unrecovered.
     *
     * The instantiation's table addresses a body of its own here rather than the base's, so the
     * template overrides the member. The body is not written.
     *
     * @ghidraAddress 0x00100e70
     */
    virtual void Slot1();

    /**
     * First object of the range.
     *
     * Public for the same reason as GenericSequencer::mCursor. +0x24
     */
    T mBegin;

    /**
     * One past the last object of the range.
     *
     * Public for the same reason as GenericSequencer::mCursor. +0x28
     */
    T mFinish;
};

/**
 * Post a sequencer against a clock, sending everything it dispatches to one sink.
 *
 * Writes the clock into the sequencer's `+0x0c` and registers it with that clock.
 * MultiMusePlayer::Start() is the one caller.
 *
 * The body is not reconstructed.
 *
 * @param pSequencer The sequencer to post.
 * @param pClock The clock to post it against.
 * @param pSink The sink everything it dispatches goes to.
 * @ghidraAddress 0x00100ef8
 */
void PostSequencer(GenericSequencer *pSequencer, void *pClock, MsgSink *pSink);
