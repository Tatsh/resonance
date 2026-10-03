#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <eekernel.h>
#include <libmc.h>
#include <sifcmd.h>
#include <sifrpc.h>

#include "os/log.h"

enum {
    kMcServerId = 0x80000400,
};

enum {
    // The server function numbers. Rename shares the file information function.
    kMcFunctionCardInfo = 1,
    kMcFunctionOpen = 2,
    kMcFunctionClose = 3,
    kMcFunctionSeek = 4,
    kMcFunctionRead = 5,
    kMcFunctionWrite = 6,
    kMcFunctionFlush = 10,
    kMcFunctionChdir = 12,
    kMcFunctionGetDir = 13,
    kMcFunctionFileInfo = 14,
    kMcFunctionDelete = 15,
    kMcFunctionFormat = 16,
    kMcFunctionUnformat = 17,
    kMcFunctionEntSpace = 18,
    kMcFunctionChangePriority = 20,
    kMcFunctionSlotMax = 21,
    kMcFunctionInit = 254,
    kMcBindDelay = 0x100000,
    kMcSyncDelayTicks = 60,
    kMcMinimumServerVersion = 0x20a,
    kMcMinimumManagerVersion = 0x20e,
    kMcErrorUnbound = -100,
    kMcErrorRpcBias = -100,
    kMcErrorOldServer = -120,
    kMcErrorOldManager = -121,
    kMcErrorBusy = -200,
    kMcErrorBadName = -210,
    kMcNameSize = 1024,
    kMcEndDataSize = 64,
    kMcWriteInlineSize = 16,
    kMcWriteAlignMask = 15,
    kMcFileInfoValidMask = sceMcFileInfoCreate | sceMcFileInfoModify | sceMcFileInfoAttr,
    kMcFileInfoRename = 0x10,
    // The reply sizes of the initialisation call and of every other call.
    kMcInitResultSize = 12,
    kMcResultSize = 4,
};

// The server writes the buffer back through the uncached segment before the end callback runs.
typedef struct {
    union {
        struct {
            int nHeadSize;
            int nTailSize;
            unsigned char *pHead;
            unsigned char *pTail;
            unsigned char abHead[kMcEndDataSize];
            unsigned char abTail[kMcEndDataSize];
        } read;
        struct {
            int nType;
            int nFree;
        } info;
    };
    int nFormat; // Written by the card information call only.
} __attribute__((aligned(64))) McEndData;

// The packet of the calls that address a descriptor or a card. The file information calls reuse
// the fields as flags.
typedef struct {
    int nFd;
    int nPort;
    int nSlot;
    int nSize;   // sceMcGetInfo() sets it when the format state is wanted.
    int nOffset; // sceMcGetInfo() sets it when the free space is wanted.
    int nOrigin; // The inline byte count of a write, the priority, or the card type request.
    const unsigned char *pBuffer;
    McEndData *pEndData;
    unsigned char abData[kMcWriteInlineSize];
} McDescPacket;

// The packet of the calls that address a path.
typedef struct {
    int nPort;
    int nSlot;
    int nFlags;
    int nMaxEntries;
    union {
        sceMcTblGetDir *pTable;
        char *pszDirectory;
    };
    char szName[kMcNameSize];
} McNamePacket;

// NTSC-U/C: 0x00761730, PAL: 0x007a4660
static int g_nMcCommand = 0;
// NTSC-U/C: 0x00761734, PAL: 0x007a4664
static int g_nMcSemaId = -1;

// NTSC-U/C: 0x008e0180, PAL: 0x00925140
static sceSifClientData g_mcClient __attribute__((aligned(64)));
// NTSC-U/C: 0x008e01a8, PAL: 0x00925168
static int *g_pnMcInfoType;
// NTSC-U/C: 0x008e01ac, PAL: 0x0092516c
static int *g_pnMcInfoFree;
// NTSC-U/C: 0x008e01b0, PAL: 0x00925170
static int *g_pnMcInfoFormat;
// SIF DMA moves whole quadwords. The buffers below retain retail's placement (64-byte boundaries,
// and a 16-byte boundary for the path packet).
// NTSC-U/C: 0x008e01c0, PAL: 0x00925180
static sceMcTblGetDir g_mcFileInfo __attribute__((aligned(64)));
// NTSC-U/C: 0x008e0200, PAL: 0x009251c0
static McDescPacket g_mcDescPacket __attribute__((aligned(64)));
// NTSC-U/C: 0x008e0230, PAL: 0x009251f0
static McNamePacket g_mcNamePacket __attribute__((aligned(16)));
// NTSC-U/C: 0x008e0680, PAL: 0x00925640
static McEndData g_mcEndData __attribute__((aligned(64)));
// NTSC-U/C: 0x008e0740, PAL: 0x00925700
static char g_szMcCurrentDir[kMcNameSize] __attribute__((aligned(64)));
// NTSC-U/C: 0x008e1740, PAL: 0x00926700
static sceMcRpcResult g_mcResult __attribute__((aligned(64)));

