#pragma once

#include "gs/effector.h"

/**
 * Effect that sets a channel's volume controller while it is applied.
 *
 * Its RTTI descriptor is at `0x008ef6c0`. It has Effector as its one base. The factory's
 * allocation at `0x001a0e64` measures the object at 0x20 bytes. Its table is at `0x007de810`. The
 * factory reads the controller value from configuration code 0x392.
 */
class VolumeEffector : public Effector {
public:
    /**
     * @param nChannel The MIDI channel the controller change goes to.
     * @param nAppliedLevel The controller value while the effect is applied.
     * @ghidraAddress NTSC-U/C: 0x001a1a10
     * @ghidraAddress PAL: 0x001a7778
     */
    VolumeEffector(unsigned char nChannel, int nAppliedLevel);

    /**
     * Switch the effect off, then tear down the base.
     *
     * @ghidraAddress NTSC-U/C: 0x001a1a68
     * @ghidraAddress PAL: 0x001a77d0
     */
    virtual ~VolumeEffector();

    /**
     * Report which effect this object applies.
     *
     * @return kEffectorTypeVolume.
     * @ghidraAddress NTSC-U/C: 0x001a1b30
     * @ghidraAddress PAL: 0x001a7898
     */
    virtual int Type();

    /**
     * Send controller 0x2f with mAppliedLevel while applied and with 0x7f otherwise.
     *
     * A request that repeats the current position sends nothing.
     *
     * @param bEnabled Non-zero to apply the effect.
     * @ghidraAddress NTSC-U/C: 0x001a07c0
     * @ghidraAddress PAL: 0x001a6528
     */
    virtual void SetEnabled(int bEnabled);

private:
    unsigned char mChannel; // +0x14
    int mAppliedLevel;      // +0x18
    int mEnabled;           // +0x1c
};
