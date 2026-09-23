#pragma once

#include "app/ticktask.h"
#include "gs/effector.h"
#include "mid/mbt.h"
#include "synth/source.h"

namespace Sch {
class TickClock;
} // namespace Sch

/**
 * Effect that chops a channel's level with a square wave while it is applied.
 *
 * `15StutterEffector` in the RTTI descriptor at `0x008eef48`, with TickTask at offset 0 and
 * Effector at offset 0x20. Its tables are at `0x007de708` for TickTask and `0x007de6d0` for the
 * Effector subobject. The object is 0x48 bytes, which the factory's allocation measures.
 *
 * The task runs every half period, aligned. Each run samples mOscillator and sends controller 0x30
 * between 127 and mFloor. Applying the effect runs the task at once, sends controller 0x50 with
 * 0x7f, and starts the task. Releasing it stops the task, restores controller 0x30 to 0x7f, and
 * sends controller 0x50 with zero.
 *
 * The factory reads both tuning values from configuration code 0x391, which must hold exactly two
 * values.
 */
class StutterEffector : public TickTask, public Effector {
public:
    /**
     * Build the effect with a square oscillator.
     *
     * Inline. Effector::CreateForType() expands it, and the image also keeps this out-of-line copy.
     *
     * @param pClock The clock the task runs against.
     * @param nChannel The MIDI channel the controller changes go to.
     * @param nPeriod The oscillator period. The task runs every half period.
     * @param nFloor The level the chop falls to.
     * @ghidraAddress 0x001a21c8
     */
    StutterEffector(Sch::TickClock *pClock, unsigned char nChannel, int nPeriod, int nFloor)
        : TickTask(pClock, Mid::MBT(nPeriod / 2).mTick, 1), mEnabled(0), mPending(0),
          mChannel(nChannel), mFloor(nFloor),
          mOscillator(Source::AllocateSquareSource(static_cast<float>(nPeriod))) {
    }

    /**
     * Switch the effect off through this class's own Enable(), then release the oscillator.
     *
     * @ghidraAddress 0x001a2338
     */
    virtual ~StutterEffector();

    /**
     * Report which effect this object applies.
     *
     * @return kEffectorTypeStutter.
     * @ghidraAddress 0x001a2420
     */
    virtual int Type();

    /**
     * Apply or release the effect.
     *
     * A request that repeats the current position sends nothing once a request has been sent.
     *
     * @param bEnabled Non-zero to apply the effect.
     * @ghidraAddress 0x001a0ae0
     */
    virtual void Enable(int bEnabled);

    /**
     * Send controller 0x30 at the level the oscillator's value selects.
     *
     * The level is the value times 127 plus one minus the value times mFloor, computed in double
     * precision.
     *
     * @param nElapsedTicks Ticks since the task's epoch.
     * @return 1 always.
     * @ghidraAddress 0x001a0cc8
     */
    virtual int Tick(int nElapsedTicks);

private:
    int mEnabled;           // +0x34
    int mPending;           // +0x38, set once the first request has been sent
    unsigned char mChannel; // +0x3c
    int mFloor;             // +0x40
    Source *mOscillator;    // +0x44, a square wave
};
