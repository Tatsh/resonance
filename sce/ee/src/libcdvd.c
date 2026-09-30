#include <eekernel.h>
#include <libcdvd.h>
#include <sifcmd.h>
#include <sifrpc.h>
#include <stdio.h>

enum {
    kCdServerInit = 0x80000592,
    kCdServerSCmd = 0x80000593,
    kCdServerNCmd = 0x80000595,
    kCdServerSearchFile = 0x80000597,
    kCdServerDiskReady = 0x8000059a,
    // The SIF command the IOP raises before the console powers off.
    kCdPOffCommand = 0x80000012,

    // N-command server functions.
    kNCmdFuncRead = 1,
    kNCmdFuncSeek = 5,
    kNCmdFuncStop = 7,
    kNCmdFuncStream = 9,
    kNCmdFuncDiskReady = 14,

    // S-command server functions.
    kSCmdFuncReadClock = 1,
    kSCmdFuncGetDiskType = 3,
    kSCmdFuncGetError = 4,
    kSCmdFuncTrayReq = 5,
    kSCmdFuncMmode = 34,

    // The command each caller records while it has an N-command or S-command semaphore.
    kNCmdSearchFile = 1,
    kNCmdDiskReady = 2,
    kNCmdRead = 4,
    kNCmdSeek = 9,
    kNCmdStop = 11,
    kNCmdStream = 15,
    kSCmdGetDiskType = 1,
    kSCmdGetError = 3,
    kSCmdReadClock = 15,
    kSCmdTrayReq = 26,
    kSCmdMmode = 34,

    // Stream commands of the N-command server's stream function.
    kStreamStart = 1,
    kStreamRead = 2,
    kStreamStop = 3,
    kStreamSeek = 4,
    kStreamInit = 5,
    kStreamStat = 6,
    kStreamPause = 7,
    kStreamResume = 8,
    kStreamSeekF = 9,

    // A function code whose completion releases the library without calling the callback.
    kCdFuncUnreported = 11,
    // The value that requests the callback thread to end.
    kCdFuncExitThread = -1,

    // sceCdDiskReady() reports a busy library as -1 for this mode.
    kDiskReadyModeReportBusy = 8,
    kDiskReadyBusy = -1,

    // The last init reply word does not change the debug level, or raises it to 1.
    kInitDebugUnchanged = 255,
    kInitDebugOn = 254,
    // IOP module versions below this major number make sceCdInit() report kInitOldModules.
    kInitMajorVersionMin = 2,
    kInitVersionMajorDivisor = 256,
    kInitDone = 1,
    kInitOldModules = 2,

    kUnbound = -1,
    kBound = 0,
    kNoSemaphore = -1,
    kBindDelay = 0x100000,
    kSyncSleepTicks = 60,
    kStreamRetryTicks = 8,
    kSectorSize = 2048,
    kSectorSize2328 = 2328,
    kSectorSize2340 = 2340,
    kSearchNameSize = 256,
    kReadEdgeSize = 64,
    kStreamErrorShift = 16,
    kStreamCountMask = 0xffff,
    kCacheLine = 64,

    // Bits of g_nCdEeReadMode.
    kEeReadModeNoDiskCheck = 1,
    kEeReadModeNoWriteBack = 2,

    // Words of the S-command reply buffer.
    kReplyTrayCount = 1,
    kReplyClock = 1,
    kReplyInitDriverVersion = 1,
    kReplyInitServerVersion = 2,
    kReplyInitDebug = 3,
    kReplyTrayWords = 2,
    kReplyClockWords = 4,
    kReplyInitWords = 4,
};

// The file search packet. The server writes the found record over mFile, at the address in pDest.
typedef struct {
    sceCdlFILE mFile;
    char mName[kSearchNameSize];
    void *pDest;
} CdSearchFilePacket;

// The unaligned head and tail of a read. The server delivers them here instead of into the
// destination, and the completion copies them out.
typedef struct {
    int nHeadSize;
    int nTailSize;
    unsigned char *pHeadDest;
    unsigned char *pTailDest;
    unsigned char mHead[kReadEdgeSize];
    unsigned char mTail[kReadEdgeSize];
} CdReadEdges;

typedef struct {
    unsigned int nLsn;
    unsigned int nSectors;
    void *pBuffer;
    sceCdRMode mMode;
    CdReadEdges *pEdges;
    volatile unsigned int *pProgress;
} CdReadPacket;

typedef struct {
    unsigned int nLsn;
    unsigned int nSectors;
    void *pBuffer;
    int nCommand;
    sceCdRMode mMode;
} CdStreamPacket;

// The N-command send buffer each N-command fills in its layout.
typedef union {
    unsigned int nSeekLsn;
    CdReadPacket mRead;
    CdStreamPacket mStream;
} CdNCmdPacket;

