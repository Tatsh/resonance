#include <stdio.h>
#include <string.h>

#include <eekernel.h>
#include <iopcontrol.h>
#include <libcdvd.h>
#include <libcdvdinternal.h>
#include <sifrpc.h>

// Sony's libcdvd file search, in the packet layout the cdvdfsv of the replacement IOP image reads.
// The SDK's own sceCdSearchFile() places the name 4 bytes earlier, where this server reads the
// last word of the result. Every search then misses.

enum {
    kSearchFileServerId = 0x80000597,
    kSearchFileCommand = 1,
    kSearchFileResultSize = 0x24,
    kSearchFileNameSize = 0x100,
    kSearchFileBindDelay = 0x100000,
};

// The packet the server reads. It writes the found file's record back over the result area by
// DMA, at the address in pDest.
typedef struct {
    unsigned char mResult[kSearchFileResultSize];
    char mName[kSearchFileNameSize];
    void *pDest;
} SearchFilePacket;

static SearchFilePacket g_searchFilePacket __attribute__((aligned(64)));
static int g_anSearchFileReceive[4] __attribute__((aligned(64)));
static SifRpcClientData_t g_searchFileClient __attribute__((aligned(64)));

// 0x004ff620
int sceCdSearchFile(sceCdlFILE *pFile, const char *pszName) {
    int nResult;
    unsigned int nLength;
    int i;

    _CdSemaInit();
    if (PollSema(nCmdSemaId) != nCmdSemaId) {
        return 0;
    }
    nCmdNum = kSearchFileCommand;
    if (sceCdSync(1) != 0) {
        SignalSema(nCmdSemaId);
        return 0;
    }
    sceSifInitRpc(0);
    // The binary binds once. The server moves when the IOP reboots, and a reboot binds again.
    if (HasIopRebootedSinceLastCall()) {
        memset(&g_searchFileClient, 0, sizeof(g_searchFileClient));
    }
    while (g_searchFileClient.server == NULL) {
        if (sceSifBindRpc(&g_searchFileClient, kSearchFileServerId, 0) < 0 && CdDebug > 0) {
            printf("Libcdvd bind err CdSearchFile\n");
        }
        if (g_searchFileClient.server != NULL) {
            break;
        }
        for (i = kSearchFileBindDelay; i != -1; --i) {
            __asm__ volatile("nop");
        }
    }

    for (nLength = 0; nLength < kSearchFileNameSize && pszName[nLength] != '\0'; ++nLength) {
        g_searchFilePacket.mName[nLength] = pszName[nLength];
    }
    if (nLength < kSearchFileNameSize) {
        g_searchFilePacket.mName[nLength] = '\0';
    } else {
        g_searchFilePacket.mName[kSearchFileNameSize - 1] = '\0';
    }
    g_searchFilePacket.pDest = &g_searchFilePacket;
    if (CdDebug > 0) {
        printf("ee call cmd search %s\n", g_searchFilePacket.mName);
    }
    if (sceSifCallRpc(&g_searchFileClient,
                      0,
                      0,
                      &g_searchFilePacket,
                      sizeof(g_searchFilePacket),
                      g_anSearchFileReceive,
                      sizeof(g_anSearchFileReceive[0]),
                      NULL,
                      NULL) < 0) {
        SignalSema(nCmdSemaId);
        return 0;
    }
    // The server's record ends in a flag word the SDK's sceCdlFILE does not have.
    memcpy(pFile, (const void *)UNCACHED_SEG(g_searchFilePacket.mResult), sizeof(*pFile));
    if (CdDebug > 0) {
        printf("search name %s\n", pFile->name);
        printf("search size %d\n", (int)pFile->size);
        printf("search loc lbn %d\n", (int)pFile->lsn);
    }
    nResult = *(volatile int *)UNCACHED_SEG(&g_anSearchFileReceive[0]);
    SignalSema(nCmdSemaId);
    return nResult;
}
