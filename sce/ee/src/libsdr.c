#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <eekernel.h>
#include <libsdr.h>
#include <sifrpc.h>

#include "os/log.h"

enum {
    // The sound driver RPC server uses this identifier for the bind call.
    kSdrRpcServer = 0x80000701,
    // The bind retry loop spins this many iterations before checking readiness.
    kSdrBindDelay = 10000,
    // The packet at the fixed address uses this base for its self reference.
    kSdrPacketBase = 0x8e3e00,
    // The client structure resides at this address in the image.
    kSdrClientBase = 0x8e3e40,
    // The callback table resides at this address in the image.
    kSdrCallbackBase = 0x7b2754,
    // The extra argument comes from this address when the first argument is zero.
    kSdrExtraSource = 0x765d38,
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

// The packet at 0x8e3e00 stores the self reference with six forwarded words.
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

// 0x005652c8
extern int sceSifCallRpcInternal(void *pClient,
                                 int nFunction,
                                 int nMode,
                                 void *pSend,
                                 int nSendSize,
                                 void *pReceive,
                                 int nReceiveSize,
                                 void *pExtra,
                                 void *pReserved);

// 0x00576fe0
int sceSdRemoteInit(void) {
    SifRpcClientData_t *pClient = (SifRpcClientData_t *)kSdrClientBase;
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
        if (pClient->server == NULL) {
            continue;
        }
        FlushCache(0);
        return 0;
    }
}

// 0x00577120
int sceSdRemote(int nControl, ...) {
    SdrPacket *pPacket = (SdrPacket *)kSdrPacketBase;
    SifRpcClientData_t *pClient = (SifRpcClientData_t *)kSdrClientBase;
    SdrCallbackTable *pTable = (SdrCallbackTable *)kSdrCallbackBase;
    va_list oArguments;
    int nCommand;
    int nFlag;
    int nExtra;
    // The block-read path below returns the preserved register rather than a computed value, so
    // no initialiser can reproduce it. The reconstruction returns zero there.
    int nResult = 0;
    void *pExtra;

    nFlag = 0;
    nExtra = 0;
    if (nControl == 0) {
        nFlag = 1;
        nExtra = *(volatile int *)kSdrExtraSource;
    }
    pExtra = (void *)(uintptr_t)nExtra;
    pPacket->mUnknown00 = kSdrPacketBase;

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
        sceSifCallRpcInternal(pClient,
                              pPacket->mUnknown04 | kSdrCommand8130,
                              nFlag,
                              (void *)(uintptr_t)pPacket->mUnknown08,
                              kSdrSendSize,
                              NULL,
                              0,
                              pExtra,
                              pPacket);
        nResult = pPacket->mUnknown00;
        return nResult;
    }
    if (nCommand == kSdrCommand8140) {
        sceSifCallRpcInternal(pClient,
                              pPacket->mUnknown04 | kSdrCommand8140,
                              nFlag,
                              pPacket,
                              kSdrSendSize,
                              (void *)(uintptr_t)pPacket->mUnknown08,
                              kSdrSendSize,
                              pExtra,
                              (void *)(uintptr_t)pPacket->mUnknown08);
        return nResult;
    }
    sceSifCallRpcInternal(pClient,
                          nCommand,
                          nFlag,
                          pPacket,
                          kSdrSendSize,
                          pPacket,
                          kSdrReceiveSize,
                          pExtra,
                          pPacket);
    nResult = pPacket->mUnknown00;
    return nResult;
}
