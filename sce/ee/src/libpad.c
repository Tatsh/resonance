#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <eekernel.h>
#include <libpad.h>
#include <sifdev.h>
#include <sifrpc.h>

#include "os/log.h"

enum {
    kPadServerId = 0x80000100,
};

enum {
    kPadClientCount = 2,
    kPadPortCount = 2,
    kPadSlotCount = 4,
    kPadFrameCount = 2,
    kPadBindDelay = 0x10000,
    kPadModVersionMajor = 4,
    kPadModVersionMinor = 0,
    kPadVersionShift = 8,
    kPadVersionMinorMask = 0xff,
    kPadFrameAlignMask = 63,
    kPadReportFill = 0xff,
    // The server's commands, all sent to function 1 of the first server.
    kPadRpcFunction = 1,
    kPadCommandOpen = 1,
    kPadCommandSetMainMode = 6,
    kPadCommandSetActAlign = 8,
    kPadCommandSetButtonInfo = 10,
    kPadCommandSetVrefParam = 11,
    kPadCommandGetPortMax = 12,
    kPadCommandGetSlotMax = 13,
    kPadCommandClose = 14,
    kPadCommandEnd = 15,
    kPadCommandInit = 16,
    kPadCommandGetModVersion = 18,
    kPadCommandSetWarningLevel = 20,
    kPadCloseMode = 1,
    kPadResultAccepted = 1,
    kPadEndStopped = 1,
    kPadDirectCommandActuators = 1,
    kPadActuatorByteCount = 6,
    kPadVrefByteCount = 12,
    // nCurrentTask once padman has read the mode and actuator tables.
    kPadTaskReady = 1,
    // nModeConfig when the controller has no mode table, and the least value with actuators.
    kPadModeConfigNone = 1,
    kPadModeConfigActuators = 2,
    // The least nModel of a controller with pressure-sensitive buttons.
    kPadModelPressure = 2,
    // nModeCurrentId of a controller in configuration mode.
    kPadModeCurrentIdConfig = 0xf3,
    kPadModeIdShift = 4,
    kPadPressureMaskAll = 0x3ffff,
    kPadPressureButtonsAll = 0xfff,
    kPadPressureButtonsNone = 0,
    kPadInfoIndexCount = -1,
    kBitsPerByte = 8,
};

// The packet the library writes to the IOP by DMA, in turn to one of two buffers there.
typedef struct {
    int nCount;
    int nCommand;
    int nSize;
    unsigned char abData[20];
} PadDirectPacket;

typedef struct {
    PadDirectPacket packet;
    unsigned char abPadding[sizeof(PadDirectPacket)];
} __attribute__((aligned(64))) PadDirectBlock;

typedef struct {
    scePadDmaFrame *pFrames;
    PadDirectBlock *pDirect;
    unsigned int nIopAddress; // The pair of direct packet buffers on the IOP.
    unsigned int nDmaId;
    int nOpen;
    int nUnusedFirst;  // +0x14, cleared by scePadInit2() and never read.
    int nUnusedSecond; // +0x18, cleared by scePadInit2() and never read.
} PadPortState;

// The server reads its arguments from and writes its reply to the same 128 bytes.
typedef struct {
    int nCommand;
    int nPort; // Also the level scePadSetWarningLevel() sends.
    int nSlot; // Also the result scePadSetWarningLevel() receives.
    union {
        struct {
            int nResult;
            int nMode;
        } status;
        struct {
            int nResult;
            scePadDmaFrame *pFrames;
            unsigned int nIopAddress;
        } open;
        struct {
            int nOffset;
            int nLock;
            int nResult;
        } mainMode;
        struct {
            unsigned char abAlign[8];
            int nResult;
        } actAlign;
        struct {
            int nMask;
            int nResult;
        } buttonInfo;
        struct {
            unsigned char abParam[kPadVrefByteCount];
            int nReserved;
            int nResult;
        } vref;
        unsigned char abRaw[116];
    };
} __attribute__((aligned(64))) PadRpcBuffer;

// NTSC-U/C: 0x00770298, PAL: 0x007a46a0
static int isInit = 0;
// NTSC-U/C: 0x0077029c, PAL: 0x007a46a4
static int isWarning = 1;