static int g_nCdDebug;
static int g_nCdCallbackThreadId;
static int g_nCdSCmdCurrent;
static int g_nCdNCmdCurrent;
static int g_nCdCallbackSemaId = kNoSemaphore;
// Nonzero while sceCdInit() or the handler installation runs. The power-off notice is ignored then.
static volatile int g_bCdPOffBlocked;
static int g_nCdNCmdSemaId = kNoSemaphore;
static int g_nCdSCmdSemaId = kNoSemaphore;
// Nonzero from the start of an asynchronous N-command until its callback has run.
static volatile int g_bCdCallbackPending;
// Only sceCdInit() writes the read mode, and it writes zero.
static int g_nCdEeReadMode;
static int g_nCdNCmdBound = kUnbound;
static int g_nCdPOffInstalled = kUnbound;
static int g_nCdSearchFileBound = kUnbound;
static int g_nCdDiskReadyBound = kUnbound;
static int g_nCdSCmdBound = kUnbound;
static int g_nCdInitBound = kUnbound;
static int g_nCdInitCount;
static volatile int g_nCdCallbackFunction;
static volatile int g_nCdCallbackReport;

// SIF DMA moves whole quadwords from an aligned address. Retail places the RPC send and receive
// buffers on 64-byte boundaries (the disk-ready word on a 16-byte one), and each buffer here starts
// a cache line.
static unsigned int g_anCdNCmdReceive[kCacheLine / sizeof(unsigned int)]
    __attribute__((aligned(kCacheLine)));
static CdNCmdPacket g_cdNCmdSend __attribute__((aligned(kCacheLine)));
static CdReadEdges g_cdReadEdges __attribute__((aligned(kCacheLine)));
static volatile unsigned int g_nCdReadProgress __attribute__((aligned(kCacheLine)));
static sceSifClientData g_cdNCmdClient __attribute__((aligned(kCacheLine)));
static unsigned int g_anCdSCmdReceive[kCacheLine / sizeof(unsigned int)]
    __attribute__((aligned(kCacheLine)));
static unsigned int g_nCdSCmdSend __attribute__((aligned(kCacheLine)));
static sceSifClientData g_cdSCmdClient __attribute__((aligned(kCacheLine)));

static sceCdCBFunc g_pfnCdCallback;
static sceCdPOffFunc g_pfnCdPOffCallback;
static void *g_pCdPOffArgument;
// Cleared when the callback thread ends. It is never read.
static int g_nCdCallbackThreadEnded;
static int g_nCdCallerThreadId;
static struct ThreadParam g_cdCallerThreadParam;
static struct ThreadParam g_cdCallbackThreadParam;
static CdSearchFilePacket g_cdSearchFilePacket __attribute__((aligned(kCacheLine)));
static unsigned int g_anCdSearchFileReceive[kCacheLine / sizeof(unsigned int)]
    __attribute__((aligned(kCacheLine)));
static sceSifClientData g_cdSearchFileClient __attribute__((aligned(kCacheLine)));
static sceSifClientData g_cdInitClient __attribute__((aligned(kCacheLine)));
static sceSifClientData g_cdDiskReadyClient __attribute__((aligned(kCacheLine)));
static unsigned int g_nCdInitSend __attribute__((aligned(kCacheLine)));
static unsigned int g_nCdDiskReadySend __attribute__((aligned(kCacheLine)));

static int g_nCdStreamActive;
static sceCdRMode g_cdStreamDefaultMode;

static inline void sceCdBindDelay(void) {
    int i;

    for (i = kBindDelay; i != -1; --i) {
        __asm__ volatile("nop\n\tnop\n\tnop\n\tnop");
    }
}

// Binds until the server responds, pausing after each failure or empty reply.
static inline void
sceCdBindServer(sceSifClientData *pClient, unsigned int nServer, const char *pszBindError) {
    for (;;) {
        if (sceSifBindRpc(pClient, nServer, 0) < 0) {
            if (g_nCdDebug > 0) {
                printf(pszBindError);
            }
        } else if (pClient->serve != NULL) {
            return;
        }
        sceCdBindDelay();
    }
}

static inline int sceCdReceivedWord(unsigned int *pReceive) {
    return *(volatile int *)UNCACHED_SEG(pReceive);
}

// 0x004ff030
static void sceCdAlarmWake(int nAlarm, unsigned short nTime, void *pSema) {
    (void)nAlarm;
    (void)nTime;
    iSignalSema((int)pSema);
    ExitHandler();
}

// 0x004ff058
static void sceCdSleepTicks(unsigned short nTicks) {
    struct SemaParam sema;
    int nSemaId;

    sema.maxCount = 1;
    sema.initCount = 0;
    sema.option = 0;
    nSemaId = CreateSema(&sema);
    SetAlarm(nTicks, sceCdAlarmWake, (void *)nSemaId);
    WaitSema(nSemaId);
    DeleteSema(nSemaId);
}

// 0x004ff0c0
sceCdCBFunc sceCdCallback(sceCdCBFunc func) {
    sceCdCBFunc pfnPrevious;

    if (sceCdSync(SCECdNonblock) != 0) {
        return NULL;
    }
    DIntr();
    pfnPrevious = g_pfnCdCallback;
    g_pfnCdCallback = func;
    EIntr();
    return pfnPrevious;
}

