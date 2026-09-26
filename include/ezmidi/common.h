#pragma once

/**
 * Common support of the EZMIDI synthesiser: transfers, threads, timers, buffers, and
 * the synthesiser control layer.
 *
 * All addresses in this header are relative to the EZMIDI image base.
 */

/**
 * Copy a bank into SPU memory.
 *
 * Hands off to the IOP-to-SPU copier. EZMIDI `0x5744`.
 *
 * @param nSpuAddr SPU address. Inferred.
 * @param pSource IOP source. Inferred.
 * @param nSize Size. Inferred.
 * @return Zero.
 */
int HardSynthLoadBD(int nSpuAddr, const void *pSource, int nSize);

/**
 * Update every playing note and clear the recompute mask.
 *
 * EZMIDI `0x5c54`.
 *
 * @return Zero.
 */
int HardSynthUpdate(void);

/** Nonzero prints tick brackets. EZMIDI `0x6e84`. Inferred. */
extern int gTickTrace;

/** Tick mark printed before sleeping. EZMIDI `0x6e50`. Inferred. */
extern const char gTickPre[];

/** Tick mark printed after waking. EZMIDI `0x6e54`. Inferred. */
extern const char gTickPost[];

/** Created tick thread identifier. EZMIDI `0x8564`. Inferred. */
extern int gTickThread;

/**
 * Tick timer state.
 *
 * Holds the thread identifier with its alarm and clock words. EZMIDI `0x8558`.
 * Inferred.
 */
struct TimerState {
    int mThread; /**< +0x00. Thread identifier. */
    int mTimer;  /**< +0x04. Alarm identifier. */
    int mClock;  /**< +0x08. Clock word. */
};

/** Tick timer state. EZMIDI `0x8558`. Inferred. */
extern struct TimerState gTimerState;

/** Scan count message. EZMIDI `0x6e40`. Inferred. */
extern const char gScanFmt[];

/**
 * One input buffer.
 *
 * Two live in `gInBuf`; the scan stages one into `gStagedBuf`. The count word
 * gates the copy and measures the data past the header.
 */
struct InBuffer {
    int mUnknown00;             /**< +0x00. Copied with the data. Inferred. */
    int mCount;                 /**< +0x04. Data byte count. Inferred. */
    unsigned char mData[0x3f8]; /**< +0x08. */
};

/** Input buffers. EZMIDI `0x7040`. Inferred. */
extern struct InBuffer gInBuf[2];

/** Staged input buffer. EZMIDI `0x7840`. Inferred. */
extern struct InBuffer gStagedBuf;

/**
 * Parse a staged input buffer.
 *
 * EZMIDI `0x5d34`.
 *
 * @param pData Staged data. Inferred.
 * @param nCount Data byte count. Inferred.
 * @param nBuffer Buffer index, ignored by the binary. Inferred.
 * @return Zero, or -1 when a message fails to parse.
 */
int HardSynthParseNew(unsigned char *pData, int nCount, int nBuffer);

/**
 * Silence every voice on a channel.
 *
 * With a -1 channel, silences every playing note. EZMIDI `0x4bf4`.
 *
 * @param nChannel Channel index, or -1 for every channel. Inferred.
 * @param nReset Nonzero resets the tick and flushes. Inferred.
 */
void HardSynthAllNotesOff(int nChannel, int nReset);

/**
 * Handle one MIDI message.
 *
 * EZMIDI `0x4d70`.
 *
 * @param pMsg Message bytes. Inferred.
 * @return The next message, or zero when parsing stops. Inferred.
 */
unsigned char *HandleMidiMessage(unsigned char *pMsg);

/** RPC reply word. EZMIDI `0x6e70`. Inferred. */
extern int gRpcReply;

/** Unknown command message. EZMIDI `0x6c10`. Inferred. */
extern const char gRpcError[];