// NTSC-U/C: 0x005659e8, PAL: 0x005a4158
int sceMcInitLibrary(void) {
    struct SemaParam param;
    int nResult;
    int i;

    if (g_nMcSemaId < 0) {
        param.maxCount = 1;
        param.initCount = 1;
        param.option = 0;
        g_nMcSemaId = CreateSema(&param);
    }
    sceMcSync(sceMcWait, NULL, NULL);
    WaitSema(g_nMcSemaId);
    sceSifInitRpc(0);
    for (;;) {
        if (sceSifBindRpc(&g_mcClient, kMcServerId, 0) < 0) {
            LogPrintf("bind error libmc \n");
            for (;;) {
            }
        }
        if (g_mcClient.serve != NULL) {
            break;
        }
        for (i = kMcBindDelay; i != 0; --i) {
            __asm__ volatile("nop");
        }
    }

    nResult = sceSifCallRpc(&g_mcClient,
                            kMcFunctionInit,
                            0,
                            &g_mcDescPacket,
                            sizeof(g_mcDescPacket),
                            &g_mcResult,
                            kMcInitResultSize,
                            NULL,
                            NULL);
    SignalSema(g_nMcSemaId);
    if (nResult < 0) {
        g_mcClient.serve = NULL;
        return nResult + kMcErrorRpcBias;
    }
    if (g_mcResult.nServerVersion < kMcMinimumServerVersion) {
        LogPrintf("libmc: too old release of mcserv.irx\n");
        g_mcClient.serve = NULL;
        return kMcErrorOldServer;
    }
    if (g_mcResult.nManagerVersion < kMcMinimumManagerVersion) {
        LogPrintf("libmc: too old release of mcman.irx\n");
        g_mcClient.serve = NULL;
        return kMcErrorOldManager;
    }
    return g_mcResult.nResult;
}

// NTSC-U/C: 0x00565ba0, PAL: 0x005a4310
sceSifClientData *sceMcGetRpcState(sceMcRpcResult **ppResult, int **ppnCommand) {
    *ppResult = &g_mcResult;
    *ppnCommand = &g_nMcCommand;
    g_mcResult.nSemaId = g_nMcSemaId;
    return &g_mcClient;
}

// Reserve the library for a command. Returns zero, or the error the entry point returns.
static inline int McBeginCommand(void) {
    if (PollSema(g_nMcSemaId) < 0) {
        return kMcErrorBusy;
    }
    if (g_mcClient.serve == NULL) {
        SignalSema(g_nMcSemaId);
        return kMcErrorUnbound;
    }
    return 0;
}

// Record the command a sent call started, or release the library when the call failed.
static inline int McEndCommand(int nResult, int nCommand) {
    if (nResult != 0) {
        SignalSema(g_nMcSemaId);
    } else {
        g_nMcCommand = nCommand;
    }
    return nResult;
}

// Send the descriptor packet without waiting for the reply.
static inline int McCallDesc(int nFunction, sceSifEndFunc pfnEnd, void *pEndParameter) {
    return sceSifCallRpc(&g_mcClient,
                         nFunction,
                         SIF_RPCM_NOWAIT,
                         &g_mcDescPacket,
                         sizeof(g_mcDescPacket),
                         &g_mcResult,
                         kMcResultSize,
                         pfnEnd,
                         pEndParameter);
}