// 0x004ff118
// Runs in interrupt context when an asynchronous N-command ends.
static void sceCdRpcEnd(void *pFunction) {
    g_nCdCallbackFunction = *(volatile int *)pFunction;
    g_nCdCallbackReport = g_nCdCallbackFunction;
    if (g_nCdCallbackFunction == kCdFuncUnreported) {
        g_nCdCallbackFunction = 0;
        g_bCdCallbackPending = 0;
        return;
    }
    iSignalSema(g_nCdNCmdSemaId);
    if (g_nCdCallbackThreadId != 0 && g_pfnCdCallback != NULL) {
        iSignalSema(g_nCdCallbackSemaId);
    } else {
        g_bCdCallbackPending = 0;
    }
    g_nCdCallbackFunction = 0;
}

// 0x004ff1b8
static void sceCdCallbackThread(void *pArgument) {
    (void)pArgument;
    for (;;) {
        WaitSema(g_nCdCallbackSemaId);
        if (g_nCdCallbackFunction == kCdFuncExitThread) {
            g_bCdCallbackPending = 0;
            g_nCdCallbackFunction = 0;
            g_nCdCallbackThreadId = 0;
            g_nCdCallbackThreadEnded = 0;
            ExitDeleteThread();
        }
        if (g_nCdDebug > 0) {
            printf(
                "sceCdCbfunc= %d sceCdCbfunc_num= %d\n", (int)g_pfnCdCallback, g_nCdCallbackReport);
        }
        if (g_pfnCdCallback != NULL && g_nCdCallbackReport != 0) {
            g_pfnCdCallback(g_nCdCallbackReport);
        }
        g_bCdCallbackPending = 0;
    }
}

// 0x004ff278
int sceCdInitEeCB(int priority, void *stack, int stacksize) {
    if (g_nCdCallbackThreadId != 0) {
        ChangeThreadPriority(g_nCdCallbackThreadId, priority);
        return 0;
    }
    g_nCdCallerThreadId = GetThreadId();
    ReferThreadStatus(g_nCdCallerThreadId, &g_cdCallerThreadParam);
    g_cdCallbackThreadParam.stackSize = stacksize;
    g_cdCallbackThreadParam.gpReg = _gp;
    g_cdCallbackThreadParam.entry = sceCdCallbackThread;
    g_cdCallbackThreadParam.stack = stack;
    g_cdCallbackThreadParam.initPriority = priority;
    g_nCdCallbackThreadId = CreateThread(&g_cdCallbackThreadParam);
    StartThread(g_nCdCallbackThreadId, NULL);
    return 1;
}

// 0x004ff350
// Runs in interrupt context when sceCdRead() ends.
static void sceCdReadEnd(void *pEdges) {
    volatile CdReadEdges *pUncached = UNCACHED_SEG(pEdges);
    int i;

    for (i = 0; i < pUncached->nHeadSize; ++i) {
        pUncached->pHeadDest[i] = pUncached->mHead[i];
    }
    for (i = 0; i < pUncached->nTailSize; ++i) {
        pUncached->pTailDest[i] = pUncached->mTail[i];
    }
    sceCdRpcEnd((void *)&g_nCdCallbackFunction);
}

// 0x004ff3f0
static void sceCdSemaInit(void) {
    struct SemaParam sema;

    if (g_nCdNCmdSemaId != kNoSemaphore && g_nCdSCmdSemaId != kNoSemaphore) {
        return;
    }
    sema.option = 0;
    sema.maxCount = 1;
    sema.initCount = 1;
    g_nCdNCmdSemaId = CreateSema(&sema);
    g_nCdSCmdSemaId = CreateSema(&sema);
    sema.initCount = 0;
    g_nCdCallbackSemaId = CreateSema(&sema);
    g_bCdCallbackPending = 0;
}

// 0x004ff488
static void sceCdSemaExit(void) {
    if (g_nCdCallbackThreadId != 0) {
        g_nCdCallbackFunction = kCdFuncExitThread;
        SignalSema(g_nCdCallbackSemaId);
    }
    DeleteSema(g_nCdNCmdSemaId);
    DeleteSema(g_nCdSCmdSemaId);
    DeleteSema(g_nCdCallbackSemaId);
    DIntr();
    sceSifRemoveCmdHandler(kCdPOffCommand);
    EIntr();
}

// 0x004ff578
static void sceCdPOffHandler(void *pPacket, void *pData) {
    (void)pPacket;
    (void)pData;
    if (g_pfnCdPOffCallback != NULL && g_bCdPOffBlocked == 0) {
        g_pfnCdPOffCallback(g_pCdPOffArgument);
    }
}

// 0x004ff5b8
static int sceCdPOffHandlerInstall(void) {
    g_bCdPOffBlocked = 1;
    DIntr();
    sceSifAddCmdHandler(kCdPOffCommand, sceCdPOffHandler, NULL);
    EIntr();
    g_bCdPOffBlocked = 0;
    g_nCdPOffInstalled = 1;
    return 1;
}

// 0x004ff508
sceCdPOffFunc sceCdPOffCallback(sceCdPOffFunc func, void *addr) {
    sceCdPOffFunc pfnPrevious;

    if (g_nCdPOffInstalled < 0) {
        sceCdPOffHandlerInstall();
    }
    DIntr();
    pfnPrevious = g_pfnCdPOffCallback;
    g_pCdPOffArgument = addr;
    g_pfnCdPOffCallback = func;
    EIntr();
    return pfnPrevious;
}