// NTSC-U/C: 0x007702a0, PAL: 0x007a46a8
static const char *const g_apszPadStateNames[] = {
    "DISCONNECT", "", "FINDCTP1", "", "", "EXECCMD", "STABLE", "ERROR"};
// NTSC-U/C: 0x007702c0, PAL: 0x007a46c8
static const char *const g_apszPadReqStateNames[] = {
    "COMPLETE", "FAILED", "BUSY", NULL}; // Yes, the binary's table ends in a null entry.

// NTSC-U/C: 0x008e4280, PAL: 0x00926740
static sceSifClientData g_aPadClients[kPadClientCount] __attribute__((aligned(64)));
// NTSC-U/C: 0x008e42d0, PAL: 0x00926790
static PadPortState g_aaPadPorts[kPadPortCount][kPadSlotCount];
// SIF DMA moves whole quadwords. The two buffers below retain retail's 64-byte placement.
// NTSC-U/C: 0x008e43c0, PAL: 0x00926880
static PadDirectBlock g_aaPadDirect[kPadPortCount][kPadSlotCount] __attribute__((aligned(64)));
// NTSC-U/C: 0x008e45c0, PAL: 0x00926a80
static PadRpcBuffer g_padRpc __attribute__((aligned(64)));

static inline int PadCall(void) {
    return sceSifCallRpc(&g_aPadClients[0],
                         kPadRpcFunction,
                         0,
                         &g_padRpc,
                         sizeof(g_padRpc),
                         &g_padRpc,
                         sizeof(g_padRpc),
                         NULL,
                         NULL);
}

// Mark the request busy when padman accepted it, and return padman's result.
static inline int PadAccepted(int nPort, int nSlot, const int *pnResult) {
    if (*pnResult == kPadResultAccepted) {
        scePadSetReqState(nPort, nSlot, scePadReqStateBusy);
    }
    return *pnResult;
}

// NTSC-U/C: 0x0059bfd8, PAL: 0x005a62c0
static void _send_to_iop(int nPort, int nSlot) {
    PadPortState *pState = &g_aaPadPorts[nPort][nSlot];
    PadDirectBlock *pDirect = pState->pDirect;
    sceSifDmaData dma;
    unsigned int nDmaId;

    if (sceSifDmaStat(pState->nDmaId) >= 0) {
        if (isWarning) {
            LogPrintf("libpad: tPadDma Structure Invalid\n");
        }
        return;
    }
    ++pDirect->packet.nCount;
    SyncDCache(&pDirect->packet, &pDirect->abPadding[0]);
    dma.data = (uintptr_t)&pDirect->packet;
    dma.addr = pState->nIopAddress + (pDirect->packet.nCount & 1) * sizeof(PadDirectPacket);
    dma.size = sizeof(PadDirectPacket);
    dma.mode = 0;
    nDmaId = sceSifSetDma(&dma, 1);
    if (nDmaId == 0 && isWarning) {
        LogPrintf("libpad: tPadDma Structure Invalid\n");
    }
    pState->nDmaId = nDmaId;
}

// NTSC-U/C: 0x0059c108, PAL: 0x005a63f0
int scePadInit(int nMode) {
    int nVersion;
    int i;
    int k;

    isInit = 1;
    for (k = 0; k < kPadClientCount; ++k) {
        for (;;) {
            sceSifBindRpc(&g_aPadClients[k], kPadServerId + k, 0);
            if (g_aPadClients[k].serve != NULL) {
                break;
            }
            for (i = kPadBindDelay; i != -1; --i) {
                __asm__ volatile("nop");
            }
        }
    }

    nVersion = scePadGetModVersion();
    if ((nVersion >> kPadVersionShift) != kPadModVersionMajor) {
        if (isWarning) {
            LogPrintf("libpad: Module version mismatch ");
            LogPrintf("[libpad.a = %d.%d, padman.irx = %d.%d]\n",
                   kPadModVersionMajor,
                   kPadModVersionMinor,
                   nVersion >> kPadVersionShift,
                   nVersion & kPadVersionMinorMask);
        }
        return 0;
    }
    return scePadInit2(nMode);
}

