#include <stdint.h>
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

enum {
    kSeekCommand = 5,
};

static unsigned int g_nSeekSector __attribute__((aligned(64)));

// 0x005ae998
// The command is the N-command server's seek, but the callback reports SCECdFuncSeek. The SDK's
// version reports the command number instead. A callback that tests for SCECdFuncSeek does not
// recognise the command number.
int sceCdSeek(unsigned int nSector) {
    if (sceCdDiskReady(SCECdNonblock) == SCECdNotReady) {
        return 0;
    }
    if (_CdCheckNCmd(kSeekCommand) == 0) {
        return 0;
    }
    g_nSeekSector = nSector;
    CdCallbackNum = SCECdFuncSeek;
    cbSema = 1;
    if (sceSifCallRpc(&clientNCmd,
                      kSeekCommand,
                      SIF_RPC_M_NOWAIT,
                      &g_nSeekSector,
                      sizeof(g_nSeekSector),
                      NULL,
                      0,
                      _CdGenericCallbackFunction,
                      (void *)&CdCallbackNum) < 0) {
        CdCallbackNum = 0;
        cbSema = 0;
        SignalSema(nCmdSemaId);
        return 0;
    }
    return 1;
}

enum {
    kStreamCommandRead = 2,
    kStreamRetryTicks = 8,
    kSectorSize = 0x800,
};

// 0x004ff030
static void sceCdAlarmWake(s32 nAlarm, u16 nTime, void *pSema) {
    (void)nAlarm;
    (void)nTime;
    iSignalSema((int)(intptr_t)pSema);
    ExitHandler();
}

// 0x004ff058
// Sleeps for a number of alarm ticks.
static void sceCdSleepTicks(unsigned short nTicks) {
    ee_sema_t sema;
    int semaId;

    sema.init_count = 0;
    sema.max_count = 1;
    sema.option = 0;
    semaId = CreateSema(&sema);
    SetAlarm(nTicks, sceCdAlarmWake, (void *)(intptr_t)semaId);
    WaitSema(semaId);
    DeleteSema(semaId);
}

// 0x00611dc0
// A blocking read resumes each pass at the byte past the sectors already delivered, and waits a
// few ticks when a pass returns neither data nor an error. The SDK's version scales the resume
// offset as words and so writes past the buffer after any partial pass.
int sceCdStRead(unsigned int nSectors, unsigned int *pBuffer, unsigned int nMode,
                unsigned int *pError) {
    unsigned char *pBytes = (unsigned char *)pBuffer;
    unsigned int nRead = 0;
    unsigned int nLastError = 0;
    int reply;

    if (CdDebug > 0) {
        printf("sceCdStRead call read size=%d mode=%d\n", nSectors, nMode);
    }
    if (streamStatus == 0) {
        return 0;
    }
    sceSifWriteBackDCache(pBuffer, (int)(nSectors * kSectorSize));
    if (nMode == STMNBLK) {
        reply = sceCdStream(0, nSectors, pBuffer, kStreamCommandRead, &dummyMode);
        *pError = (unsigned int)reply >> 16;
        return reply & 0xFFFF;
    }
    for (;;) {
        unsigned int nPass;
        unsigned int nError;

        reply = sceCdStream(0, nSectors - nRead, pBytes + nRead * kSectorSize, kStreamCommandRead,
                            &dummyMode);
        nPass = (unsigned int)reply & 0xFFFFU;
        nError = (unsigned int)reply >> 16;
        nRead += nPass;
        if (nError == 0) {
            if (nPass == 0) {
                sceCdSleepTicks(kStreamRetryTicks);
            }
        } else {
            nLastError = nError;
            if (CdDebug > 0) {
                printf("sceCdStRead BLK Read cur_size= %d read_size= %d req_size= %d err 0x%x\n",
                       nRead, nPass, nSectors, nError);
            }
        }
        if (nRead == nSectors || (nError != 0 && nPass == 0)) {
            break;
        }
    }
    if (CdDebug > 0) {
        printf("sceCdStRead BLK Read Ended\n");
    }
    *pError = nLastError;
    return (int)nRead;
}
