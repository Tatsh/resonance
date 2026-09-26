#pragma once

#include "ezmidi/ezmidi.h"

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
 * Read a voice parameter.
 *
 * Sony's sound library. The one call site masks the result to sixteen bits.
 *
 * @param nEntry Voice entry.
 * @return The parameter value.
 */
extern int sceSdGetParam(int nEntry);

/**
 * Write a voice switch.
 *
 * Sony's sound library.
 *
 * @param nEntry Switch entry.
 * @param nValue Value.
 */
extern void sceSdSetSwitch(int nEntry, unsigned int nValue);

/**
 * Read a voice switch.
 *
 * Sony's sound library. The tick setup snapshots the shadows with it.
 *
 * @param nEntry Switch entry.
 * @return The switch value.
 */
extern unsigned int sceSdGetSwitch(int nEntry);

/**
 * Read the system time.
 *
 * Kernel. The tick setup stamps the time with it.
 *
 * @param pTime Two-word time buffer. Inferred.
 * @return The time result.
 */
extern int GetSystemTime(unsigned int *pTime);

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

/**
 * Print a message.
 *
 * C library. The tick passes a bare message with no arguments.
 *
 * @param pFormat Message. Inferred.
 * @return The print result.
 */
extern int printf(const char *pFormat, ...);

/**
 * Sleep the calling thread.
 *
 * Kernel. The tick ignores the wake result.
 *
 * @return The wake result.
 */
extern int SleepThread(void);

/**
 * Copy memory.
 *
 * C library. The input scan stages the buffer with it.
 *
 * @param pDest Destination.
 * @param pSource Source.
 * @param nSize Byte count.
 * @return The destination.
 */
extern void *memcpy(void *pDest, const void *pSource, unsigned int nSize);

/**
 * Enable CPU interrupts.
 *
 * Kernel.
 */
extern void CpuEnableIntr(void);

/**
 * Enable an interrupt.
 *
 * Kernel. The server thread enables two interrupt lines with it.
 *
 * @param nLine Interrupt line. Inferred.
 * @return The interrupt result.
 */
extern int EnableIntr(int nLine);

/**
 * Check SIF initialisation.
 *
 * SIF module.
 *
 * @return Nonzero when initialisation is needed.
 */
extern int sceSifCheckInit(void);

/**
 * Initialise SIF.
 *
 * SIF module.
 */
extern void sceSifInit(void);

/**
 * Initialise the RPC layer.
 *
 * SIF module. Both call sites pass zero.
 *
 * @param nMode Mode. Inferred.
 */
extern void sceSifInitRpc(int nMode);

/**
 * Fetch the calling thread.
 *
 * Kernel.
 *
 * @return The thread identifier.
 */
extern int GetThreadId(void);

/**
 * Bind an RPC queue to a thread.
 *
 * SIF module.
 *
 * @param pQueue Queue. Inferred.
 * @param nThreadId Thread identifier. Inferred.
 */
extern void sceSifSetRpcQueue(void *pQueue, int nThreadId);

/**
 * Register an RPC server.
 *
 * SIF module. The binary passes two zero words and the queue after the buffer.
 *
 * @param pServerData Server data. Inferred.
 * @param nServerId Server identifier. Inferred.
 * @param pFunc Command handler. Inferred.
 * @param pBuf Receive buffer. Inferred.
 * @param nUnknown0 Always zero at the call site. Inferred.
 * @param nUnknown1 Always zero at the call site. Inferred.
 * @param pQueue Queue. Inferred.
 */
extern void sceSifRegisterRpc(void *pServerData,
                              unsigned int nServerId,
                              SifRpcFunc pFunc,
                              void *pBuf,
                              int nUnknown0,
                              int nUnknown1,
                              void *pQueue);

/**
 * Pump the RPC queue forever.
 *
 * SIF module.
 *
 * @param pQueue Queue. Inferred.
 */
extern void sceSifRpcLoop(void *pQueue);

/**
 * Create a thread.
 *
 * Kernel.
 *
 * @param pParam Thread parameters. Inferred.
 * @return The thread identifier, or zero and below when creation fails.
 */
extern int CreateThread(struct ThreadParam *pParam);

/**
 * Start a thread.
 *
 * Kernel. The entry passes zero with the identifier.
 *
 * @param nThreadId Thread identifier. Inferred.
 * @param nArg Argument. Inferred.
 * @return The start result.
 */
extern int StartThread(int nThreadId, int nArg);
