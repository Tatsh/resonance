#include <stdint.h>
#include <string.h>

#include "os/log.h"

// Declarations match compat/libpad.h. The header is not included here
// because the syntax check does not provide the pad include path.
int scePadRead(int nPort, int nSlot, unsigned char *pData);
int scePadSetActDirect(int nPort, int nSlot, const unsigned char *pData);
int scePadSetActAlign(int nPort, int nSlot, const unsigned char *pAlign);

// The actuator byte count copied by both direct and align paths.
enum {
    kPadActuatorCount = 6,
    kPadActDirectReady = 1,
    kPadAlignRequestCode = 8,
    kPadAlignFunction = 1,
    kPadAlignTransferSize = 0x80,
    kPadSetReqStateBusy = 2
};

// The per slot request block the direct path fills.
typedef struct {
    unsigned char mUnknown00[4]; // +0x00
    int mUnknown04; // +0x04 pending flag
    int mUnknown08; // +0x08 byte count
    unsigned char mUnknown0C[6]; // +0x0C actuator bytes
} PadActRequest;

// The DMA report the read path copies.
typedef struct {
    unsigned char mUnknown00[0x60]; // +0x00
    int mUnknown60; // +0x60 report length
    unsigned char mUnknown64[0x0E]; // +0x64
    unsigned char mUnknown72; // +0x72 actuator capability
} PadDmaReport;

// One slot entry inside the pad state table.
typedef struct {
    void *mUnknown00; // +0x00: DMA pointer for the report selector.
    void *mUnknown04; // +0x04: counter block.
    void *mUnknown08; // +0x08: DMA buffer for transfers.
    int mUnknown0C; // +0x0c: transfer id.
    int mUnknown10; // +0x10 active flag
    unsigned char mUnknown14[8]; // +0x14
} PadSlotEntry;

// One port with four slot entries. Four times 0x1C is 0x70.
typedef struct {
    PadSlotEntry mSlots[4]; // +0x00
} PadPort;

// The align packet sent through the remote call.
typedef struct {
    int mUnknown00; // +0x00 request code
    int mUnknown04; // +0x04 port
    int mUnknown08; // +0x08 slot
    unsigned char mUnknown0C[6]; // +0x0C align bytes
    unsigned char mUnknown12[2]; // +0x12
    int mUnknown14; // +0x14 result
    unsigned char mUnknown18[0x68]; // +0x18
} PadAlignPacket;

// Six actuator bytes as passed by the caller.
typedef struct {
    unsigned char mBytes[6]; // +0x00
} PadActuatorBytes;

// The pad state table at 0x8E42D0 in the image.
static PadPort *PadStateTable(void) {
    return (PadPort *)(uintptr_t)0x8E42D0U;
}

// The align packet at 0x8E45C0 in the image.
static PadAlignPacket *PadAlignPacketBlock(void) {
    return (PadAlignPacket *)(uintptr_t)0x8E45C0U;
}

// One DMA buffer the report selector returns, with the compared words.
typedef struct {
    unsigned char mReserved00[0x58]; // +0x00
    int mUnknown58; // +0x58: compared against the word at +0xd8.
    unsigned char mReserved5C[0x15]; // +0x5c
    unsigned char mUnknown71; // +0x71: request state byte.
    unsigned char mReserved72[0x66]; // +0x72
    int mUnknownD8; // +0xd8.
    unsigned char mReservedDC[0x24]; // +0xdc
} PadDmaBuffer;

// SIF DMA primitives the pad paths share, provided by the toolchain kernel.
extern int sceSifDmaStat(int nTransfer);
extern int sceSifSetDma(void *pTransfer, int nCount);

// 0x0059c608
void *scePadGetDmaStr(int nPort, int nSlot) {
    PadSlotEntry *pEntry = &PadStateTable()[nPort].mSlots[nSlot];
    PadDmaBuffer *pDma = (PadDmaBuffer *)pEntry->mUnknown00;
    void *pEnd = (void *)((uintptr_t)pDma + 0x100u);

    LibkSyncDCacheRange(pDma, pEnd);
    if (pDma->mUnknown58 < pDma->mUnknownD8) {
        return (void *)((uintptr_t)pDma + 0x80u);
    }
    return pDma;
}

// One transfer packet the slot starter sends, with the counter block, the buffer, and the size.
typedef struct {
    void *mUnknown00; // +0x00: counter block.
    void *mUnknown04; // +0x04: buffer.
    int mUnknown08; // +0x08: size word.
    int mUnknown0C; // +0x0c: cleared.
} PadDmaPacket;

// 0x0059bfd8
void scePadSub0059bfd8(int nPort, int nSlot) {
    PadSlotEntry *pEntry = &PadStateTable()[nPort].mSlots[nSlot];
    int *pCounter = (int *)pEntry->mUnknown04;
    int nCount;
    PadDmaPacket packet;
    int nStatus;

    nStatus = sceSifDmaStat(pEntry->mUnknown0C);
    if (nStatus < 0) {
        return;
    }
    if (*(volatile int *)(uintptr_t)0x7729c == 0) {
        LogPrintf("libpad: sceSifSetDma faild\n");
        return;
    }
    nCount = *pCounter + 1;
    *pCounter = nCount;
    LibkSyncDCacheRange(pCounter, (void *)((uintptr_t)pCounter + 0x20u));
    packet.mUnknown00 = pCounter;
    packet.mUnknown04 = (void *)((uintptr_t)pEntry->mUnknown08 + (unsigned int)(nCount & 1) * 0x20u);
    packet.mUnknown08 = 0x20;
    packet.mUnknown0C = 0;
    nStatus = sceSifSetDma(&packet, 1);
    if (nStatus == 0 && *(volatile int *)(uintptr_t)0x7729c == 0) {
        LogPrintf("libpad: sceSifSetDma faild\n");
    }
    pEntry->mUnknown0C = nStatus;
}