// 0x004ff620
int sceCdSearchFile(sceCdlFILE *fp, const char *name) {
    int nResult;
    int i;

    sceCdSemaInit();
    if (PollSema(g_nCdNCmdSemaId) != g_nCdNCmdSemaId) {
        return 0;
    }
    g_nCdNCmdCurrent = kNCmdSearchFile;
    ReferThreadStatus(g_nCdCallerThreadId, &g_cdCallerThreadParam);
    if (sceCdSync(SCECdNonblock) != 0) {
        SignalSema(g_nCdNCmdSemaId);
        return 0;
    }
    sceSifInitRpc(0);
    if (g_nCdSearchFileBound < 0) {
        sceCdBindServer(
            &g_cdSearchFileClient, kCdServerSearchFile, "Libcdvd bind err CdSearchFile\n");
        g_nCdSearchFileBound = kBound;
    }

    for (i = 0; i < kSearchNameSize; ++i) {
        g_cdSearchFilePacket.mName[i] = name[i];
        if (name[i] == '\0') {
            break;
        }
    }
    if (i == kSearchNameSize) {
        g_cdSearchFilePacket.mName[kSearchNameSize - 1] = '\0';
    }
    g_cdSearchFilePacket.pDest = &g_cdSearchFilePacket;
    if (g_nCdDebug > 0) {
        printf("ee call cmd search %s\n", g_cdSearchFilePacket.mName);
    }
    sceSifWriteBackDCache(&g_cdSearchFilePacket, sizeof(g_cdSearchFilePacket));
    if (sceSifCallRpc(&g_cdSearchFileClient,
                      0,
                      0,
                      &g_cdSearchFilePacket,
                      sizeof(g_cdSearchFilePacket),
                      g_anCdSearchFileReceive,
                      sizeof(g_anCdSearchFileReceive[0]),
                      NULL,
                      NULL) < 0) {
        SignalSema(g_nCdNCmdSemaId);
        return 0;
    }
    *fp = *(sceCdlFILE *)UNCACHED_SEG(&g_cdSearchFilePacket.mFile);
    if (g_nCdDebug > 0) {
        printf("search name %s\n", fp->name);
        printf("search size %d\n", fp->size);
        printf("search loc lbn %d\n", fp->lsn);
    }
    nResult = sceCdReceivedWord(g_anCdSearchFileReceive);
    SignalSema(g_nCdNCmdSemaId);
    return nResult;
}

// 0x004ff920
// Takes the N-command semaphore for a command and binds the server on first use.
static int sceCdCheckNCmd(int nCommand) {
    sceCdSemaInit();
    if (PollSema(g_nCdNCmdSemaId) != g_nCdNCmdSemaId) {
        if (g_nCdDebug > 0) {
            printf("Ncmd fail sema cur_cmd:%d keep_cmd:%d\n", nCommand, g_nCdNCmdCurrent);
        }
        return 0;
    }
    g_nCdNCmdCurrent = nCommand;
    ReferThreadStatus(g_nCdCallerThreadId, &g_cdCallerThreadParam);
    if (sceCdSync(SCECdNonblock) != 0) {
        SignalSema(g_nCdNCmdSemaId);
        return 0;
    }
    sceSifInitRpc(0);
    if (g_nCdNCmdBound < 0) {
        sceCdBindServer(&g_cdNCmdClient, kCdServerNCmd, "Libcdvd bind err N CMD\n");
        g_nCdNCmdBound = kBound;
    }
    return 1;
}

// 0x004ffa90
// The drive check the N-commands run first. It reports SCECdComplete or SCECdNotReady.
static int sceCdNCmdDiskReady(void) {
    int nResult;

    if (sceCdCheckNCmd(kNCmdDiskReady) == 0) {
        return 0;
    }
    if (sceSifCallRpc(&g_cdNCmdClient,
                      kNCmdFuncDiskReady,
                      0,
                      NULL,
                      0,
                      g_anCdNCmdReceive,
                      sizeof(g_anCdNCmdReceive[0]),
                      NULL,
                      NULL) < 0) {
        SignalSema(g_nCdNCmdSemaId);
        return 0;
    }
    nResult = sceCdReceivedWord(g_anCdNCmdReceive);
    SignalSema(g_nCdNCmdSemaId);
    return nResult;
}

// 0x004ffb28
int sceCdSync(int mode) {
    if (mode == SCECdBlock) {
        if (g_nCdDebug > 0) {
            printf("N cmd wait\n");
        }
        while (g_bCdCallbackPending != 0 || sceSifCheckStatRpc(&g_cdNCmdClient.rpcd) != 0) {
            sceCdSleepTicks(kSyncSleepTicks);
        }
        return 0;
    }
    if (g_bCdCallbackPending != 0 || sceSifCheckStatRpc(&g_cdNCmdClient.rpcd) != 0) {
        return 1;
    }
    return 0;
}