// NTSC-U/C: 0x0059c248, PAL: 0x005a6530
int scePadInit2(int nMode) {
    int nPort;
    int nSlot;

    (void)nMode;
    for (nSlot = 0; nSlot < kPadSlotCount; ++nSlot) {
        for (nPort = 0; nPort < kPadPortCount; ++nPort) {
            g_aaPadPorts[nPort][nSlot].nOpen = 0;
            g_aaPadPorts[nPort][nSlot].nUnusedSecond = 0;
            g_aaPadPorts[nPort][nSlot].nUnusedFirst = 0;
        }
    }
    g_padRpc.nCommand = kPadCommandInit;
    g_padRpc.status.nMode = 0;
    if (PadCall() < 0) {
        return 0;
    }
    return g_padRpc.status.nResult;
}

// NTSC-U/C: 0x0059c2e8, PAL: 0x005a65d0
int scePadEnd(void) {
    g_padRpc.nCommand = kPadCommandEnd;
    if (PadCall() < 0) {
        return 0;
    }
    if (g_padRpc.status.nResult == kPadEndStopped) {
        isInit = 0;
    }
    return g_padRpc.status.nResult;
}

// NTSC-U/C: 0x0059c368, PAL: 0x005a6650
int scePadPortOpen(int nPort, int nSlot, scePadDmaFrame *pFrames) {
    PadPortState *pState;
    int i;

    if (((uintptr_t)pFrames & kPadFrameAlignMask) != 0) {
        if (isWarning) {
            LogPrintf("libpad: buffer addr is not 64 byte align. %08x\n",
                   (unsigned int)(uintptr_t)pFrames);
        }
        return 0;
    }
    pState = &g_aaPadPorts[nPort][nSlot];
    if (pState->nOpen == 1) {
        if (isWarning) {
            LogPrintf("libpad: pad port is already open [%d][%d]\n", nPort, nSlot);
        }
        return 0;
    }
    for (i = 0; i < kPadFrameCount; ++i) {
        pFrames[i].nFrame = 0;
        pFrames[i].nState = scePadStateExecCmd;
        pFrames[i].nReqState = scePadReqStateBusy;
        pFrames[i].nReportReady = 0;
        memset(pFrames[i].abData, kPadReportFill, sizeof(pFrames[i].abData));
        pFrames[i].nLength = 0;
    }

    g_padRpc.nCommand = kPadCommandOpen;
    g_padRpc.nPort = nPort;
    g_padRpc.nSlot = nSlot;
    g_padRpc.open.pFrames = pFrames;
    if (PadCall() < 0) {
        return 0;
    }
    pState->nOpen = 1;
    pState->nDmaId = 0;
    pState->nIopAddress = g_padRpc.open.nIopAddress;
    pState->pFrames = pFrames;
    pState->pDirect = &g_aaPadDirect[nPort][nSlot];
    g_aaPadDirect[nPort][nSlot].packet.nCount = 0;
    return g_padRpc.open.nResult;
}

// NTSC-U/C: 0x0059c550, PAL: 0x005a6838
int scePadPortClose(int nPort, int nSlot) {
    PadPortState *pState = &g_aaPadPorts[nPort][nSlot];

    if (pState->nOpen == 0) {
        return 0;
    }
    g_padRpc.nCommand = kPadCommandClose;
    g_padRpc.nPort = nPort;
    g_padRpc.nSlot = nSlot;
    g_padRpc.status.nMode = kPadCloseMode;
    if (PadCall() < 0) {
        return 0;
    }
    pState->nOpen = 0;
    return g_padRpc.status.nResult;
}

// NTSC-U/C: 0x0059c608, PAL: 0x005a68f0
scePadDmaFrame *scePadGetDmaStr(int nPort, int nSlot) {
    scePadDmaFrame *pFrames = g_aaPadPorts[nPort][nSlot].pFrames;

    SyncDCache(pFrames, &pFrames[kPadFrameCount]);
    return &pFrames[pFrames[0].nFrame < pFrames[1].nFrame];
}

