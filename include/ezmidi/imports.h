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
