#ifndef SIFDEV_H
#define SIFDEV_H

// The game was built against Sony's official SDK, whose file-service header is <sifdev.h>. The
// open-source ps2sdk does not provide an equivalent header. It exposes the same service under fio*
// names in <fileio.h> rather than the sce* names the game calls. Only the declarations the
// reconstruction uses are shimmed here. The build therefore does not depend on a mapping that does
// not exist upstream.
// This file is build support for the open-source SDK, not part of the reconstructed source.

#include <iopcontrol.h>
#include <iopheap.h>
#include <loadfile.h>

#define SCE_RDONLY 0x0001

#define SCE_SEEK_SET 0
#define SCE_SEEK_CUR 1
#define SCE_SEEK_END 2

// ps2sdk declares these four under Sif* names with identical signatures. They are forwarded rather
// than redeclared.
#define sceSifAllocIopHeap SifAllocIopHeap
#define sceSifInitIopHeap SifInitIopHeap
#define sceSifLoadModule SifLoadModule
#define sceSifRebootIop SifIopReboot

// The free call forwards the same way.
#define sceSifFreeIopHeap SifFreeIopHeap

#ifdef __cplusplus
extern "C" {
#endif

// Reports whether the IOP has finished booting, and closes the console once it has. ps2sdk's
// SifIopSync() performs the test without the console reset.
int sceSifSyncIop(void);

// The file-service client is defined in sce/ee/src/fileio.c. It speaks the protocol of the
// multi-threaded file server in the IOP replacement image. The ps2sdk client does not.

// Unbinds the file-service client after an IOP reboot. The next call binds the new server.
int sceFsReset(void);

// The optional mode argument is sent to the server as it is.
int sceOpen(const char *pszPath, int nFlags, ...);
int sceClose(int nDescriptor);
int sceRead(int nDescriptor, void *pBuffer, int nBytes);
int sceWrite(int nDescriptor, const void *pBuffer, int nBytes);
int sceLseek(int nDescriptor, int nOffset, int nWhence);

// Sends a control request for an open file to its device.
int sceIoctl(int nDescriptor, int nRequest, void *pArg);

#ifdef __cplusplus
}
#endif

#endif
