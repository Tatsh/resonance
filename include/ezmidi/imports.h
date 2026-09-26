#pragma once

/**
 * Foreign routines EZMIDI.IRX calls into.
 *
 * These live in other modules (Sony's sound, SIF, and kernel modules, and the C
 * library) and have no bodies in this tree. Each declaration below mirrors one call
 * the binary makes; argument roles come from those call sites.
 */

/**
 * Convert a note to a pitch value.
 *
 * Sony's sound library. The one call site passes a constant zero last.
 *
 * @param nNote Note value. Inferred.
 * @param nFine Fine tune. Inferred.
 * @param nTune Tune. Inferred.
 * @param nReserved Always zero at the call site.
 * @return The pitch value.
 */
extern int sceSdNote2Pitch(int nNote, int nFine, int nTune, int nReserved);

/**
 * Write a voice parameter.
 *
 * Sony's sound library.
 *
 * @param nEntry Voice entry.
 * @param nValue Value.
 */
extern void sceSdSetParam(int nEntry, int nValue);

/**
 * Write a voice address.
 *
 * Sony's sound library.
 *
 * @param nEntry Voice entry.
 * @param nAddress Address.
 */
extern void sceSdSetAddr(int nEntry, int nAddress);

/**
 * Move a block between IOP memory and SPU memory.
 *
 * Sony's sound library.
 *
 * @param nChannel Channel.
 * @param nMode Mode.
 * @param nSpuAddr SPU address.
 * @param pSource IOP source.
 * @param nSize Size.
 * @return Zero.
 */
extern int sceSdVoiceTrans(int nChannel, int nMode, int nSpuAddr, const void *pSource, int nSize);

/**
 * Poll a block move.
 *
 * Sony's sound library.
 *
 * @param nChannel Channel.
 * @param nMode Mode.
 * @return Nonzero once done.
 */
extern int sceSdVoiceTransStatus(int nChannel, int nMode);
