#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <eekernel.h>
#include <libsdr.h>
#include <sifrpc.h>

#include "os/log.h"

enum {
    // The sound driver RPC server uses this identifier for the bind call.
    kSdrRpcServer = 0x80000701,
    // The bind retry loop spins this many iterations before checking readiness.
    kSdrBindDelay = 10000,
    // The selector that installs a transfer-completion handler for one DMA channel.
    kSdrCommandSetTransIntrHandler = 0x8160,
    // The selector that installs the SPU2 interrupt handler.
    kSdrCommandSetSpu2IntrHandler = 0x8170,
    // The selector that sends a core's effect attributes from a caller buffer.
    kSdrCommandSetEffectAttr = 0x8130,
    // The selector that reads a core's effect attributes into a caller buffer.
    kSdrCommandGetEffectAttr = 0x8140,
    // The remote call sends this many bytes for the three main paths.
    kSdrSendSize = 0x40,
    // The default path receives this many bytes into the packet.
    kSdrReceiveSize = 0x10,
};

// The packet stores the self reference with six forwarded words. Every call sends the whole
// 0x40-byte packet.
typedef struct {
    // The packet address before the call, and the reply word the IOP writes back over it.
    int mResult;
    // The six variadic argument words after the command, in call order.
    int mArgument0;
    int mArgument1;
    int mArgument2;
    int mArgument3;
    int mArgument4;
    int mArgument5;
    // The rest of the packet the remote call transfers.
    unsigned char mReserved1C[0x24]; // +0x1C
} SdrPacket;

// NTSC-U/C: 0x008e3e00, PAL: 0x00929180
static SdrPacket g_sdrPacket __attribute__((aligned(64)));

// NTSC-U/C: 0x008e3e40, PAL: 0x009291c0
static sceSifClientData g_sdrClient __attribute__((aligned(64)));

// The transfer handler of DMA channel 0. The interrupt-handler commands record it and the five
// words below.
// NTSC-U/C: 0x007b2754, PAL: 0x007f6454
static int _sce_sdr_transIntr0Hdr;

// The transfer handler of DMA channel 1.
// NTSC-U/C: 0x007b2758, PAL: 0x007f6458
static int _sce_sdr_transIntr1Hdr;

// The SPU2 interrupt handler.
// NTSC-U/C: 0x007b275c, PAL: 0x007f645c
static int _sce_sdr_spu2IntrHdr;

// The argument of the channel 0 transfer handler.
// NTSC-U/C: 0x007b2760, PAL: 0x007f6460
static int _sce_sdr_transIntr0Arg;

// The argument of the channel 1 transfer handler.
// NTSC-U/C: 0x007b2764, PAL: 0x007f6464
static int _sce_sdr_transIntr1Arg;

// The argument of the SPU2 interrupt handler.
// NTSC-U/C: 0x007b2768, PAL: 0x007f6468
static int _sce_sdr_spu2IntrArg;

// The completion callback of a call made with a zero control word. The image never writes it.
// NTSC-U/C: 0x00765d38, PAL: 0x007a8cd0
static sceSifEndFunc g_pfnSdrEndFunction;

int sceSdRemoteInit(void) {
    sceSifClientData *pClient = &g_sdrClient;
    int nBind;
    int nDelay;

    sceSifInitRpc(0);

    for (;;) {
        nBind = sceSifBindRpc(pClient, kSdrRpcServer, 0);
        if (nBind < 0) {
            printf("sceSdRemoteInit() RPC bind error!\n");
            return -1;
        }
        nDelay = kSdrBindDelay;
        nDelay--;
        while (nDelay != -1) {
            nDelay--;
        }
        if (pClient->serve == NULL) {
            continue;
        }
        FlushCache(0);
        return 0;
    }
}

int sceSdRemote(int nControl, ...) {
    SdrPacket *pPacket = &g_sdrPacket;
    sceSifClientData *pClient = &g_sdrClient;
    va_list oArguments;
    int nCommand;
    int nFlag;
    // The block-read path below returns the preserved register rather than a computed value, so
    // no initialiser can reproduce it. The reconstruction returns zero there.
    int nResult = 0;
    sceSifEndFunc pfnEnd;

    nFlag = 0;
    pfnEnd = NULL;
    if (nControl == 0) {
        nFlag = 1;
        pfnEnd = g_pfnSdrEndFunction;
    }
    pPacket->mResult = (int)(uintptr_t)pPacket;

    va_start(oArguments, nControl);
    nCommand = va_arg(oArguments, int);
    pPacket->mArgument0 = va_arg(oArguments, int);
    pPacket->mArgument1 = va_arg(oArguments, int);
    pPacket->mArgument2 = va_arg(oArguments, int);
    pPacket->mArgument3 = va_arg(oArguments, int);
    pPacket->mArgument4 = va_arg(oArguments, int);
    pPacket->mArgument5 = va_arg(oArguments, int);
    va_end(oArguments);

    if (nCommand == kSdrCommandSetTransIntrHandler) {
        if (pPacket->mArgument0 == 0) {
            _sce_sdr_transIntr0Arg = pPacket->mArgument2;
            _sce_sdr_transIntr0Hdr = pPacket->mArgument1;
        } else {
            _sce_sdr_transIntr1Arg = pPacket->mArgument2;
            _sce_sdr_transIntr1Hdr = pPacket->mArgument1;
        }
    }
    if (nCommand == kSdrCommandSetSpu2IntrHandler) {
        _sce_sdr_spu2IntrArg = pPacket->mArgument1;
        _sce_sdr_spu2IntrHdr = pPacket->mArgument0;
    }
    if (nCommand == kSdrCommandSetEffectAttr) {
        sceSifCallRpc(pClient,
                      pPacket->mArgument0 | kSdrCommandSetEffectAttr,
                      nFlag,
                      (void *)(uintptr_t)pPacket->mArgument1,
                      kSdrSendSize,
                      NULL,
                      0,
                      pfnEnd,
                      pPacket);
        nResult = pPacket->mResult;
        return nResult;
    }
    if (nCommand == kSdrCommandGetEffectAttr) {
        sceSifCallRpc(pClient,
                      pPacket->mArgument0 | kSdrCommandGetEffectAttr,
                      nFlag,
                      pPacket,
                      kSdrSendSize,
                      (void *)(uintptr_t)pPacket->mArgument1,
                      kSdrSendSize,
                      pfnEnd,
                      (void *)(uintptr_t)pPacket->mArgument1);
        return nResult;
    }
    sceSifCallRpc(pClient,
                  nCommand,
                  nFlag,
                  pPacket,
                  kSdrSendSize,
                  pPacket,
                  kSdrReceiveSize,
                  pfnEnd,
                  pPacket);
    nResult = pPacket->mResult;
    return nResult;
}
