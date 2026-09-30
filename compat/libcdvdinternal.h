#ifndef LIBCDVDINTERNAL_H
#define LIBCDVDINTERNAL_H

#include <libcdvd.h>
#include <sifrpc.h>

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

// The N-command RPC client.
extern SifRpcClientData_t clientNCmd;

// The value the completion callback reports, and the flag that marks a callback outstanding.
extern volatile int CdCallbackNum;
extern volatile int cbSema;

// Creates the command semaphores on first use.
void _CdSemaInit(void);

// Takes the command semaphore for an N-command, or reports 0 when another is running.
int _CdCheckNCmd(int nCommand);

// Reports the finished N-command to the registered callback.
void _CdGenericCallbackFunction(void *pFunction);

// Nonzero once a stream has been started.
extern int streamStatus;

// The read mode the stream commands pass when the caller supplies none.
extern sceCdRMode dummyMode;

// Sends one stream command and reports the server's packed reply.
int sceCdStream(unsigned int nSector, unsigned int nSectors, void *pBuffer, int nCommand,
                sceCdRMode *pMode);

#ifdef __cplusplus
}
#endif

#endif