// 0x004ffbc8
int sceCdSyncS(int mode) {
    if (mode == SCECdBlock) {
        if (g_nCdDebug > 0) {
            printf("S cmd wait\n");
        }
        while (sceSifCheckStatRpc(&g_cdSCmdClient.rpcd) != 0) {
            sceCdSleepTicks(kSyncSleepTicks);
        }
        return 0;
    }
    return sceSifCheckStatRpc(&g_cdSCmdClient.rpcd);
}

// 0x004ffc38
// Takes the S-command semaphore for a command and binds the server on first use.
static int sceCdCheckSCmd(int nCommand) {
    sceCdSemaInit();
    if (PollSema(g_nCdSCmdSemaId) != g_nCdSCmdSemaId) {
        if (g_nCdDebug > 0) {
            printf("Scmd fail sema cur_cmd:%d keep_cmd:%d\n", nCommand, g_nCdSCmdCurrent);
        }
        return 0;
    }
    g_nCdSCmdCurrent = nCommand;
    ReferThreadStatus(g_nCdCallerThreadId, &g_cdCallerThreadParam);
    if (sceCdSyncS(SCECdNonblock) != 0) {
        SignalSema(g_nCdSCmdSemaId);
        return 0;
    }
    sceSifInitRpc(0);
    if (g_nCdSCmdBound < 0) {
        sceCdBindServer(&g_cdSCmdClient, kCdServerSCmd, "Libcdvd bind err S cmd\n");
        g_nCdSCmdBound = kBound;
    }
    return 1;
}

// 0x004ffda8
int sceCdInit(int init_mode) {
    int nResult;
    int nBind;
    int nDebug;
    int nDriverVersion;
    int nServerVersion;

    if (sceCdSyncS(SCECdNonblock) != 0) {
        return 0;
    }
    sceSifInitRpc(0);
    g_nCdCallerThreadId = GetThreadId();
    g_bCdPOffBlocked = 1;
    g_nCdPOffInstalled = kUnbound;
    g_nCdSearchFileBound = kUnbound;
    g_nCdNCmdBound = kUnbound;
    g_nCdSCmdBound = kUnbound;
    g_nCdDiskReadyBound = kUnbound;
    g_nCdEeReadMode = 0;
    ++g_nCdInitCount;
    g_nCdInitBound = kUnbound;
    for (;;) {
        nBind = sceSifBindRpc(&g_cdInitClient, kCdServerInit, 0);
        if (nBind < 0) {
            if (g_nCdDebug > 0) {
                printf("Libcdvd bind err %d CD_Init %d\n", nBind, g_nCdInitCount);
            }
        } else if (g_cdInitClient.serve != NULL) {
            break;
        }
        sceCdBindDelay();
    }
    g_nCdInitSend = (unsigned int)init_mode;
    g_nCdInitBound = kBound;
    sceSifWriteBackDCache(&g_nCdInitSend, sizeof(g_nCdInitSend));
    if (sceSifCallRpc(&g_cdInitClient,
                      0,
                      0,
                      &g_nCdInitSend,
                      sizeof(g_nCdInitSend),
                      g_anCdSCmdReceive,
                      kReplyInitWords * sizeof(g_anCdSCmdReceive[0]),
                      NULL,
                      NULL) < 0) {
        g_bCdPOffBlocked = 0;
        return 0;
    }

    nDriverVersion = sceCdReceivedWord(&g_anCdSCmdReceive[kReplyInitDriverVersion]);
    nServerVersion = sceCdReceivedWord(&g_anCdSCmdReceive[kReplyInitServerVersion]);
    nDebug = sceCdReceivedWord(&g_anCdSCmdReceive[kReplyInitDebug]);
    nResult = kInitDone;
    if (nDebug != kInitDebugUnchanged) {
        if (nDebug == kInitDebugOn) {
            g_nCdDebug = 1;
        } else if (nDriverVersion / kInitVersionMajorDivisor < kInitMajorVersionMin ||
                   nServerVersion / kInitVersionMajorDivisor < kInitMajorVersionMin) {
            nResult = kInitOldModules;
        }
    }
    g_bCdPOffBlocked = 0;

    if (init_mode == SCECdEXIT) {
        if (g_nCdDebug > 0) {
            printf("Libcdvd Exit\n");
        }
        sceCdSemaExit();
        g_nCdNCmdSemaId = kNoSemaphore;
        g_nCdSCmdSemaId = kNoSemaphore;
        g_nCdCallbackSemaId = kNoSemaphore;
        return nResult;
    }
    sceCdSemaInit();
    sceCdPOffHandlerInstall();
    return nResult;
}

