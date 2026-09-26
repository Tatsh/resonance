#pragma once

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
