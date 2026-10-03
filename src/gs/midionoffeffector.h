#pragma once

#include "gs/effector.h"

/**
 * Effect that switches one MIDI controller fully on while it is applied.
 *
 * Its RTTI descriptor is at `0x008efe20`. It has Effector as its one base. The object is 0x24 bytes
 * and its table is at `0x007de7d8`. The factory builds one for each of the three EffectorType
 * values from kEffectorTypeMidiOnOffFirst on, with controllers 0x52, 0x53, and 0x51, and the object
 * reports the type it was built for.
 */
class MidiOnOffEffector : public Effector {
public:
    /**
     * @param nChannel The MIDI channel the controller change goes to.
     * @param nType The EffectorType Type() reports.
     * @param nController The controller number.
     * @ghidraAddress NTSC-U/C: 0x001a1bc0
     * @ghidraAddress PAL: 0x001a7928
     */
    MidiOnOffEffector(unsigned char nChannel, int nType, unsigned char nController);

    /**
     * Switch the effect off, then tear down the base.
     *
     * @ghidraAddress NTSC-U/C: 0x001a1c28
     * @ghidraAddress PAL: 0x001a7990
     */
    virtual ~MidiOnOffEffector();

    /**
     * Report which effect this object applies.
     *
     * @return The type the constructor received.
     * @ghidraAddress NTSC-U/C: 0x001a1cf0
     * @ghidraAddress PAL: 0x001a7a58
     */
    virtual int Type();

    /**
     * Send the controller with 0x7f while applied and with zero otherwise.
     *
     * A request that repeats the current position sends nothing.
     *
     * @param bEnabled Non-zero to apply the effect.
     * @ghidraAddress NTSC-U/C: 0x001a0860
     * @ghidraAddress PAL: 0x001a65c8
     */
    virtual void SetEnabled(int bEnabled);

private:
    int mType;                 // +0x14
    unsigned char mChannel;    // +0x18
    int mEnabled;              // +0x1c
    unsigned char mController; // +0x20
};
