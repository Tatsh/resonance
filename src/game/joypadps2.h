#pragma once

#include <vector>

#include "game/padrecord.h"

/**
 * Handle to one of the eight pad slots, two ports of four multitap slots each.
 *
 * The class is not polymorphic, emits no RTTI, and has no embedded file path. The eight-byte
 * object, the allocation InputPoller::Setup() makes, stores the slot index and the position mId.
 * Constructing a handle marks its slot in sSlotsInUse and destroying it clears the mark. Every
 * other member forwards to the PadRecord of the slot in sRecords.
 */
class JoypadPS2 {
public:
    /** Pad slots, two ports of four multitap slots each. */
    static constexpr int kSlotCount = 8;

    /**
     * Claim slot nIndex.
     *
     * @param nIndex The slot, from 0.
     * @ghidraAddress NTSC-U/C: 0x004ec2b8
     * @ghidraAddress PAL: 0x0052ae40
     */
    explicit JoypadPS2(int nIndex);

    /**
     * Release the slot.
     *
     * @ghidraAddress NTSC-U/C: 0x004ec458
     * @ghidraAddress PAL: 0x0052afe0
     */
    ~JoypadPS2();

    /**
     * Decode the slot's latest report through PadRecord::Read(), without the pressure outputs.
     *
     * InputPoller::ReadControllers() is the caller.
     *
     * @param pButtons Receives the button word, or null.
     * @param pAxis0 Receives the first analog byte, or null.
     * @param pAxis1 Receives the second analog byte, or null.
     * @param pAxis2 Receives the third analog byte, or null.
     * @param pAxis3 Receives the fourth analog byte, or null.
     * @return PadRecord::Read()'s result.
     * @ghidraAddress NTSC-U/C: 0x004ecaf8
     * @ghidraAddress PAL: 0x0052b680
     */
    int Poll(unsigned int *pButtons,
             unsigned char *pAxis0,
             unsigned char *pAxis1,
             unsigned char *pAxis2,
             unsigned char *pAxis3);

    /**
     * Restart the slot's setup state machine.
     *
     * Clears PadRecord::mPhase and PadRecord::mReadyLevel. InputPoller::ResetJoypads() is the
     * caller, expanded inline in InputPoller::FindJoypadConnections(). The title is inferred.
     *
     * @ghidraAddress NTSC-U/C: 0x004ecb30
     * @ghidraAddress PAL: 0x0052b6d8
     */
    void Reset();

    /**
     * Open the slot's pad through PadRecord::Open().
     *
     * InputPoller::Setup() is the caller. The title is inferred.
     *
     * @param nPort The port, from 0.
     * @param nSlot The multitap slot, from 0.
     * @param nDeadZone The analog dead zone, passed through.
     * @ghidraAddress NTSC-U/C: 0x004ecb60
     * @ghidraAddress PAL: 0x0052b708
     */
    void Open(int nPort, int nSlot, int nDeadZone);

    /**
     * Close the slot's pad through scePadPortClose().
     *
     * InputPoller::Shutdown() is the caller.
     *
     * @ghidraAddress NTSC-U/C: 0x004ecb90
     * @ghidraAddress PAL: 0x0052b738
     */
    void DeInitPadData();

    /**
     * Drive the slot's motors through PadRecord::SetVibration().
     *
     * InputPoller::SetVibration() is the caller. The title is inferred.
     *
     * @param nSmallMotor Positive to run the small motor.
     * @param nBigMotor The big motor's level.
     * @ghidraAddress NTSC-U/C: 0x004ecbc8
     * @ghidraAddress PAL: 0x0052b770
     */
    void SetVibration(int nSmallMotor, int nBigMotor);

    /**
     * Report whether a pad is present.
     *
     * InputPoller::NumberConnectedJoypads() is the caller. The title is inferred.
     *
     * @return 1 unless scePadGetState() reports the slot disconnected or closed, else 0.
     * @ghidraAddress NTSC-U/C: 0x004ecbf8
     * @ghidraAddress PAL: 0x0052b7a0
     */
    int IsConnected();

#ifdef VIDEO_STANDARD_PAL
    /**
     * Shut libpad down through PadRecord::EndLibrary().
     *
     * Only the European release has the routine, and it has no caller. The title is inferred.
     *
     * @ghidraAddress PAL: 0x0052b6b8
     */
    static void EndLibrary();
#endif

private:
    int mIndex; // +0x00

public:
    /**
     * Position of the handle among the InputPoller's JoypadPS2 objects, from 0.
     *
     * The constructor does not write it. InputPoller::Setup() assigns it after construction, and
     * InputPoller::ReadControllers() indexes the player table with it. Both accesses are direct.
     * +0x04
     */
    int mId;

private:
    // One bit per slot, set while a handle holds the slot. The unit's static initialiser (at
    // `0x004ec7d0`) builds it at 0x00704b40 with every bit clear.
    static std::vector<bool> sSlotsInUse;

    // One record per slot, at 0x00704bc0.
    static PadRecord sRecords[kSlotCount];
};
