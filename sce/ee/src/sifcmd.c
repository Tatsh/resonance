#include <stddef.h>
#include <stdint.h>

#include <eekernel.h>
#include <sifcmd.h>
#include <sifdev.h>

enum {
    kSystemHandlerCount = 32,
    kSoftwareRegisterCount = 32,
    kQuadSize = 16,
    kMinPacketSize = 16,
    kPacketQuadCount = SIF_CMD_PACKET_MAX / kQuadSize,
    kUnusedBufferSize = 64,
    kDCacheLineSize = 64,

    // The system handlers the layer installs itself.
    kChangeAddressSlot = 0,
    kSetSoftwareRegisterSlot = 1,

    // The DMA controller status register, and its SIF0 channel interrupt bit. Writing the bit
    // clears it.
    kDmacStatRegister = 0x1000e010,
    kDmacStatSif0 = 0x20,
    // The SIF0 channel control register, and its bit set while the channel runs.
    kSif0ChcrRegister = 0x1000c000,
    kChcrStart = 0x100,

    kPacketTransferMode = SIF_DMA_INT_O | SIF_DMA_ERT,
};

// One quadword, the unit the command packets move in.
typedef unsigned int SifQuad __attribute__((mode(TI)));

// A command packet, as the SIF0 channel delivers it.
typedef union {
    SifQuad aQuads[kPacketQuadCount];
    sceSifCmdHdr header;
} SifCmdPacket;

// The state the command layer shares with its handlers. The IOP receives its address through
// SIF_SYSREG_MAINADDR. The members retain the order and 32-bit width of the original.
typedef struct {
    SifCmdPacket *pReceiveBuffer; // Uncached.
    void *pUnusedBuffer;          // Uncached, and never read after start-up.
    unsigned int nIopBuffer;
    sceSifCmdData *pSystemHandlers;
    int nSystemHandlerCount;
    sceSifCmdData *pUserHandlers;
    int nUserHandlerCount;
    int *pSoftwareRegisters;
} SifCmdState;

// NTSC-U/C: 0x0077b4d4, PAL: 0x007bf2a4
static int g_nCmdInitialized = 0;

// NTSC-U/C: 0x008e4b00, PAL: 0x00929b00, where the SIF0 channel delivers each packet from the IOP.
// SIF DMA needs the 64-byte alignment retail gives it.
static SifCmdPacket g_cmdReceiveBuffer __attribute__((aligned(64)));

// NTSC-U/C: 0x008e4b80, PAL: 0x00929b80
static unsigned char g_abCmdUnusedBuffer[kUnusedBufferSize] __attribute__((aligned(64)));

// NTSC-U/C: 0x008e4bc0, PAL: 0x00929bc0. SIF DMA needs the 64-byte alignment retail gives it.
static sceSifCmdCSData g_cmdInitPacket __attribute__((aligned(64)));

// NTSC-U/C: 0x008e4bd4, PAL: 0x00929bd4
static int g_nCmdHandlerId;

// NTSC-U/C: 0x008e4bd8, PAL: 0x00929bd8
static SifCmdState g_sifCmdState;

// NTSC-U/C: 0x008e4c00, PAL: 0x00929c00
static sceSifCmdData g_aSystemHandlers[kSystemHandlerCount];

// NTSC-U/C: 0x008e4d00, PAL: 0x00929d00
static int g_anSoftwareRegisters[kSoftwareRegisterCount];

// NTSC-U/C: 0x005d3558, PAL: 0x006155c0
static void _set_sreg(void *pPacket, void *pData) {
    const sceSifCmdSRData *pRequest = pPacket;
    SifCmdState *pState = pData;

    pState->pSoftwareRegisters[pRequest->rno] = (int)pRequest->value;
}

// NTSC-U/C: 0x005d3578, PAL: 0x006155e0
static void changeAddressHandler(void *pPacket, void *pData) {
    const sceSifCmdCSData *pRequest = pPacket;
    SifCmdState *pState = pData;

    pState->nIopBuffer = pRequest->newaddr;
}

