#pragma once

#include <vector>

#include "game/padrecord.h"

/**
 * Handle to one of the eight pad slots, two ports of four multitap slots each.
 *
 * The class is not polymorphic, emits no RTTI, and has no embedded file path. The title is
 * inferred from the vocabulary of InputPollerPS2.cpp, whose one file-private class is
 * FindJoypadConnectionsCmd. The object is the four-byte slot index. Constructing a handle marks
 * its slot in sSlotsInUse and destroying it clears the mark. Every other member forwards to the
 * PadRecord of the slot in sRecords.
 */
class Joypad {
public:
    /** Pad slots, two ports of four multitap slots each. */
    static constexpr int kSlotCount = 8;

    /**
     * Claim slot nIndex.
     *
     * @param nIndex The slot, from 0.
     * @ghidraAddress 0x004ec2b8
     */
    explicit Joypad(int nIndex);

    /**
     * Release the slot.
     *
     * @ghidraAddress 0x004ec458
     */
    ~Joypad();

    /**
     * Decode the slot's latest report through PadRecord::Read(), without the pressure outputs.
     *
     * InputPoller's reading routine at `0x001dfab0` is the caller. The title is inferred.
     *
     * @param pButtons Receives the button word, or null.
     * @param pAxis0 Receives the first analog byte, or null.
     * @param pAxis1 Receives the second analog byte, or null.
     * @param pAxis2 Receives the third analog byte, or null.
     * @param pAxis3 Receives the fourth analog byte, or null.
     * @return PadRecord::Read()'s result.
     * @ghidraAddress 0x004ecaf8
     */
    int Read(unsigned int *pButtons,
             unsigned char *pAxis0,
             unsigned char *pAxis1,
             unsigned char *pAxis2,
             unsigned char *pAxis3);

    /**
     * Restart the slot's setup state machine.
     *
     * Clears PadRecord::mPhase and PadRecord::mUnknown124. FindJoypadConnectionsCmd and the
     * routine at `0x001e1a88` are the callers. The title is inferred.
     *
     * @ghidraAddress 0x004ecb30
     */
    void Reset();

    /**
     * Open the slot's pad through PadRecord::Open().
     *
     * InputPoller's setup routine at `0x001df248` is the caller. The title is inferred.
     *
     * @param nPort The port, from 0.
     * @param nSlot The multitap slot, from 0.
     * @param nUnknown128 Passed through.
     * @ghidraAddress 0x004ecb60
     */
    void Open(int nPort, int nSlot, int nUnknown128);

    /**
     * Close the slot's pad through scePadPortClose().
     *
     * The routine at `0x001df9d8` is the caller. The title is inferred.
     *
     * @ghidraAddress 0x004ecb90
     */
    void Close();

    /**
     * Drive the slot's motors through PadRecord::SetVibration().
     *
     * InputPoller::SetVibration() is the caller. The title is inferred.
     *
     * @param nSmallMotor Positive to run the small motor.
     * @param nBigMotor The big motor's level.
     * @ghidraAddress 0x004ecbc8
     */
    void SetVibration(int nSmallMotor, int nBigMotor);

    /**
     * Report whether a pad is present.
     *
     * The routine at `0x001e1ad8` is the caller. The title is inferred.
     *
     * @return 1 unless scePadGetState() reports the slot disconnected or closed, else 0.
     * @ghidraAddress 0x004ecbf8
     */
    int IsConnected();

private:
    int mIndex; // +0x00

    // One bit per slot, set while a handle holds the slot. The unit's static initialiser at
    // 0x004ec7d0 builds it at 0x00704b40 with every bit clear.
    static std::vector<bool> sSlotsInUse;

    // One record per slot, at 0x00704bc0.
    static PadRecord sRecords[kSlotCount];
};
