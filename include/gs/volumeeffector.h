#pragma once

#include "gs/effector.h"

/**
 * Effect that sets a channel's volume controller while it is applied.
 *
 * `14VolumeEffector` in the RTTI descriptor at `0x008ef6c0`, with Effector as its one base. The
 * object is 0x20 bytes, which the factory's allocation at `0x001a0e64` measures. Its table is at
 * `0x007de810`.
 * The factory reads the controller value from configuration code 0x392.
 */
class VolumeEffector : public Effector {
public:
    /**
     * @param nChannel The MIDI channel the controller change goes to.
     * @param nValue The controller value while the effect is applied.
     * @ghidraAddress 0x001a1a10
     */
    VolumeEffector(unsigned char nChannel, int nValue);

    /**
     * Switch the effect off, then tear down the base.
     *
     * @ghidraAddress 0x001a1a68
     */
    virtual ~VolumeEffector();

    /**
     * Report which effect this object applies.
     *
     * @return kEffectorTypeVolume.
     * @ghidraAddress 0x001a1b30
     */
    virtual int Type();

    /**
     * Send controller 0x2f with mValue while applied and with 0x7f otherwise.
     *
     * A request that repeats the current position sends nothing.
     *
     * @param bEnabled Non-zero to apply the effect.
     * @ghidraAddress 0x001a07c0
     */
    virtual void Enable(int bEnabled);

private:
    unsigned char mChannel; // +0x14
    int mValue;             // +0x18
    int mEnabled;           // +0x1c
};