// Send the path packet without waiting for the reply.
static inline int McCallName(int nFunction, sceSifEndFunc pfnEnd, void *pEndParameter) {
    return sceSifCallRpc(&g_mcClient,
                         nFunction,
                         SIF_RPCM_NOWAIT,
                         &g_mcNamePacket,
                         sizeof(g_mcNamePacket),
                         &g_mcResult,
                         kMcResultSize,
                         pfnEnd,
                         pEndParameter);
}

static inline int McIsBadName(const char *pszName) {
    return pszName == NULL || pszName[0] == '\0';
}

static inline void McSetName(const char *pszName) {
    strncpy(g_mcNamePacket.szName, pszName, kMcNameSize - 1);
    g_mcNamePacket.szName[kMcNameSize - 1] = '\0';
}

// NTSC-U/C: 0x00565bd0, PAL: 0x005a4340
int sceMcChangeThreadPriority(int nLevel) {
    int nResult;

    if ((nResult = McBeginCommand()) != 0) {
        return nResult;
    }
    g_mcDescPacket.nOrigin = nLevel;
    return McEndCommand(McCallDesc(kMcFunctionChangePriority, NULL, NULL), sceMcFuncNoChgPrior);
}

// NTSC-U/C: 0x00565c88, PAL: 0x005a43f8
int sceMcGetSlotMax(int nPort) {
    int nResult;

    if ((nResult = McBeginCommand()) != 0) {
        return nResult;
    }
    g_mcDescPacket.nPort = nPort;
    nResult = sceSifCallRpc(&g_mcClient,
                            kMcFunctionSlotMax,
                            0,
                            &g_mcDescPacket,
                            sizeof(g_mcDescPacket),
                            &g_mcResult,
                            kMcResultSize,
                            NULL,
                            NULL);
    SignalSema(g_nMcSemaId);
    if (nResult != 0) {
        return nResult;
    }
    return g_mcResult.nResult;
}

// NTSC-U/C: 0x00565d48, PAL: 0x005a44b8
int sceMcOpen(int nPort, int nSlot, const char *pszName, int nMode) {
    int nResult;

    if ((nResult = McBeginCommand()) != 0) {
        return nResult;
    }
    if (McIsBadName(pszName)) {
        SignalSema(g_nMcSemaId);
        return kMcErrorBadName;
    }
    McSetName(pszName);
    g_mcNamePacket.nPort = nPort;
    g_mcNamePacket.nFlags = nMode;
    g_mcNamePacket.nSlot = nSlot;
    return McEndCommand(McCallName(kMcFunctionOpen, NULL, NULL), sceMcFuncNoOpen);
}

// NTSC-U/C: 0x00565e80, PAL: 0x005a45f0
int sceMcMkDir(int nPort, int nSlot, const char *pszName) {
    const int nResult = sceMcOpen(nPort, nSlot, pszName, sceMcFileCreateDir);

    if (nResult == 0) {
        g_nMcCommand = sceMcFuncNoMkdir;
    }
    return nResult;
}

// NTSC-U/C: 0x00565eb8, PAL: 0x005a4628
int sceMcClose(int nFd) {
    int nResult;

    if ((nResult = McBeginCommand()) != 0) {
        return nResult;
    }
    g_mcDescPacket.nFd = nFd;
    return McEndCommand(McCallDesc(kMcFunctionClose, NULL, NULL), sceMcFuncNoClose);
}

// NTSC-U/C: 0x00565f70, PAL: 0x005a46e0
int sceMcSeek(int nFd, int nOffset, int nMode) {
    int nResult;

    if ((nResult = McBeginCommand()) != 0) {
        return nResult;
    }
    g_mcDescPacket.nFd = nFd;
    g_mcDescPacket.nOffset = nOffset;
    g_mcDescPacket.nOrigin = nMode;
    return McEndCommand(McCallDesc(kMcFunctionSeek, NULL, NULL), sceMcFuncNoSeek);
}

// NTSC-U/C: 0x00566048, PAL: 0x005a47b8
static void McReadEnd(void *pParameter) {
    const McEndData *pEnd = UNCACHED_SEG(pParameter);
    int i;

    for (i = 0; i < pEnd->read.nHeadSize; ++i) {
        pEnd->read.pHead[i] = pEnd->read.abHead[i];
    }
    for (i = 0; i < pEnd->read.nTailSize; ++i) {
        pEnd->read.pTail[i] = pEnd->read.abTail[i];
    }
}

