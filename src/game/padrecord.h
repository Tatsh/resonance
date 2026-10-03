#pragma once

#include <libpad.h>

/**
 * One controller's pad-library state: the DMA area libpad fills and the decoder's working state.
 *
 * The class is not polymorphic, emits no RTTI, and has no embedded file path, so the title is
 * inferred. JoypadPS2 has a table of eight at `0x00704bc0`, one per port and multitap slot, and
 * every JoypadPS2 member forwards to the record its index selects. The record is 0x180 bytes. The
 * first 0x100 bytes are the area scePadPortOpen() receives, which is why the record starts on a
 * 64-byte boundary.
 *
 * The routines sit between the RndArena and Python units, and none of them includes a string or a
 * structure that matches a Sony sample or ps2sdk. They are therefore treated as game code. The
 * free routines below operate on a record.
 */
class PadRecord {
public:
    /** Bytes of the actuator buffers scePadSetActDirect() and scePadSetActAlign() receive. */
    static constexpr int kActuatorByteCount = 6;

    /** Bytes of the pressure baseline BreugPadRead() subtracts from each pressure reading. */
    static constexpr int kPressureByteCount = 12;

    /** Frames of the DMA area scePadPortOpen() receives. */
    static constexpr int kDmaFrameCount = 2;

    /** Entries of mLastAxes. BreugPadInit() clears them one byte at a time. */
    static constexpr int kLastAxisCount = 4;

    /** Entries of mAxisDeltas. BreugPadInit() clears them one halfword at a time. */
    static constexpr int kAxisDeltaCount = 4;

#ifdef VIDEO_STANDARD_PAL
    /**
     * Shut libpad down through scePadEnd().
     *
     * Only the European release has the routine, and it has no caller. The first-open flag of
     * BreugPadInit() retains its value. The title is inferred.
     *
     * @ghidraAddress PAL: 0x00585f20
     */
    static void EndLibrary();
#endif

    /** The area libpad writes reports into. +0x000 */
    alignas(64) scePadDmaFrame mDmaArea[kDmaFrameCount];

    /** The decoded button word BreugPadRead() reports. +0x100 */
    unsigned int mButtons;

    // Cleared by BreugPadInit(), and written by BreugPadRead() and never read. +0x104 to +0x10c
    unsigned int mHeldButtonsSeen; // every button held since BreugPadInit()
    unsigned int mToggledButtons;  // each new press toggles its bit
    unsigned int mPreviousButtons; // mButtons before the latest report

    /** The port BreugPadInit() received. +0x110 */
    int mPort;

    /** The multitap slot BreugPadInit() received. +0x114 */
    int mSlot;

    short mRawButtons; // +0x118, the latest report's button word, cleared by BreugPadInit()

    /**
     * Index into BreugPadRead()'s setup state machine. BreugPadInit() and JoypadPS2::Reset() clear
     * it. +0x11c
     */
    int mPhase;

    int mReportMode; // +0x120, the latest report's mode byte, cleared by BreugPadInit()

    /**
     * Setup progress BreugPadRead() records. BreugPadSetMotors() acts only from 2, and
     * JoypadPS2::Reset() clears it. +0x124
     */
    int mReadyLevel;

    int mDeadZone;                           // +0x128, BreugPadInit()'s fourth argument
    int mReadCount;                          // +0x12c, counted by BreugPadRead()
    unsigned char mLastAxes[kLastAxisCount]; // +0x130, written by BreugPadRead()
    short mAxisDeltas[kAxisDeltaCount];      // +0x134, written by BreugPadRead()

    /** The pressure baseline BreugPadRead() subtracts. +0x13c */
    unsigned char mPressureBaseline[kPressureByteCount];

    /** The actuator levels BreugPadSetMotors() sends. +0x148 */
    unsigned char mActDirect[kActuatorByteCount];

    /** The actuator alignment BreugPadRead() sends during setup. +0x14e */
    unsigned char mActAlign[kActuatorByteCount];
};

/**
 * Reset a record and open the pad at nPort and nSlot.
 *
 * Clears the actuator levels and the pressure baseline, aligns actuator 0 to byte 0 and actuator 1
 * to byte 1 with the rest unused, starts libpad through scePadInit(0) the first time any record
 * opens, and opens the port with mDmaArea.
 *
 * @param pPad The record.
 * @param nPort The port, from 0.
 * @param nSlot The multitap slot, from 0.
 * @param nDeadZone Stored in mDeadZone.
 * @ghidraAddress NTSC-U/C: 0x005bcf58
 * @ghidraAddress PAL: 0x00585f40
 */
void BreugPadInit(PadRecord *pPad, int nPort, int nSlot, int nDeadZone);

/**
 * Advance a pad's setup state machine and decode the latest report.
 *
 * mPhase indexes a 78-entry jump table at `0x00834430` that walks the pad through analog mode,
 * actuator alignment, and pressure-sensitive mode, and mReadyLevel records how far it has got.
 * Each non-null output receives one value of the decoded report, the analog values centred on zero
 * and zeroed inside the mDeadZone dead zone. A pad that is not stable reports only mButtons and
 * zero axes.
 *
 * @param pPad The record.
 * @param pButtons Receives the button word at mButtons, or null.
 * @param pAxis0 Receives the first analog byte, or null.
 * @param pAxis1 Receives the second analog byte, or null.
 * @param pAxis2 Receives the third analog byte, or null.
 * @param pAxis3 Receives the fourth analog byte, or null.
 * @param pPressures Receives the pressure bytes, or null.
 * @param pPressureDeltas Receives each pressure less its baseline, or null.
 * @return mReadyLevel, from 0 (no report) to 3 (pressure-sensitive), or 0 when the pad is not
 *         stable or the read fails.
 * @ghidraAddress NTSC-U/C: 0x005bc998
 * @ghidraAddress PAL: 0x00585960
 */
int BreugPadRead(PadRecord *pPad,
                 unsigned int *pButtons,
                 unsigned char *pAxis0,
                 unsigned char *pAxis1,
                 unsigned char *pAxis2,
                 unsigned char *pAxis3,
                 unsigned char *pPressures,
                 short *pPressureDeltas);

/**
 * Drive a pad's two vibration motors.
 *
 * Does nothing until mReadyLevel arrives at 2. Otherwise the small motor runs whenever nSmallMotor
 * is positive, and the big motor takes nBigMotor as its level.
 *
 * @param pPad The record.
 * @param nSmallMotor Positive to run the small motor.
 * @param nBigMotor The big motor's level.
 * @ghidraAddress NTSC-U/C: 0x005bd0b0
 * @ghidraAddress PAL: 0x00586098
 */
void BreugPadSetMotors(PadRecord *pPad, int nSmallMotor, int nBigMotor);