// NTSC-U/C: 0x005d3ac8, PAL: 0x00615b30
// Runs for every packet the IOP sends. The packet is copied out so the channel can take the next
// one before the handler runs.
static int cmdInterruptHandler(int nChannel) {
    SifCmdPacket packet;
    SifCmdPacket *pReceived = g_sifCmdState.pReceiveBuffer;
    sceSifCmdData *pHandler;
    int nSize;
    int nQuads;
    int nCode;
    int i;

    (void)nChannel;
    EIntr(); // Yes, the binary enables interrupts inside its interrupt handler.
    nSize = pReceived->header.psize;
    if (nSize == 0) {
        return 0;
    }
    nQuads = (nSize + (kQuadSize - 1)) / kQuadSize;
    pReceived->header.psize = 0;
    for (i = 0; i < nQuads; ++i) {
        packet.aQuads[i] = pReceived->aQuads[i];
    }
    isceSifSetDChain();

    nCode = (int)packet.header.fcode;
    if (nCode < 0) {
        nCode &= ~SIF_CMDC_SYSTEM;
        if (nCode < g_sifCmdState.nSystemHandlerCount) {
            pHandler = &g_sifCmdState.pSystemHandlers[nCode];
            if (pHandler->func != NULL) {
                pHandler->func(&packet, pHandler->data);
            }
        }
    } else if (nCode < g_sifCmdState.nUserHandlerCount) {
        pHandler = &g_sifCmdState.pUserHandlers[nCode];
        if (pHandler->func != NULL) {
            pHandler->func(&packet, pHandler->data);
        }
    }
    ExitHandler();
    return 0;
}

// NTSC-U/C: 0x005d3910, PAL: 0x00615978
// The common body of sceSifSendCmd() and isceSifSendCmd(). The extra data travels in the same DMA
// chain, ahead of the packet.
static unsigned int _sceSifSendCmd(unsigned int fcode,
                                   unsigned int nMode,
                                   sceSifCmdHdr *pPacket,
                                   int nPacketSize,
                                   void *pSrc,
                                   void *pDest,
                                   int nSize) {
    sceSifDmaData aTransfers[2];
    int nCount = 0;

    if ((unsigned int)(nPacketSize - kMinPacketSize) > SIF_CMD_PACKET_MAX - kMinPacketSize) {
        return 0;
    }
    if (nSize > 0) {
        pPacket->dsize = (unsigned int)nSize;
        pPacket->daddr = (unsigned int)(uintptr_t)pDest;
        aTransfers[0].data = (unsigned int)(uintptr_t)pSrc;
        aTransfers[0].addr = (unsigned int)(uintptr_t)pDest;
        aTransfers[0].size = (unsigned int)nSize;
        aTransfers[0].mode = 0;
        nCount = 1;
        if ((nMode & SIF_CMDM_WBDC) != 0) {
            sceSifWriteBackDCache(pSrc, nSize);
        }
    } else {
        pPacket->dsize = 0;
        pPacket->daddr = 0;
    }
    aTransfers[nCount].data = (unsigned int)(uintptr_t)pPacket;
    aTransfers[nCount].addr = g_sifCmdState.nIopBuffer;
    aTransfers[nCount].size = (unsigned int)nPacketSize;
    aTransfers[nCount].mode = kPacketTransferMode;
    pPacket->fcode = fcode;
    pPacket->psize = (unsigned int)nPacketSize;
    sceSifWriteBackDCache(pPacket, nPacketSize);
    ++nCount;
    if ((nMode & SIF_CMDM_INTR) != 0) {
        return isceSifSetDma(aTransfers, nCount);
    }
    return sceSifSetDma(aTransfers, nCount);
}