// 0x0059c7e8
int scePadSetReqState(int nPort, int nSlot, int nState) {
    PadSlotEntry *pEntry = &PadStateTable()[nPort].mSlots[nSlot];
    PadDmaBuffer *pDma;

    if (pEntry->mUnknown10 == 0) {
        return 0;
    }
    pDma = (PadDmaBuffer *)scePadGetDmaStr(nPort, nSlot);
    pDma->mUnknown71 = (unsigned char)nState;
    return 1;
}

// Cache range sync for DMA buffers, through the interrupt-safe helper.
// 0x006207d8
void LibkSyncDCacheRange(void *pStart, void *pEnd) {
    unsigned int nStatus;
    int bEnabled;

    __asm__ volatile("mfc0 %0, $12" : "=r"(nStatus));
    bEnabled = (nStatus & 0x10000u) != 0;
    if (bEnabled != 0) {
        SpinDisableInterrupts();
    }
    scePadSub00620730((void *)((uintptr_t)pStart & 0xffffc0u),
        (void *)((uintptr_t)pEnd & 0xffffc0u));
    if (bEnabled != 0) {
        ReenableInterrupts();
    }
}

// Selective range writeback over the aligned bounds. A blanket writeback covers the same lines,
// so the reconstruction delegates to the toolchain primitive.
// 0x00620730
void scePadSub00620730(void *pStart, void *pEnd) {
    sceSifWriteBackDCache(
        pStart, (int)((uintptr_t)pEnd - (uintptr_t)pStart));
}

// 0x005652c8
extern int sceSifCallRpcInternal(void *pClient,
                                 int nFunction,
                                 int nMode,
                                 void *pSend,
                                 int nSendSize,
                                 void *pReceive,
                                 int nReceiveSize,
                                 void *pExtra,
                                 int nReserved);

// 0x0059c6b8
int scePadRead(int nPort, int nSlot, unsigned char *pData) {
    PadSlotEntry *pEntry = &PadStateTable()[nPort].mSlots[nSlot];
    PadDmaReport *pReport;
    int nLength;

    // An inactive slot has no report to copy.
    if (pEntry->mUnknown10 == 0) {
        return 0;
    }
    pReport = (PadDmaReport *)scePadGetDmaStr(nPort, nSlot);
    nLength = pReport->mUnknown60;
    memcpy(pData, pReport, (size_t)nLength);
    return pReport->mUnknown60;
}

// 0x0059cd08
int scePadSetActDirect(int nPort, int nSlot, const unsigned char *pData) {
    PadDmaReport *pReport = (PadDmaReport *)scePadGetDmaStr(nPort, nSlot);
    PadSlotEntry *pEntry;
    PadActRequest *pRequest;
    const PadActuatorBytes *pSource;
    int nIndex;

    // Only a pad reporting actuator support accepts direct levels.
    if (pReport->mUnknown72 != kPadActDirectReady) {
        return 0;
    }
    pEntry = &PadStateTable()[nPort].mSlots[nSlot];
    pRequest = pEntry->mUnknown04;
    pSource = (const PadActuatorBytes *)pData;
    nIndex = 0;
    while (nIndex < kPadActuatorCount) {
        pRequest->mUnknown0C[nIndex] = pSource->mBytes[nIndex];
        nIndex++;
    }
    pRequest->mUnknown04 = 1;
    pRequest->mUnknown08 = kPadActuatorCount;
    scePadSub0059bfd8(nPort, nSlot);
    return 1;
}

// 0x0059cdc0
int scePadSetActAlign(int nPort, int nSlot, const unsigned char *pAlign) {
    PadAlignPacket *pPacket = PadAlignPacketBlock();
    const PadActuatorBytes *pSource = (const PadActuatorBytes *)pAlign;
    void *pClient = (void *)(uintptr_t)0x8E4280U;
    int nResult;
    int nStatus;
    int nIndex;

    // The packet carries the request code with the port and slot.
    pPacket->mUnknown00 = kPadAlignRequestCode;
    pPacket->mUnknown04 = nPort;
    pPacket->mUnknown08 = nSlot;
    nIndex = 0;
    while (nIndex < kPadActuatorCount) {
        pPacket->mUnknown0C[nIndex] = pSource->mBytes[nIndex];
        nIndex++;
    }
    nResult = sceSifCallRpcInternal(pClient,
        kPadAlignFunction,
        0,
        pPacket,
        kPadAlignTransferSize,
        pPacket,
        kPadAlignTransferSize,
        NULL,
        0);
    // A failed remote call reports zero.
    if (nResult < 0) {
        return 0;
    }
    nStatus = pPacket->mUnknown14;
    // Any result besides success returns unchanged.
    if (nStatus != 1) {
        return nStatus;
    }
    scePadSetReqState(nPort, nSlot, kPadSetReqStateBusy);
    nStatus = pPacket->mUnknown14;
    return nStatus;
}
