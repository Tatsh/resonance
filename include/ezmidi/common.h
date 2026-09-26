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
