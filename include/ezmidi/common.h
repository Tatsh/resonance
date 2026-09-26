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

/**
 * Prepare the tick.
 *
 * EZMIDI `0x1550`.
 */
void hs_tick_setup(void);

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