// NTSC-U/C: 0x005660d8, PAL: 0x005a4848
int sceMcRead(int nFd, void *pBuffer, int nSize) {
    int nResult;

    if ((nResult = McBeginCommand()) != 0) {
        return nResult;
    }
    g_mcDescPacket.nFd = nFd;
    g_mcDescPacket.pEndData = &g_mcEndData;
    g_mcDescPacket.pBuffer = pBuffer;
    g_mcDescPacket.nSize = nSize;
    sceSifWriteBackDCache(pBuffer, nSize);
    sceSifWriteBackDCache(&g_mcEndData, sizeof(g_mcEndData));
    return McEndCommand(McCallDesc(kMcFunctionRead, McReadEnd, &g_mcEndData), sceMcFuncNoRead);
}

// NTSC-U/C: 0x005661f8, PAL: 0x005a4968
int sceMcWrite(int nFd, const void *pBuffer, int nSize) {
    const unsigned char *pbSource = pBuffer;
    unsigned int nHead;
    unsigned int i;
    int nResult;

    if ((nResult = McBeginCommand()) != 0) {
        return nResult;
    }
    g_mcDescPacket.nFd = nFd;
    if (nSize <= kMcWriteInlineSize) {
        g_mcDescPacket.nOrigin = nSize;
        g_mcDescPacket.pBuffer = NULL;
        g_mcDescPacket.nSize = 0;
    } else {
        // The bytes before the next 16-byte boundary go inline, and the server reads the rest by
        // DMA.
        nHead = (((uintptr_t)pbSource - 1) & ~kMcWriteAlignMask) -
                ((uintptr_t)pbSource - kMcWriteInlineSize);
        g_mcDescPacket.pBuffer = &pbSource[nHead];
        g_mcDescPacket.nSize = nSize - nHead;
        g_mcDescPacket.nOrigin = nHead;
    }
    for (i = 0; i < (unsigned int)g_mcDescPacket.nOrigin; ++i) {
        g_mcDescPacket.abData[i] = pbSource[i];
    }
    FlushCache(WRITEBACK_DCACHE);
    return McEndCommand(McCallDesc(kMcFunctionWrite, NULL, NULL), sceMcFuncNoWrite);
}

// NTSC-U/C: 0x00566378, PAL: 0x005a4ae8
static void McAlarmHandler(int nId, unsigned short nTime, void *pThread) {
    (void)nId;
    (void)nTime;
    iWakeupThread((int)(intptr_t)pThread);
    ExitHandler();
}

// NTSC-U/C: 0x005663a0, PAL: 0x005a4b10
static void McDelayThread(unsigned short nTicks) {
    SetAlarm(nTicks, McAlarmHandler, (void *)(intptr_t)GetThreadId());
    SleepThread();
}

// NTSC-U/C: 0x005663e8, PAL: 0x005a4b58
int sceMcSync(int nMode, int *pnCommand, int *pnResult) {
    int nBusy;

    if (g_nMcCommand == 0) {
        return sceMcExecIdle;
    }
    nBusy = sceSifCheckStatRpc(&g_mcClient.rpcd);
    if (nMode == sceMcWait && nBusy != 0) {
        while (sceSifCheckStatRpc(&g_mcClient.rpcd) != 0) {
            McDelayThread(kMcSyncDelayTicks);
        }
        nBusy = 0;
    }
    if (pnCommand != NULL) {
        *pnCommand = g_nMcCommand;
    }
    if (nBusy != 0) {
        return sceMcExecRun;
    }
    g_nMcCommand = 0;
    if (pnResult != NULL) {
        *pnResult = g_mcResult.nResult;
    }
    SignalSema(g_nMcSemaId);
    return sceMcExecFinish;
}

// NTSC-U/C: 0x005664c8, PAL: 0x005a4c38
static void McGetInfoEnd(void *pParameter) {
    const McEndData *pEnd = UNCACHED_SEG(pParameter);

    if (g_pnMcInfoType != NULL) {
        *g_pnMcInfoType = pEnd->info.nType;
    }
    if (g_pnMcInfoFree != NULL) {
        *g_pnMcInfoFree = pEnd->info.nFree;
    }
    if (g_pnMcInfoFormat != NULL) {
        *g_pnMcInfoFormat = pEnd->nFormat;
    }
}