// 0x00500088
int sceCdDiskReady(int mode) {
    int nResult;

    if (g_nCdDebug > 0) {
        printf("DiskReady 0\n");
    }
    sceCdSemaInit();
    if (PollSema(g_nCdSCmdSemaId) != g_nCdSCmdSemaId) {
        return SCECdNotReady;
    }
    if (sceCdSyncS(SCECdNonblock) == 0) {
        sceSifInitRpc(0);
        if (g_nCdDiskReadyBound < 0) {
            sceCdBindServer(
                &g_cdDiskReadyClient, kCdServerDiskReady, "Libcdvd bind err CdDiskReady\n");
            g_nCdDiskReadyBound = kBound;
        }
        g_nCdDiskReadySend = (unsigned int)mode;
        sceSifWriteBackDCache(&g_nCdDiskReadySend, sizeof(g_nCdDiskReadySend));
        if (sceSifCallRpc(&g_cdDiskReadyClient,
                          0,
                          0,
                          &g_nCdDiskReadySend,
                          sizeof(g_nCdDiskReadySend),
                          g_anCdSCmdReceive,
                          sizeof(g_anCdSCmdReceive[0]),
                          NULL,
                          NULL) >= 0) {
            if (g_nCdDebug > 0) {
                printf("DiskReady ended\n");
            }
            nResult = sceCdReceivedWord(g_anCdSCmdReceive);
            SignalSema(g_nCdSCmdSemaId);
            return nResult;
        }
    }
    SignalSema(g_nCdSCmdSemaId);
    return (mode == kDiskReadyModeReportBusy) ? kDiskReadyBusy : SCECdNotReady;
}

// Sends an S-command whose reply begins with the result word.
static int sceCdSCmdCall(unsigned int nFunction, void *pSend, int nSendSize, int nReceiveSize) {
    return sceSifCallRpc(&g_cdSCmdClient,
                         nFunction,
                         0,
                         pSend,
                         nSendSize,
                         g_anCdSCmdReceive,
                         nReceiveSize,
                         NULL,
                         NULL);
}

// 0x00500280
int sceCdMmode(int media) {
    int nResult;

    if (sceCdCheckSCmd(kSCmdMmode) == 0) {
        return 0;
    }
    g_nCdSCmdSend = (unsigned int)media;
    sceSifWriteBackDCache(&g_nCdSCmdSend, sizeof(g_nCdSCmdSend));
    if (sceCdSCmdCall(
            kSCmdFuncMmode, &g_nCdSCmdSend, sizeof(g_nCdSCmdSend), sizeof(g_anCdSCmdReceive[0])) <
        0) {
        SignalSema(g_nCdSCmdSemaId);
        return 0;
    }
    nResult = sceCdReceivedWord(g_anCdSCmdReceive);
    SignalSema(g_nCdSCmdSemaId);
    return nResult;
}

// Starts an asynchronous N-command that reports nFunction to the callback when it ends.
static int sceCdNCmdCallAsync(unsigned int nServerFunction,
                              int nFunction,
                              void *pSend,
                              int nSendSize,
                              sceSifEndFunc pfnEnd,
                              void *pEndArgument) {
    g_nCdCallbackFunction = nFunction;
    g_bCdCallbackPending = 1;
    if (sceSifCallRpc(&g_cdNCmdClient,
                      nServerFunction,
                      SIF_RPCM_NOWAIT,
                      pSend,
                      nSendSize,
                      NULL,
                      0,
                      pfnEnd,
                      pEndArgument) < 0) {
        g_nCdCallbackFunction = 0;
        g_bCdCallbackPending = 0;
        SignalSema(g_nCdNCmdSemaId);
        return 0;
    }
    return 1;
}

// 0x00519928
int sceCdStop(void) {
    if (sceCdNCmdDiskReady() == SCECdNotReady) {
        return 0;
    }
    if (sceCdCheckNCmd(kNCmdStop) == 0) {
        return 0;
    }
    return sceCdNCmdCallAsync(
        kNCmdFuncStop, SCECdFuncStop, NULL, 0, sceCdRpcEnd, (void *)&g_nCdCallbackFunction);
}

// 0x00545bd0
int sceCdGetDiskType(void) {
    int nResult;

    if (sceCdCheckSCmd(kSCmdGetDiskType) == 0) {
        return 0;
    }
    if (sceCdSCmdCall(kSCmdFuncGetDiskType, NULL, 0, sizeof(g_anCdSCmdReceive[0])) < 0) {
        SignalSema(g_nCdSCmdSemaId);
        return 0;
    }
    nResult = sceCdReceivedWord(g_anCdSCmdReceive);
    SignalSema(g_nCdSCmdSemaId);
    return nResult;
}

// 0x00558c20
int sceCdTrayReq(int param, unsigned int *traycnt) {
    int nResult;

    if (sceCdCheckSCmd(kSCmdTrayReq) == 0) {
        return -1;
    }
    g_nCdSCmdSend = (unsigned int)param;
    sceSifWriteBackDCache(&g_nCdSCmdSend, sizeof(g_nCdSCmdSend));
    if (sceCdSCmdCall(kSCmdFuncTrayReq,
                      &g_nCdSCmdSend,
                      sizeof(g_nCdSCmdSend),
                      kReplyTrayWords * sizeof(g_anCdSCmdReceive[0])) < 0) {
        SignalSema(g_nCdSCmdSemaId);
        return -1;
    }
    if (traycnt != NULL) {
        *traycnt = (unsigned int)sceCdReceivedWord(&g_anCdSCmdReceive[kReplyTrayCount]);
    }
    nResult = sceCdReceivedWord(g_anCdSCmdReceive);
    SignalSema(g_nCdSCmdSemaId);
    return nResult;
}