// NTSC-U/C: 0x005d3588, PAL: 0x006155f0
int sceSifGetSreg(int reg) {
    return g_anSoftwareRegisters[reg];
}

// NTSC-U/C: 0x005d35a0, PAL: 0x00615608
int sceSifSetSreg(int reg, int value) {
    g_anSoftwareRegisters[reg] = value;
    return value;
}

// NTSC-U/C: 0x005d35d0, PAL: 0x00615638
void sceSifInitCmd(void) {
    volatile unsigned int *pDmacStat = (volatile unsigned int *)kDmacStatRegister;
    volatile unsigned int *pSif0Chcr = (volatile unsigned int *)kSif0ChcrRegister;
    int i;

    DIntr();
    if (g_nCmdInitialized != 0) {
        EIntr();
        return;
    }
    g_nCmdInitialized = 1;
    g_sifCmdState.pReceiveBuffer = UNCACHED_SEG(&g_cmdReceiveBuffer);
    g_sifCmdState.pUnusedBuffer = UNCACHED_SEG(g_abCmdUnusedBuffer);
    g_sifCmdState.nIopBuffer = 0;
    g_sifCmdState.pSystemHandlers = g_aSystemHandlers;
    g_sifCmdState.nSystemHandlerCount = kSystemHandlerCount;
    g_sifCmdState.pUserHandlers = NULL;
    g_sifCmdState.nUserHandlerCount = 0;
    g_sifCmdState.pSoftwareRegisters = g_anSoftwareRegisters;
    for (i = 0; i < kSystemHandlerCount; ++i) {
        g_aSystemHandlers[i].func = NULL;
        g_aSystemHandlers[i].data = NULL;
    }
    for (i = 0; i < kSoftwareRegisterCount; ++i) {
        g_anSoftwareRegisters[i] = 0;
    }
    g_aSystemHandlers[kChangeAddressSlot].func = changeAddressHandler;
    g_aSystemHandlers[kChangeAddressSlot].data = &g_sifCmdState;
    g_aSystemHandlers[kSetSoftwareRegisterSlot].func = _set_sreg;
    g_aSystemHandlers[kSetSoftwareRegisterSlot].data = &g_sifCmdState;
    EIntr();

    FlushCache(WRITEBACK_DCACHE);
    if ((*pDmacStat & kDmacStatSif0) != 0) {
        *pDmacStat = kDmacStatSif0;
    }
    if ((*pSif0Chcr & kChcrStart) == 0) {
        sceSifSetDChain();
    }
    g_nCmdHandlerId = AddDmacHandler(DMAC_SIF0, cmdInterruptHandler, 0);
    EnableDmac(DMAC_SIF0);

    // A nonzero address means the IOP command layer is already running, and it only needs the new
    // receive buffer.
    g_sifCmdState.nIopBuffer = (unsigned int)sceSifGetReg(SIF_SYSREG_SUBADDR);
    if (g_sifCmdState.nIopBuffer != 0) {
        g_cmdInitPacket.newaddr = (unsigned int)(uintptr_t)&g_cmdReceiveBuffer;
        sceSifSendCmd(
            SIF_CMDC_CHANGE_SADDR, &g_cmdInitPacket, sizeof(g_cmdInitPacket), NULL, NULL, 0);
        return;
    }
    while ((sceSifGetReg(SIF_REG_SMFLAG) & SIF_STAT_CMDINIT) == 0) {
    }
    g_sifCmdState.nIopBuffer = (unsigned int)sceSifGetReg(SIF_REG_SUBADDR);
    sceSifSetReg(SIF_SYSREG_SUBADDR, (int)g_sifCmdState.nIopBuffer);
    sceSifSetReg(SIF_SYSREG_MAINADDR, (int)(uintptr_t)&g_sifCmdState);
    g_cmdInitPacket.newaddr = (unsigned int)(uintptr_t)&g_cmdReceiveBuffer;
    g_cmdInitPacket.chdr.opt = 0;
    sceSifSendCmd(SIF_CMDC_INIT_CMD, &g_cmdInitPacket, sizeof(g_cmdInitPacket), NULL, NULL, 0);
}