// NTSC-U/C: 0x00566520, PAL: 0x005a4c90
int sceMcGetInfo(int nPort, int nSlot, int *pnType, int *pnFree, int *pnFormat) {
    int nResult;

    if ((nResult = McBeginCommand()) != 0) {
        return nResult;
    }
    g_mcDescPacket.nPort = nPort;
    g_mcDescPacket.nSlot = nSlot;
    g_mcDescPacket.pEndData = &g_mcEndData;
    g_mcDescPacket.nOrigin = pnType != NULL;
    g_mcDescPacket.nOffset = pnFree != NULL;
    g_mcDescPacket.nSize = pnFormat != NULL;
    g_pnMcInfoType = pnType;
    g_pnMcInfoFree = pnFree;
    g_pnMcInfoFormat = pnFormat;
    sceSifWriteBackDCache(&g_mcEndData, sizeof(g_mcEndData));
    return McEndCommand(McCallDesc(kMcFunctionCardInfo, McGetInfoEnd, &g_mcEndData),
                        sceMcFuncNoCardInfo);
}

// NTSC-U/C: 0x005666a8, PAL: 0x005a4e18
int sceMcGetDir(int nPort,
                int nSlot,
                const char *pszName,
                unsigned int nMode,
                int nMaxEntries,
                sceMcTblGetDir *pTable) {
    int nResult;

    if ((nResult = McBeginCommand()) != 0) {
        return nResult;
    }
    if (McIsBadName(pszName)) {
        SignalSema(g_nMcSemaId);
        return kMcErrorBadName;
    }
    g_mcNamePacket.nPort = nPort;
    g_mcNamePacket.nSlot = nSlot;
    g_mcNamePacket.nFlags = nMode;
    g_mcNamePacket.nMaxEntries = nMaxEntries;
    g_mcNamePacket.pTable = pTable;
    McSetName(pszName);
    if (nMaxEntries >= 0) {
        sceSifWriteBackDCache(pTable, nMaxEntries * sizeof(sceMcTblGetDir));
    }
    return McEndCommand(McCallName(kMcFunctionGetDir, NULL, NULL), sceMcFuncNoGetDir);
}

// NTSC-U/C: 0x00566800, PAL: 0x005a4f70
static void McChdirEnd(void *pParameter) {
    char *pszDest = pParameter;
    const char *pszDirectory;
    size_t nLength;

    if (pszDest == NULL) {
        return;
    }
    pszDirectory = UNCACHED_SEG(g_szMcCurrentDir);
    nLength = strlen(pszDirectory) < kMcNameSize ? strlen(pszDirectory) : kMcNameSize - 1;
    memcpy(pszDest, pszDirectory, nLength);
    pszDest[nLength] = '\0';
}

// NTSC-U/C: 0x00566888, PAL: 0x005a4ff8
int sceMcChdir(int nPort, int nSlot, const char *pszNewDir, char *pszCurrentDir) {
    int nResult;

    if ((nResult = McBeginCommand()) != 0) {
        return nResult;
    }
    if (McIsBadName(pszNewDir)) {
        SignalSema(g_nMcSemaId);
        return kMcErrorBadName;
    }
    g_mcNamePacket.nPort = nPort;
    g_mcNamePacket.pszDirectory = g_szMcCurrentDir;
    g_mcNamePacket.nSlot = nSlot;
    McSetName(pszNewDir);
    sceSifWriteBackDCache(g_szMcCurrentDir, sizeof(g_szMcCurrentDir));
    return McEndCommand(McCallName(kMcFunctionChdir, McChdirEnd, pszCurrentDir), sceMcFuncNoChDir);
}

// NTSC-U/C: 0x005669d8, PAL: 0x005a5148
int sceMcFormat(int nPort, int nSlot) {
    int nResult;

    if ((nResult = McBeginCommand()) != 0) {
        return nResult;
    }
    g_mcDescPacket.nSlot = nSlot;
    g_mcDescPacket.nPort = nPort;
    return McEndCommand(McCallDesc(kMcFunctionFormat, NULL, NULL), sceMcFuncNoFormat);
}

