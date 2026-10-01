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
    // The selector for the first voice path uses this value.
    kSdrCommand8160 = 0x8160,
    // The selector for the second voice path uses this value.
    kSdrCommand8170 = 0x8170,
    // The selector for the block write path uses this value.
    kSdrCommand8130 = 0x8130,
    // The selector for the block read path uses this value.
    kSdrCommand8140 = 0x8140,
    // The remote call sends this many bytes for the three main paths.
    kSdrSendSize = 0x40,
    // The default path receives this many bytes into the packet.
    kSdrReceiveSize = 0x10,
};

// The packet stores the self reference with six forwarded words. Every call sends the whole
// 0x40-byte packet.
typedef struct {
    // The field occupies offset 0x00.
    int mUnknown00;
    // The field occupies offset 0x04.
    int mUnknown04;
    // The field occupies offset 0x08.
    int mUnknown08;
    // The field occupies offset 0x0c.
    int mUnknown0c;
    // The field occupies offset 0x10.
    int mUnknown10;
    // The field occupies offset 0x14.
    int mUnknown14;
    // The field occupies offset 0x18.
    int mUnknown18;
    // The rest of the packet the remote call transfers.
    unsigned char mUnknown1C[0x24];
} SdrPacket;

// The callback table stores six words for the voice paths.
typedef struct {
    // The field occupies offset 0x00.
    int mUnknown00;
    // The field occupies offset 0x04.
    int mUnknown04;
    // The field occupies offset 0x08.
    int mUnknown08;
    // The field occupies offset 0x0c.
    int mUnknown0c;
    // The field occupies offset 0x10.
    int mUnknown10;
    // The field occupies offset 0x14.
    int mUnknown14;
} SdrCallbackTable;

// 0x008e3e00
static SdrPacket g_sdrPacket __attribute__((aligned(64)));

// 0x008e3e40
static sceSifClientData g_sdrClient;

// 0x007b2754
static SdrCallbackTable g_sdrCallbackTable;

// The completion callback of a call made with a zero control word. The image never writes it.
// 0x00765d38
static sceSifEndFunc g_pfnSdrEndFunction;

// 0x00576fe0
int sceSdRemoteInit(void) {
    sceSifClientData *pClient = &g_sdrClient;
    int nBind;
    int nDelay;

    sceSifInitRpc(0);

    for (;;) {
        nBind = sceSifBindRpc(pClient, kSdrRpcServer, 0);
        if (nBind < 0) {
            LogPrintf("sceSdRemoteInit() RPC bind error!\n");
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

// 0x00577120
int sceSdRemote(int nControl, ...) {
    SdrPacket *pPacket = &g_sdrPacket;
    sceSifClientData *pClient = &g_sdrClient;
    SdrCallbackTable *pTable = &g_sdrCallbackTable;
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
    pPacket->mUnknown00 = (int)(uintptr_t)pPacket;

    va_start(oArguments, nControl);
    nCommand = va_arg(oArguments, int);
    pPacket->mUnknown04 = va_arg(oArguments, int);
    pPacket->mUnknown08 = va_arg(oArguments, int);
    pPacket->mUnknown0c = va_arg(oArguments, int);
    pPacket->mUnknown10 = va_arg(oArguments, int);
    pPacket->mUnknown14 = va_arg(oArguments, int);
    pPacket->mUnknown18 = va_arg(oArguments, int);
    va_end(oArguments);

    if (nCommand == kSdrCommand8160) {
        if (pPacket->mUnknown04 == 0) {
            pTable->mUnknown0c = pPacket->mUnknown0c;
            pTable->mUnknown00 = pPacket->mUnknown08;
        } else {
            pTable->mUnknown10 = pPacket->mUnknown0c;
            pTable->mUnknown04 = pPacket->mUnknown08;
        }
    }
    if (nCommand == kSdrCommand8170) {
        pTable->mUnknown14 = pPacket->mUnknown08;
        pTable->mUnknown08 = pPacket->mUnknown04;
    }
    if (nCommand == kSdrCommand8130) {
        sceSifCallRpc(pClient,
                      pPacket->mUnknown04 | kSdrCommand8130,
                      nFlag,
                      (void *)(uintptr_t)pPacket->mUnknown08,
                      kSdrSendSize,
                      NULL,
                      0,
                      pfnEnd,
                      pPacket);
        nResult = pPacket->mUnknown00;
        return nResult;
    }
    if (nCommand == kSdrCommand8140) {
        sceSifCallRpc(pClient,
                      pPacket->mUnknown04 | kSdrCommand8140,
                      nFlag,
                      pPacket,
                      kSdrSendSize,
                      (void *)(uintptr_t)pPacket->mUnknown08,
                      kSdrSendSize,
                      pfnEnd,
                      (void *)(uintptr_t)pPacket->mUnknown08);
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
    nResult = pPacket->mUnknown00;
    return nResult;
}