// NTSC-U/C: 0x005d3850, PAL: 0x006158b8
void sceSifExitCmd(void) {
    DisableDmac(DMAC_SIF0);
    RemoveDmacHandler(DMAC_SIF0, g_nCmdHandlerId);
    g_nCmdInitialized = 0;
}

// NTSC-U/C: 0x005d3888, PAL: 0x006158f0
sceSifCmdData *sceSifSetCmdBuffer(sceSifCmdData *db, int size) {
    sceSifCmdData *pPrevious = g_sifCmdState.pUserHandlers;

    g_sifCmdState.nUserHandlerCount = size;
    g_sifCmdState.pUserHandlers = db;
    return pPrevious;
}

// NTSC-U/C: 0x005d38a0, PAL: 0x00615908
sceSifCmdData *sceSifSetSysCmdBuffer(sceSifCmdData *db, int size) {
    sceSifCmdData *pPrevious = g_sifCmdState.pSystemHandlers;

    g_sifCmdState.nSystemHandlerCount = size;
    g_sifCmdState.pSystemHandlers = db;
    return pPrevious;
}

// NTSC-U/C: 0x005d38b8, PAL: 0x00615920
void sceSifAddCmdHandler(unsigned int fcode, sceSifCmdHandler handler, void *data) {
    sceSifCmdData *pTable = g_sifCmdState.pUserHandlers;

    if ((fcode & SIF_CMDC_SYSTEM) != 0) {
        pTable = g_sifCmdState.pSystemHandlers;
    }
    pTable[fcode & ~SIF_CMDC_SYSTEM].data = data;
    pTable[fcode & ~SIF_CMDC_SYSTEM].func = handler;
}

// NTSC-U/C: 0x005d38e8, PAL: 0x00615950
void sceSifRemoveCmdHandler(unsigned int fcode) {
    sceSifCmdData *pTable = g_sifCmdState.pUserHandlers;

    if ((fcode & SIF_CMDC_SYSTEM) != 0) {
        pTable = g_sifCmdState.pSystemHandlers;
    }
    pTable[fcode & ~SIF_CMDC_SYSTEM].func = NULL;
}

// NTSC-U/C: 0x005d3a48, PAL: 0x00615ab0
unsigned int sceSifSendCmd(unsigned int fcode,
                           void *packet,
                           int packet_size,
                           void *src_extra,
                           void *dest_extra,
                           int size_extra) {
    return _sceSifSendCmd(fcode, 0, packet, packet_size, src_extra, dest_extra, size_extra);
}

// NTSC-U/C: 0x005d3a88, PAL: 0x00615af0
unsigned int isceSifSendCmd(unsigned int fcode,
                            void *packet,
                            int packet_size,
                            void *src_extra,
                            void *dest_extra,
                            int size_extra) {
    return _sceSifSendCmd(
        fcode, SIF_CMDM_INTR, packet, packet_size, src_extra, dest_extra, size_extra);
}

// NTSC-U/C: 0x005d3bf0, PAL: 0x00615c58
void sceSifWriteBackDCache(void *addr, int size) {
    uintptr_t nLine;
    uintptr_t nLast;

    if (size <= 0) {
        return;
    }
    nLine = (uintptr_t)addr & ~(uintptr_t)(kDCacheLineSize - 1);
    nLast = ((uintptr_t)addr + (uintptr_t)size - 1) & ~(uintptr_t)(kDCacheLineSize - 1);
    for (; nLine <= nLast; nLine += kDCacheLineSize) {
        // Hit write-back invalidate of one data cache line.
        __asm__ volatile("sync.l\n\tcache 0x18, 0(%0)\n\tsync.l" : : "r"(nLine) : "memory");
    }
}
