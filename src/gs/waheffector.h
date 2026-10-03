#pragma once

#include "app/ticktask.h"
#include "gs/effector.h"
#include "mid/tick.h"
#include "synth/source.h"

namespace Sch {
class TickClock;
} // namespace Sch

/**
 * Effect that sweeps a channel's filter controller with a triangle wave while it is applied.
 *
 * Its RTTI descriptor is at `0x008f2a60`. It has TickTask at offset 0 and Effector at offset 0x20.
 * Its tables are at `0x007de770` for TickTask and `0x007de738` for the Effector subobject. The
 * factory's allocation measures the object at 0x48 bytes.
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
     * @ghidraAddress NTSC-U/C: 0x001a1ed8
     * @ghidraAddress PAL: 0x001a7c40
     */
    WahEffector(Sch::TickClock *pClock, unsigned char nChannel, int nDepth, int nPeriod)
        : TickTask(pClock, Sch::Tick(kPeriodTicks).mTick, 0), mChannel(nChannel), mDepth(nDepth),
          mOscillator(Source::AllocateTriSource(static_cast<float>(nPeriod), kStartPhase)),
          mEnabled(0), mPending(0) {
    }

    /**
     * Switch the effect off through this class's own SetEnabled(), then release the oscillator.
     *
     * @ghidraAddress NTSC-U/C: 0x001a2040
     * @ghidraAddress PAL: 0x001a7da8
     */
    virtual ~WahEffector();

    /**
     * Report which effect this object applies.
     *
     * @return kEffectorTypeWah.
     * @ghidraAddress NTSC-U/C: 0x001a2128
     * @ghidraAddress PAL: 0x001a7e90
     */
    virtual int Type();

    /**
     * Send controller 0x51 with 0x7f to apply the effect or zero to release it.
     *
     * A request that repeats the current position sends nothing once a request has been sent.
     *
     * @param bEnabled Non-zero to apply the effect.
     * @ghidraAddress NTSC-U/C: 0x001a08f8
     * @ghidraAddress PAL: 0x001a6660
     */
    virtual void SetEnabled(int bEnabled);

    /**
     * Send controller 0x4a with mDepth times the oscillator's value at the elapsed count.
     *
     * @param nElapsedTicks Ticks since the task's epoch.
     * @return 1 always.
     * @ghidraAddress NTSC-U/C: 0x001a0a10
     * @ghidraAddress PAL: 0x001a6778
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