// NTSC-U/C: 0x0059c668, PAL: 0x005a6950
unsigned int scePadGetFrameCount(int nPort, int nSlot) {
    if (g_aaPadPorts[nPort][nSlot].nOpen == 0) {
        return 0;
    }
    return scePadGetDmaStr(nPort, nSlot)->nFrame;
}

// NTSC-U/C: 0x0059c6b8, PAL: 0x005a69a0
int scePadRead(int nPort, int nSlot, unsigned char *pData) {
    const scePadDmaFrame *pFrame;

    if (g_aaPadPorts[nPort][nSlot].nOpen == 0) {
        return 0;
    }
    pFrame = scePadGetDmaStr(nPort, nSlot);
    memcpy(pData, pFrame->abData, pFrame->nLength);
    return pFrame->nLength;
}

// NTSC-U/C: 0x0059c738, PAL: 0x005a6a20
int scePadGetState(int nPort, int nSlot) {
    const scePadDmaFrame *pFrame;

    if (g_aaPadPorts[nPort][nSlot].nOpen == 0) {
        return scePadStateClosed;
    }
    pFrame = scePadGetDmaStr(nPort, nSlot);
    if (pFrame->nState == scePadStateStable && pFrame->nReqState == scePadReqStateBusy) {
        return scePadStateExecCmd;
    }
    return pFrame->nState;
}

// NTSC-U/C: 0x0059c7b0, PAL: 0x005a6a98
void scePadStateIntToStr(int nState, char *pszName) {
    if ((unsigned int)nState < sizeof(g_apszPadStateNames) / sizeof(g_apszPadStateNames[0])) {
        strcpy(pszName, g_apszPadStateNames[nState]);
        return;
    }
    pszName[0] = '\0';
}

// NTSC-U/C: 0x0059c7e8, PAL: 0x005a6ad0
int scePadSetReqState(int nPort, int nSlot, int nState) {
    if (g_aaPadPorts[nPort][nSlot].nOpen == 0) {
        return 0;
    }
    scePadGetDmaStr(nPort, nSlot)->nReqState = nState;
    return 1;
}

// NTSC-U/C: 0x0059c850, PAL: 0x005a6b38
int scePadGetReqState(int nPort, int nSlot) {
    if (g_aaPadPorts[nPort][nSlot].nOpen == 0) {
        return 0;
    }
    return scePadGetDmaStr(nPort, nSlot)->nReqState;
}

// NTSC-U/C: 0x0059c8a0, PAL: 0x005a6b88
void scePadReqIntToStr(int nState, char *pszName) {
    if ((unsigned int)nState < sizeof(g_apszPadReqStateNames) / sizeof(g_apszPadReqStateNames[0])) {
        strcpy(pszName, g_apszPadReqStateNames[nState]);
        return;
    }
    pszName[0] = '\0';
}

// The frame of an open port whose actuator tables padman has read, or null.
static inline const scePadDmaFrame *PadActuatorFrame(int nPort, int nSlot) {
    const scePadDmaFrame *pFrame;

    if (g_aaPadPorts[nPort][nSlot].nOpen == 0) {
        return NULL;
    }
    pFrame = scePadGetDmaStr(nPort, nSlot);
    if (pFrame->nCurrentTask != kPadTaskReady || pFrame->nModeConfig < kPadModeConfigActuators) {
        return NULL;
    }
    return pFrame;
}

// NTSC-U/C: 0x0059c8d8, PAL: 0x005a6bc0
int scePadInfoAct(int nPort, int nSlot, int nActuator, int nTerm) {
    const scePadDmaFrame *pFrame = PadActuatorFrame(nPort, nSlot);

    if (pFrame == NULL || nActuator >= pFrame->nActuatorCount) {
        return 0;
    }
    if (nActuator == kPadInfoIndexCount) {
        return pFrame->nActuatorCount;
    }
    switch (nTerm) {
    case InfoActFunc:
    case InfoActSub:
    case InfoActSize:
    case InfoActCurr:
        return pFrame->aabActInfo[nActuator][nTerm - InfoActFunc];
    default:
        return 0;
    }
}