// 0x0059bdf8
int sceCdRead(unsigned int lsn, unsigned int sectors, void *buf, sceCdRMode *mode) {
    CdReadPacket *pPacket = &g_cdNCmdSend.mRead;
    unsigned int nBytes;
    int nStarted;

    if ((g_nCdEeReadMode & kEeReadModeNoDiskCheck) == 0 && sceCdNCmdDiskReady() == SCECdNotReady) {
        return 0;
    }
    if (sceCdCheckNCmd(kNCmdRead) == 0) {
        return 0;
    }
    pPacket->nLsn = lsn;
    pPacket->nSectors = sectors;
    pPacket->pBuffer = buf;
    pPacket->mMode.trycount = mode->trycount;
    pPacket->mMode.spindlctrl = mode->spindlctrl;
    pPacket->mMode.datapattern = mode->datapattern;
    pPacket->pEdges = &g_cdReadEdges;
    pPacket->pProgress = &g_nCdReadProgress;
    switch (mode->datapattern) {
    case SCECdSecS2328:
        nBytes = sectors * kSectorSize2328;
        break;
    case SCECdSecS2340:
        nBytes = sectors * kSectorSize2340;
        break;
    default:
        nBytes = sectors * kSectorSize;
        break;
    }
    g_nCdReadProgress = 0;
    if ((g_nCdEeReadMode & kEeReadModeNoWriteBack) == 0) {
        sceSifWriteBackDCache(buf, (int)nBytes);
    }
    sceSifWriteBackDCache(&g_cdReadEdges, sizeof(g_cdReadEdges));
    sceSifWriteBackDCache(pPacket, sizeof(*pPacket));
    sceSifWriteBackDCache((void *)&g_nCdReadProgress, sizeof(g_nCdReadProgress));
    if (g_nCdDebug > 0) {
        printf("call cdread cmd\n");
    }
    nStarted = sceCdNCmdCallAsync(
        kNCmdFuncRead, SCECdFuncRead, pPacket, sizeof(*pPacket), sceCdReadEnd, &g_cdReadEdges);
    if (nStarted != 0 && g_nCdDebug > 0) {
        printf("cdread end\n");
    }
    return nStarted;
}

// 0x005ae998
int sceCdSeek(unsigned int lsn) {
    if (sceCdNCmdDiskReady() == SCECdNotReady) {
        return 0;
    }
    if (sceCdCheckNCmd(kNCmdSeek) == 0) {
        return 0;
    }
    g_cdNCmdSend.nSeekLsn = lsn;
    sceSifWriteBackDCache(&g_cdNCmdSend.nSeekLsn, sizeof(g_cdNCmdSend.nSeekLsn));
    return sceCdNCmdCallAsync(kNCmdFuncSeek,
                              SCECdFuncSeek,
                              &g_cdNCmdSend.nSeekLsn,
                              sizeof(g_cdNCmdSend.nSeekLsn),
                              sceCdRpcEnd,
                              (void *)&g_nCdCallbackFunction);
}

// 0x005e0270
int sceCdGetError(void) {
    int nResult;

    if (sceCdCheckSCmd(kSCmdGetError) == 0) {
        return -1;
    }
    if (sceCdSCmdCall(kSCmdFuncGetError, NULL, 0, sizeof(g_anCdSCmdReceive[0])) < 0) {
        SignalSema(g_nCdSCmdSemaId);
        return -1;
    }
    nResult = sceCdReceivedWord(g_anCdSCmdReceive);
    SignalSema(g_nCdSCmdSemaId);
    return nResult;
}

// 0x005fc468
int sceCdReadClock(sceCdCLOCK *rtc) {
    int nResult;

    if (sceCdCheckSCmd(kSCmdReadClock) == 0) {
        return 0;
    }
    if (g_nCdDebug > 0) {
        printf("Libcdvd call Clock read 1\n");
    }
    if (sceCdSCmdCall(
            kSCmdFuncReadClock, NULL, 0, kReplyClockWords * sizeof(g_anCdSCmdReceive[0])) < 0) {
        SignalSema(g_nCdSCmdSemaId);
        return 0;
    }
    *rtc = *(sceCdCLOCK *)UNCACHED_SEG(&g_anCdSCmdReceive[kReplyClock]);
    if (g_nCdDebug > 0) {
        printf("Libcdvd call Clock read 2\n");
    }
    nResult = sceCdReceivedWord(g_anCdSCmdReceive);
    SignalSema(g_nCdSCmdSemaId);
    return nResult;
}

