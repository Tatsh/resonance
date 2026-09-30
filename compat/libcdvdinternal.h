#ifndef LIBCDVDINTERNAL_H
#define LIBCDVDINTERNAL_H

// State the SDK's libcdvd stores for its N-commands. The SDK declares these in a header it does not
// install. A reconstructed N-command shares them with the SDK's own. sceCdSync() and the command
// semaphore then cover both.
// This file is build support for the open-source SDK, not part of the reconstructed source.

#ifdef __cplusplus
extern "C" {
#endif

// Debug level; above zero the commands print what they send and receive.
extern int CdDebug;

// Semaphore every N-command takes while it runs.
extern int nCmdSemaId;

// The N-command in progress.
extern int nCmdNum;

// Creates the command semaphores on first use.
void _CdSemaInit(void);

#ifdef __cplusplus
}
#endif

#endif
