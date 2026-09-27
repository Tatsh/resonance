#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Module interface of EZMIDI.IRX.
 *
 * The module exports one entry to the loader, `start`. Everything else runs behind its
 * SIFRPC server, which the main program reaches by server identifier rather than by
 * direct call, so no other routine is an export. The command set the server dispatches
 * is recorded alongside `midiFunc` in `midi_ent.c` as each command is decoded.
 */

/**
 * Enter the module.
 *
 * Initialises the RPC layer, starts the server thread, and reports whether the module
 * stays resident. EZMIDI `0x0`.
 *
 * @param nArgc Argument count from the loader.
 * @param pArgv Argument strings from the loader.
 * @return Zero to stay resident, positive to unload.
 */
int start(int nArgc, char **pArgv);

/**
 * Thread parameters for the server thread.
 *
 * Five words on the stack at entry. Inferred.
 */
struct ThreadParam {
    int mAttr;      /**< +0x00. */
    int mOption;    /**< +0x04. */
    void *mEntry;   /**< +0x08. Entry address. Inferred. */
    int mStackSize; /**< +0x0C. */
    int mPriority;  /**< +0x10. */
};

/**
 * RPC command handler.
 *
 * Inferred.
 */
typedef void *(*SifRpcFunc)(int nCommand, void *pData, int nSize);

/**
 * Run the server thread.
 *
 * Enables interrupts, binds the RPC queue, registers the server, and loops the
 * RPC pump. EZMIDI `0xd0`.
 *
 * @return Zero.
 */
int sce_midi_loop(void);

/**
 * Handle one RPC command.
 *
 * EZMIDI `0x18c`.
 *
 * @param nCommand Command number. Inferred.
 * @param pData Command data. Inferred.
 * @param nSize Data size. Inferred.
 * @return The reply. Inferred.
 */
void *midiFunc(int nCommand, void *pData, int nSize);

/** RPC receive buffer. EZMIDI `0x8570`, sized by the gap to the notes. Inferred. */
extern unsigned char gRpcBuf[0xc0];

#ifdef __cplusplus
}
#endif