// NTSC-U/C: 0x0059c9f8, PAL: 0x005a6ce0
int scePadInfoComb(int nPort, int nSlot, int nList, int nOffset) {
    const scePadDmaFrame *pFrame = PadActuatorFrame(nPort, nSlot);

    if (pFrame == NULL) {
        return 0;
    }
    if (nList == kPadInfoIndexCount) {
        return pFrame->nCombinationCount;
    }
    if (nList >= pFrame->nCombinationCount) {
        return 0;
    }
    // Byte 0 of an entry is its member count, and the members follow.
    if (nOffset < kPadInfoIndexCount || nOffset >= (int)sizeof(pFrame->aabCombInfo[0]) - 1) {
        return 0;
    }
    return pFrame->aabCombInfo[nList][nOffset - kPadInfoIndexCount];
}

// NTSC-U/C: 0x0059cb18, PAL: 0x005a6e00
int scePadInfoMode(int nPort, int nSlot, int nTerm, int nOffset) {
    const scePadDmaFrame *pFrame;

    if (g_aaPadPorts[nPort][nSlot].nOpen == 0) {
        return 0;
    }
    pFrame = scePadGetDmaStr(nPort, nSlot);
    if (pFrame->nCurrentTask != kPadTaskReady || pFrame->nReqState == scePadReqStateBusy) {
        return 0;
    }
    switch (nTerm) {
    case InfoModeCurID:
        if (pFrame->nModeCurrentId == kPadModeCurrentIdConfig) {
            return 0;
        }
        return pFrame->nModeCurrentId >> kPadModeIdShift;
    case InfoModeCurExID:
        if (pFrame->nModeConfig == kPadModeConfigNone) {
            return 0;
        }
        return pFrame->anModeTable[pFrame->nModeCurrentOffset];
    case InfoModeCurExOffs:
        if (pFrame->nModeConfig == kPadModeConfigNone) {
            return 0;
        }
        return pFrame->nModeCurrentOffset;
    case InfoModeIdTable:
        if (pFrame->nModeConfig == kPadModeConfigNone) {
            return 0;
        }
        if (nOffset == kPadInfoIndexCount) {
            return pFrame->nModeCount;
        }
        if (nOffset >= pFrame->nModeCount) {
            return 0;
        }
        return pFrame->anModeTable[nOffset];
    default:
        return 0;
    }
}

// NTSC-U/C: 0x0059cc50, PAL: 0x005a6f38
int scePadSetMainMode(int nPort, int nSlot, int nOffset, int nLock) {
    g_padRpc.mainMode.nOffset = nOffset;
    g_padRpc.mainMode.nLock = nLock;
    g_padRpc.nCommand = kPadCommandSetMainMode;
    g_padRpc.nPort = nPort;
    g_padRpc.nSlot = nSlot;
    if (PadCall() < 0) {
        return 0;
    }
    return PadAccepted(nPort, nSlot, &g_padRpc.mainMode.nResult);
}

// NTSC-U/C: 0x0059cd08, PAL: 0x005a6ff0
int scePadSetActDirect(int nPort, int nSlot, const unsigned char *pData) {
    PadDirectPacket *pPacket;
    int i;

    // Yes, the binary does not check that the port is open.
    if (scePadGetDmaStr(nPort, nSlot)->nCurrentTask != kPadTaskReady) {
        return 0;
    }
    pPacket = &g_aaPadPorts[nPort][nSlot].pDirect->packet;
    for (i = 0; i < kPadActuatorByteCount; ++i) {
        pPacket->abData[i] = pData[i];
    }
    pPacket->nCommand = kPadDirectCommandActuators;
    pPacket->nSize = kPadActuatorByteCount;
    _send_to_iop(nPort, nSlot);
    return 1;
}

// NTSC-U/C: 0x0059cdc0, PAL: 0x005a70a8
int scePadSetActAlign(int nPort, int nSlot, const unsigned char *pData) {
    int i;

    g_padRpc.nCommand = kPadCommandSetActAlign;
    g_padRpc.nPort = nPort;
    g_padRpc.nSlot = nSlot;
    for (i = 0; i < kPadActuatorByteCount; ++i) {
        g_padRpc.actAlign.abAlign[i] = pData[i];
    }
    if (PadCall() < 0) {
        return 0;
    }
    return PadAccepted(nPort, nSlot, &g_padRpc.actAlign.nResult);
}