/**
 * Initialise the synthesiser.
 *
 * EZMIDI `0x6800`.
 *
 * @return The input buffer address. Inferred.
 */
int HardSynthInit(void);

/**
 * Create the tick thread.
 *
 * EZMIDI `0x6054`.
 *
 * @return The thread identifier. Inferred.
 */
int make_thread(void);

/**
 * Arm the tick timer.
 *
 * EZMIDI `0x60c8`.
 *
 * @param pTimer Timer state. Inferred.
 * @return Zero, or negative when arming fails.
 */
int set_timer(struct TimerState *pTimer);

/**
 * Start the tick timer.
 *
 * EZMIDI `0x620c`.
 *
 * @param pTimer Timer state. Inferred.
 * @return Zero, or -1 when starting fails.
 */
int start_timer(struct TimerState *pTimer);

/**
 * Reset the synthesiser.
 *
 * EZMIDI `0x68d0`.
 *
 * @return Zero.
 */
int HardSynthReset(void);

/**
 * Attach bank headers to bank data.
 *
 * EZMIDI `0x579c`.
 *
 * @param nBank Bank index. Inferred.
 * @param nHd Header handle. Inferred.
 * @param nBd0 Data handle. Inferred.
 * @param nBd1 Data handle. Inferred.
 * @return The attach result. Inferred.
 */
int HardSynthAttachHDtoBD(int nBank, int nHd, int nBd0, int nBd1);

/**
 * Configure the synthesiser.
 *
 * EZMIDI `0x66f4`.
 *
 * @param nValue Configuration value. Inferred.
 */
void HardSynthConfig(int nValue);

/**
 * Pause the synthesiser.
 *
 * EZMIDI `0x6390`.
 */
void HardSynthPause(void);

/**
 * Resume the synthesiser.
 *
 * EZMIDI `0x64e0`.
 */
void HardSynthResume(void);

/**
 * Set the remix mode.
 *
 * EZMIDI `0x6660`.
 *
 * @param nMode Remix mode. Inferred.
 * @return Zero.
 */
int HardSynthSetRemix(int nMode);

/**
 * Set the mono mode.
 *
 * Marks every channel for recompute. EZMIDI `0x66a4`.
 *
 * @param nMode Mono mode. Inferred.
 * @return Zero.
 */
int HardSynthSetMono(int nMode);

/**
 * Report synthesiser information.
 *
 * EZMIDI `0x6960`.
 *
 * @param nValue Information selector. Inferred.
 */
void HardSynthInfo(int nValue);

/**
 * Invalidate a bank header.
 *
 * EZMIDI `0x5a28`.
 *
 * @param nValue Header selector. Inferred.
 */
void HardSynthInvalidateHd(int nValue);

/**
 * Invalidate a bank.
 *
 * EZMIDI `0x586c`.
 *
 * @param nValue Bank selector. Inferred.
 * @return The invalidate result. Inferred.
 */
int HardSynthInvalidateBank(int nValue);

/**
 * Kill expired voices.
 *
 * EZMIDI `0x5b7c`.
 *
 * @return Zero.
 */
int HardSynthKillOld(void);

/**
 * Scan an input buffer.
 *
 * EZMIDI `0x5e00`.
 *
 * @param nBuffer Buffer index. Inferred.
 * @return The scanned count. Inferred.
 */
int scan_inbuf(int nBuffer);

/**
 * Run the synthesiser tick.
 *
 * Sleeps, scans the input buffers, updates the voices, and flushes the
 * registers, forever. EZMIDI `0x5f00`.
 */
void hsyn_atick(void);

/**
 * Copy memory from the IOP to the SPU, waiting for the move.
 *
 * EZMIDI `0x8c8`.
 *
 * @param nSpuAddr SPU address. Inferred.
 * @param pSource IOP source. Inferred.
 * @param nSize Size. Inferred.
 * @return Zero.
 */
int MemCpy_IOPtoSPU(int nSpuAddr, const void *pSource, int nSize);
