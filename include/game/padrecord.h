#pragma once

/**
 * One controller's pad-library state: the DMA area libpad fills and the decoder's working state.
 *
 * The class is not polymorphic, emits no RTTI, and has no embedded file path, so the title is
 * inferred. Joypad owns a table of eight at `0x00704bc0`, one per port and multitap slot, and
 * every Joypad member forwards to the record its index selects. The record is 0x180 bytes. The
 * first 0x100 bytes are the area scePadPortOpen() receives, which is why the record starts on a
 * 64-byte boundary.
 *
 * The routines sit between the RndArena and Python units, and none of them carries a string or a
 * structure that matches a Sony sample or ps2sdk, so they are treated as game code.
 */
class PadRecord {
public:
    /** Bytes of the actuator buffers scePadSetActDirect() and scePadSetActAlign() receive. */
    static constexpr int kActuatorByteCount = 6;

    /** Bytes of the pressure baseline Read() subtracts from each pressure reading. */
    static constexpr int kPressureByteCount = 12;

    /** Bytes of the DMA area scePadPortOpen() receives. */
    static constexpr int kDmaAreaSize = 0x100;

    /** Entries of mUnknown130, which Open() clears one byte at a time. */
    static constexpr int kUnknownByteCount = 4;

    /** Entries of mUnknown134, which Open() clears one halfword at a time. */
    static constexpr int kUnknownShortCount = 4;

    /**
     * Reset the record and open the pad at nPort and nSlot.
     *
     * Clears the actuator levels and the pressure baseline, aligns actuator 0 to byte 0 and
     * actuator 1 to byte 1 with the rest unused, starts libpad through scePadInit(0) the first time
     * any record opens, and opens the port with mDmaArea. The title is inferred.
     *
     * @param nPort The port, from 0.
     * @param nSlot The multitap slot, from 0.
     * @param nUnknown128 Stored in mUnknown128.
     * @ghidraAddress 0x005bcf58
     */
    void Open(int nPort, int nSlot, int nUnknown128);

    /**
     * Advance the pad's setup state machine and decode the latest report.
     *
     * mPhase indexes a 78-entry jump table at `0x00834430` that walks the pad through analog
     * mode, actuator alignment, and pressure-sensitive mode, and mUnknown124 records how far it
     * has got. Each non-null output receives one value of the decoded report, the analog values
     * centred on zero and zeroed inside the mUnknown128 dead zone. A pad that is not stable
     * reports only mButtons and zero axes. The title is inferred.
     *
     * @param pButtons Receives the button word at mButtons, or null.
     * @param pAxis0 Receives the first analog byte, or null.
     * @param pAxis1 Receives the second analog byte, or null.
     * @param pAxis2 Receives the third analog byte, or null.
     * @param pAxis3 Receives the fourth analog byte, or null.
     * @param pPressures Receives the pressure bytes, or null.
     * @param pPressureDeltas Receives each pressure less its baseline, or null.
     * @return mUnknown124, from 0 (no report) to 3 (pressure-sensitive), or 0 when the pad is not
     *         stable or the read fails.
     * @ghidraAddress 0x005bc998
     */
    int Read(unsigned int *pButtons,
             unsigned char *pAxis0,
             unsigned char *pAxis1,
             unsigned char *pAxis2,
             unsigned char *pAxis3,
             unsigned char *pPressures,
             short *pPressureDeltas);

    /**
     * Drive the two vibration motors.
     *
     * Does nothing until mUnknown124 reaches 2. Otherwise the small motor runs whenever nSmallMotor
     * is positive, and the big motor takes nBigMotor as its level. The title is inferred.
     *
     * @param nSmallMotor Positive to run the small motor.
     * @param nBigMotor The big motor's level.
     * @ghidraAddress 0x005bd0b0
     */
    void SetVibration(int nSmallMotor, int nBigMotor);

    /** The area libpad writes reports into. +0x000 */
    alignas(64) unsigned char mDmaArea[kDmaAreaSize];

    /** The decoded button word Read() reports. +0x100 */
    unsigned int mButtons;

    // Cleared by Open(). Read() ORs new bits into mUnknown104. +0x104 to +0x10c
    unsigned int mUnknown104;
    unsigned int mUnknown108;
    unsigned int mUnknown10c;

    /** The port Open() received. +0x110 */
    int mPort;

    /** The multitap slot Open() received. +0x114 */
    int mSlot;

    short mUnknown118; // +0x118, cleared by Open()

    /**
     * Index into Read()'s setup state machine. Open() and Joypad::Reset() clear it. +0x11c
     */
    int mPhase;

    int mUnknown120; // +0x120, cleared by Open()

    /**
     * Setup progress Read() records. SetVibration() acts only from 2, and Joypad::Reset() clears
     * it. +0x124
     */
    int mUnknown124;

    int mUnknown128;                              // +0x128, Open()'s third argument
    int mUnknown12c;                              // +0x12c, cleared by Open()
    unsigned char mUnknown130[kUnknownByteCount]; // +0x130, cleared by Open()
    short mUnknown134[kUnknownShortCount];        // +0x134, cleared by Open(), written by Read()

    /** The pressure baseline Read() subtracts. +0x13c */
    unsigned char mPressureBaseline[kPressureByteCount];

    /** The actuator levels SetVibration() sends. +0x148 */
    unsigned char mActDirect[kActuatorByteCount];

    /** The actuator alignment Read() sends during setup. +0x14e */
    unsigned char mActAlign[kActuatorByteCount];

private:
    // Run the setup step mPhase selects, given the state scePadGetState() reported. Read()
    // expands it inline, and it has no address of its own.
    void AdvancePhase(int nState);

    // Non-zero once any record has started libpad. It lives at 0x00777fbc.
    static int sPadLibraryStarted;
};
