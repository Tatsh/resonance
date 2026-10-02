#pragma once

#include "app/ticktask.h"
#include "gs/effector.h"
#include "mid/mbt.h"
#include "synth/source.h"

namespace Sch {
class TickClock;
} // namespace Sch

/**
 * Effect that sweeps a channel's filter controller with a triangle wave while it is applied.
 *
 * `11WahEffector` in the RTTI descriptor at `0x008f2a60`, with TickTask at offset 0 and Effector at
 * offset 0x20. Its tables are at `0x007de770` for TickTask and `0x007de738` for the Effector
 * subobject. The object is 0x48 bytes, which the factory's allocation measures.
 *
 * The task runs every 60 ticks and is not aligned. Each run samples mOscillator and sends
 * controller 0x4a with mDepth scaled by the sample. Switching the effect sends controller 0x51.
 *
 * The factory reads the depth from configuration code 0x390 and the oscillator period from code
 * 0x38f, both for the track.
 */
class WahEffector : public TickTask, public Effector {
public:
    /**
     * Build the effect with a triangle oscillator starting half way through its period.
     *
     * Inline. Effector::CreateForType() expands it, and the image also keeps this out-of-line copy.
     *
     * @param pClock The clock the task runs against.
     * @param nChannel The MIDI channel the controller changes go to.
     * @param nDepth The controller value at the top of the sweep.
     * @param nPeriod The oscillator period, in the ticks the task reports.
     * @ghidraAddress 0x001a1ed8
     */
    WahEffector(Sch::TickClock *pClock, unsigned char nChannel, int nDepth, int nPeriod)
        : TickTask(pClock, Mid::MBT(kPeriodTicks).mTick, 0), mChannel(nChannel), mDepth(nDepth),
          mOscillator(Source::AllocateTriSource(static_cast<float>(nPeriod), kStartPhase)),
          mEnabled(0), mPending(0) {
    }

    /**
     * Switch the effect off through this class's own Enable(), then release the oscillator.
     *
     * @ghidraAddress 0x001a2040
     */
    virtual ~WahEffector();

    /**
     * Report which effect this object applies.
     *
     * @return kEffectorTypeWah.
     * @ghidraAddress 0x001a2128
     */
    virtual int Type();

    /**
     * Send controller 0x51 with 0x7f to apply the effect or zero to release it.
     *
     * A request that repeats the current position sends nothing once a request has been sent.
     *
     * @param bEnabled Non-zero to apply the effect.
     * @ghidraAddress 0x001a08f8
     */
    virtual void Enable(int bEnabled);

    /**
     * Send controller 0x4a with mDepth times the oscillator's value at the elapsed count.
     *
     * @param nElapsedTicks Ticks since the task's epoch.
     * @return 1 always.
     * @ghidraAddress 0x001a0a10
     */
    virtual int Tick(int nElapsedTicks);

private:
    static constexpr int kPeriodTicks = 60;
    static constexpr float kStartPhase = 0.5f;

    unsigned char mChannel; // +0x34
    int mDepth;             // +0x38
    Source *mOscillator;    // +0x3c, a triangle wave
    int mEnabled;           // +0x40
    int mPending;           // +0x44, set once the first request has been sent
};
