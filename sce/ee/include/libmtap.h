#ifndef LIBMTAP_H
#define LIBMTAP_H

#ifdef __cplusplus
extern "C" {
#endif

/** Multitap access through the mtapman server on the IOP. */

/**
 * Bind the five mtapman servers and check the mtapman version.
 *
 * Exits the program when a bind request cannot be sent.
 *
 * @return 1, or zero on a version mismatch.
 */
int sceMtapInit(void);

/**
 * Start watching a port for a multitap.
 *
 * @param nPort Port.
 * @return mtapman's result, or zero when the call failed.
 */
int sceMtapPortOpen(int nPort);

/**
 * Stop watching a port for a multitap.
 *
 * @param nPort Port.
 * @return mtapman's result, or zero when the call failed.
 */
int sceMtapPortClose(int nPort);

/**
 * Report whether a multitap is connected to a port.
 *
 * @param nPort Port.
 * @return 1 when a multitap is connected, or zero when none is or the call failed.
 */
int sceMtapGetConnection(int nPort);

/**
 * Change the priorities of the mtapman threads.
 *
 * @param nFirstPriority Priority of the first thread.
 * @param nSecondPriority Priority of the second thread.
 * @return mtapman's result, or zero when the call failed.
 */
int sceMtapChangeThreadPriority(int nFirstPriority, int nSecondPriority);

/**
 * Report the mtapman version.
 *
 * @return The major version in bits 8 to 15 and the minor in bits 0 to 7, or zero when the call
 * failed.
 */
int sceMtapGetModVersion(void);

#ifdef __cplusplus
}
#endif

#endif