// NTSC-U/C: 0x0059ce98, PAL: 0x005a7180
int scePadGetButtonMask(int nPort, int nSlot) {
    const scePadDmaFrame *pFrame = PadActuatorFrame(nPort, nSlot);
    unsigned int nMask = 0;
    int i;

    if (pFrame == NULL || pFrame->nModel < kPadModelPressure) {
        return 0;
    }
    for (i = sizeof(pFrame->abButtonMask) - 1; i >= 0; --i) {
        nMask = (nMask << kBitsPerByte) | pFrame->abButtonMask[i];
    }
    return (int)nMask;
}

// NTSC-U/C: 0x0059cf50, PAL: 0x005a7238
int scePadSetButtonInfo(int nPort, int nSlot, int nMask) {
    g_padRpc.buttonInfo.nMask = nMask;
    g_padRpc.nCommand = kPadCommandSetButtonInfo;
    g_padRpc.nPort = nPort;
    g_padRpc.nSlot = nSlot;
    if (PadCall() < 0) {
        return 0;
    }
    return PadAccepted(nPort, nSlot, &g_padRpc.buttonInfo.nResult);
}

// NTSC-U/C: 0x0059d000, PAL: 0x005a72e8
int scePadInfoPressMode(int nPort, int nSlot) {
    if (g_aaPadPorts[nPort][nSlot].nOpen == 0) {
        return 0;
    }
    return scePadGetButtonMask(nPort, nSlot) == kPadPressureMaskAll;
}

// NTSC-U/C: 0x0059d060, PAL: 0x005a7348
int scePadEnterPressMode(int nPort, int nSlot) {
    if (g_aaPadPorts[nPort][nSlot].nOpen == 0) {
        return 0;
    }
    return scePadSetButtonInfo(nPort, nSlot, kPadPressureButtonsAll);
}

// NTSC-U/C: 0x0059d0b8, PAL: 0x005a73a0
int scePadExitPressMode(int nPort, int nSlot) {
    if (g_aaPadPorts[nPort][nSlot].nOpen == 0) {
        return 0;
    }
    return scePadSetButtonInfo(nPort, nSlot, kPadPressureButtonsNone);
}

// NTSC-U/C: 0x0059d110, PAL: 0x005a73f8
int scePadSetVrefParam(int nPort, int nSlot, const unsigned char *pParam) {
    g_padRpc.nPort = nPort;
    g_padRpc.nCommand = kPadCommandSetVrefParam;
    g_padRpc.nSlot = nSlot;
    memcpy(g_padRpc.vref.abParam, pParam, sizeof(g_padRpc.vref.abParam));
    if (PadCall() < 0) {
        return 0;
    }
    return PadAccepted(nPort, nSlot, &g_padRpc.vref.nResult);
}

// NTSC-U/C: 0x0059d1e0, PAL: 0x005a74c8
int scePadGetPortMax(void) {
    g_padRpc.nCommand = kPadCommandGetPortMax;
    if (PadCall() < 0) {
        return 0;
    }
    return g_padRpc.status.nResult;
}

// NTSC-U/C: 0x0059d248, PAL: 0x005a7530
int scePadGetSlotMax(int nPort) {
    g_padRpc.nPort = nPort;
    g_padRpc.nCommand = kPadCommandGetSlotMax;
    if (PadCall() < 0) {
        return 0;
    }
    return g_padRpc.status.nResult;
}

// NTSC-U/C: 0x0059d2b0, PAL: 0x005a7598
int scePadGetModVersion(void) {
    g_padRpc.nCommand = kPadCommandGetModVersion;
    if (PadCall() < 0) {
        return 0;
    }
    return g_padRpc.status.nResult;
}

// NTSC-U/C: 0x0059d318, PAL: 0x005a7600
int scePadSetWarningLevel(int nLevel) {
    g_padRpc.nPort = nLevel;
    g_padRpc.nCommand = kPadCommandSetWarningLevel;
    if (PadCall() < 0) {
        return 0;
    }
    return g_padRpc.nSlot;
}