// NTSC-U/C: 0x00566aa0, PAL: 0x005a5210
int sceMcDelete(int nPort, int nSlot, const char *pszName) {
    int nResult;

    if ((nResult = McBeginCommand()) != 0) {
        return nResult;
    }
    if (McIsBadName(pszName)) {
        SignalSema(g_nMcSemaId);
        return kMcErrorBadName;
    }
    McSetName(pszName);
    g_mcNamePacket.nPort = nPort;
    g_mcNamePacket.nSlot = nSlot;
    g_mcNamePacket.nFlags = 0;
    return McEndCommand(McCallName(kMcFunctionDelete, NULL, NULL), sceMcFuncNoDelete);
}

// NTSC-U/C: 0x00566bc8, PAL: 0x005a5338
int sceMcFlush(int nFd) {
    int nResult;

    if ((nResult = McBeginCommand()) != 0) {
        return nResult;
    }
    g_mcDescPacket.nFd = nFd;
    return McEndCommand(McCallDesc(kMcFunctionFlush, NULL, NULL), sceMcFuncNoFlush);
}

// NTSC-U/C: 0x00566c80, PAL: 0x005a53f0
int sceMcSetFileInfo(
    int nPort, int nSlot, const char *pszName, const sceMcTblGetDir *pInfo, unsigned int nValid) {
    int nResult;

    if ((nResult = McBeginCommand()) != 0) {
        return nResult;
    }
    if (McIsBadName(pszName)) {
        SignalSema(g_nMcSemaId);
        return kMcErrorBadName;
    }
    g_mcNamePacket.nPort = nPort;
    g_mcNamePacket.nSlot = nSlot;
    g_mcNamePacket.nFlags = nValid & kMcFileInfoValidMask;
    g_mcFileInfo = *pInfo;
    g_mcNamePacket.pTable = &g_mcFileInfo;
    McSetName(pszName);
    FlushCache(WRITEBACK_DCACHE);
    return McEndCommand(McCallName(kMcFunctionFileInfo, NULL, NULL), sceMcFuncNoFileInfo);
}

// NTSC-U/C: 0x00566e58, PAL: 0x005a55c8
int sceMcRename(int nPort, int nSlot, const char *pszName, const char *pszNewName) {
    int nResult;

    if ((nResult = McBeginCommand()) != 0) {
        return nResult;
    }
    if (McIsBadName(pszName) || pszNewName == NULL) {
        SignalSema(g_nMcSemaId);
        return kMcErrorBadName;
    }
    g_mcNamePacket.nPort = nPort;
    g_mcNamePacket.nSlot = nSlot;
    g_mcNamePacket.nFlags = kMcFileInfoRename;
    McSetName(pszName);
    strncpy((char *)g_mcFileInfo.EntryName, pszNewName, sizeof(g_mcFileInfo.EntryName) - 1);
    g_mcNamePacket.pTable = &g_mcFileInfo;
    g_mcFileInfo.EntryName[sizeof(g_mcFileInfo.EntryName) - 1] = '\0';
    FlushCache(WRITEBACK_DCACHE);
    return McEndCommand(McCallName(kMcFunctionFileInfo, NULL, NULL), sceMcFuncNoRename);
}

// NTSC-U/C: 0x00566fc0, PAL: 0x005a5730
int sceMcUnformat(int nPort, int nSlot) {
    int nResult;

    if ((nResult = McBeginCommand()) != 0) {
        return nResult;
    }
    g_mcDescPacket.nSlot = nSlot;
    g_mcDescPacket.nPort = nPort;
    return McEndCommand(McCallDesc(kMcFunctionUnformat, NULL, NULL), sceMcFuncNoUnformat);
}

// NTSC-U/C: 0x00567088, PAL: 0x005a57f8
int sceMcGetEntSpace(int nPort, int nSlot, const char *pszPath) {
    int nResult;

    if ((nResult = McBeginCommand()) != 0) {
        return nResult;
    }
    if (McIsBadName(pszPath)) {
        SignalSema(g_nMcSemaId);
        return kMcErrorBadName;
    }
    g_mcNamePacket.nPort = nPort;
    g_mcNamePacket.nSlot = nSlot;
    McSetName(pszPath);
    return McEndCommand(McCallName(kMcFunctionEntSpace, NULL, NULL), sceMcFuncNoEntSpace);
}
