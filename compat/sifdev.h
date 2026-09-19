// The game was built against Sony's official SDK, whose file-service header is <sifdev.h>. The
// open-source ps2sdk has no equivalent header: it exposes the same service under fio* names in
// <fileio.h> rather than the sce* names the game calls. Only the three declarations the
// reconstruction actually uses are shimmed here, so the syntax check does not depend on a mapping
// that does not exist upstream.
// This file is build support for the open-source SDK, not part of the reconstructed source.
#pragma once

#include <iopcontrol.h>
#include <iopheap.h>
#include <loadfile.h>

#define SCE_SEEK_SET 0
#define SCE_SEEK_CUR 1
#define SCE_SEEK_END 2

// ps2sdk declares these four under Sif* names with identical signatures, so they are forwarded
// rather than redeclared.
#define sceSifInitIopHeap SifInitIopHeap
#define sceSifLoadModule SifLoadModule
#define sceSifRebootIop SifIopReboot
#define sceSifSyncIop SifIopSync

#ifdef __cplusplus
extern "C" {
#endif

// Resets the file-service RPC state after an IOP reboot. ps2sdk models no equivalent under this
// name: 0x0056acc8 clears the init flag at 0x00762c08 and the cached client word at 0x008e3be8,
// which is what fioExit() does upstream.
int sceFsReset(void);

int sceOpen(const char *pszPath, int nFlags);
int sceClose(int nDescriptor);
int sceRead(int nDescriptor, void *pBuffer, int nBytes);
int sceWrite(int nDescriptor, const void *pBuffer, int nBytes);
int sceLseek(int nDescriptor, int nOffset, int nWhence);

#ifdef __cplusplus
}
#endif