// 0x00612038
// Sends one stream command. The reply packs the sector count in the low half and the drive error
// in the high half.
static int sceCdStream(
    unsigned int nLsn, unsigned int nSectors, void *pBuffer, int nCommand, sceCdRMode *pMode) {
    CdStreamPacket *pPacket = &g_cdNCmdSend.mStream;
    int nResult;

    if (sceCdCheckNCmd(kNCmdStream) == 0) {
        return 0;
    }
    if (g_nCdDebug > 0) {
        printf("call cdreadstm call\n");
    }
    pPacket->nLsn = nLsn;
    pPacket->nSectors = nSectors;
    pPacket->pBuffer = pBuffer;
    pPacket->nCommand = nCommand;
    if (pMode != NULL) {
        pPacket->mMode.trycount = pMode->trycount;
        pPacket->mMode.spindlctrl = pMode->spindlctrl;
        pPacket->mMode.datapattern = pMode->datapattern;
    }
    if (g_nCdDebug > 0) {
        printf("call cdreadstm cmd\n");
    }
    sceSifWriteBackDCache(pPacket, sizeof(*pPacket));
    if (sceSifCallRpc(&g_cdNCmdClient,
                      kNCmdFuncStream,
                      0,
                      pPacket,
                      sizeof(*pPacket),
                      g_anCdNCmdReceive,
                      sizeof(g_anCdNCmdReceive[0]),
                      NULL,
                      NULL) < 0) {
        SignalSema(g_nCdNCmdSemaId);
        return 0;
    }
    if (g_nCdDebug > 0) {
        printf("cdread end\n");
    }
    nResult = sceCdReceivedWord(g_anCdNCmdReceive);
    SignalSema(g_nCdNCmdSemaId);
    return nResult;
}

// 0x00611cc0
int sceCdStInit(unsigned int bufmax, unsigned int bankmax, unsigned int iop_bufaddr) {
    g_nCdStreamActive = 0;
    // The IOP ring buffer address travels in the packet's buffer word.
    return sceCdStream(bufmax, bankmax, (void *)iop_bufaddr, kStreamInit, &g_cdStreamDefaultMode);
}

// 0x00611cf0
int sceCdStStart(unsigned int lsn, sceCdRMode *mode) {
    g_nCdStreamActive = 1;
    return sceCdStream(lsn, 0, NULL, kStreamStart, mode);
}

// 0x00611d28
int sceCdStSeekF(unsigned int lsn) {
    return sceCdStream(lsn, 0, NULL, kStreamSeekF, &g_cdStreamDefaultMode);
}

// 0x00611d58
int sceCdStSeek(unsigned int lsn) {
    return sceCdStream(lsn, 0, NULL, kStreamSeek, &g_cdStreamDefaultMode);
}

// 0x00611d88
int sceCdStStop(void) {
    g_nCdStreamActive = 0;
    return sceCdStream(0, 0, NULL, kStreamStop, &g_cdStreamDefaultMode);
}

// 0x00611dc0
int sceCdStRead(unsigned int size, unsigned int *buf, unsigned int mode, unsigned int *err) {
    unsigned char *pBytes = (unsigned char *)buf;
    unsigned int nRead = 0;
    unsigned int nLastError = 0;
    unsigned int nReply;
    unsigned int nPass;
    unsigned int nError;

    if (g_nCdDebug > 0) {
        printf("sceCdStRead call read size= %d mode= %d\n", size, mode);
    }
    if (g_nCdStreamActive == 0) {
        return 0;
    }
    sceSifWriteBackDCache(buf, (int)(size * kSectorSize));
    if (mode == STMNBLK) {
        nReply = (unsigned int)sceCdStream(0, size, buf, kStreamRead, &g_cdStreamDefaultMode);
        *err = nReply >> kStreamErrorShift;
        return (int)(nReply & kStreamCountMask);
    }
    do {
        nReply = (unsigned int)sceCdStream(
            0, size - nRead, pBytes + nRead * kSectorSize, kStreamRead, &g_cdStreamDefaultMode);
        nPass = nReply & kStreamCountMask;
        nError = nReply >> kStreamErrorShift;
        nRead += nPass;
        if (nError != 0) {
            nLastError = nError;
            if (g_nCdDebug > 0) {
                printf("sceCdStRead BLK Read cur_size= %d read_size= %d req_size= %d err 0x%x\n",
                       nRead,
                       nPass,
                       size,
                       nError);
            }
        } else if (nPass == 0) {
            sceCdSleepTicks(kStreamRetryTicks);
        }
    } while (nRead != size && (nError == 0 || nPass != 0));
    if (g_nCdDebug > 0) {
        printf("sceCdStRead BLK Read Ended\n");
    }
    *err = nLastError;
    return (int)nRead;
}

// 0x00611f48
int sceCdStPause(void) {
    g_nCdStreamActive = 0;
    if (g_nCdDebug > 0) {
        printf("sceCdStPause call\n");
    }
    return sceCdStream(0, 0, NULL, kStreamPause, &g_cdStreamDefaultMode);
}

// 0x00611f98
int sceCdStResume(void) {
    g_nCdStreamActive = 1;
    if (g_nCdDebug > 0) {
        printf("sceCdStResume call\n");
    }
    return sceCdStream(0, 0, NULL, kStreamResume, &g_cdStreamDefaultMode);
}

// 0x00611ff0
int sceCdStStat(void) {
    if (g_nCdDebug > 0) {
        printf("sceCdStStat call\n");
    }
    return sceCdStream(0, 0, NULL, kStreamStat, &g_cdStreamDefaultMode);
}
