#include <stdint.h>
#include <string.h>

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
    void *mUnknown00; // +0x00
    PadActRequest *mUnknown04; // +0x04
    unsigned char mUnknown08[8]; // +0x08
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

// 0x0059c608
extern void *scePadGetDmaStr(int nPort, int nSlot);

// 0x0059bfd8
extern void scePadSub0059bfd8(int nPort, int nSlot);

// 0x0059c7e8
extern int scePadSetReqState(int nPort, int nSlot, int nState);

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
