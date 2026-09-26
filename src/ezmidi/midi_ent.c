#include "ezmidi/ezmidi.h"

#include "ezmidi/common.h"
#include "ezmidi/imports.h"
#include "ezmidi/synth.h"

// This file mirrors the original `midi_ent.c`: the module entry, server thread,
// and RPC command handler.

// EZMIDI 0x8570
unsigned char gRpcBuf[0xc0];

// EZMIDI 0x6e70
int gRpcReply;

// EZMIDI 0x6c10
const char gRpcError[] = "EzMIDI driver error: unknown command %d (data %d)\n";

// EZMIDI 0xd0
int sce_midi_loop(void) {
    // Queue and server data on the stack; the binary sizes them by address.
    unsigned char aQueue[0x18];
    unsigned char aServer[0x50];

    CpuEnableIntr();
    EnableIntr(0x24);
    EnableIntr(0x28);
    sceSifInitRpc(0);
    sceSifSetRpcQueue(aQueue, GetThreadId());
    sceSifRegisterRpc(aServer, 0x12346, midiFunc, gRpcBuf, 0, 0, aQueue);
    sceSifRpcLoop(aQueue);
    return 0;
}

// EZMIDI 0x18c
void *midiFunc(int nCommand, void *pData, int nSize) {
    const int *pArgs = (const int *)pData;
    int nSub = nCommand & 0xf;

    (void)nSize; // Yes, the binary takes a size and never reads it.
    gRpcReply = 0;
    switch (nCommand & 0xfff0) {
    case 0x8010:
        gRpcReply = HardSynthInit();
        break;
    case 0x1050: {
        const int *pAttach = *(const int *const *)pData;

        gRpcReply = HardSynthAttachHDtoBD(nSub, pAttach[0], pAttach[3], pAttach[4]);
        break;
    }
    case 0x1070:
        gRpcReply = HardSynthLoadBD(pArgs[1], (const void *)pArgs[3], pArgs[2]);
        break;
    case 0xc0:
        HardSynthAllNotesOff(-1, 1);
        break;
    case 0x10e0:
        HardSynthConfig((const void *)pArgs[0]);
        break;
    case 0xf0:
        if (pArgs[0] != 0) {
            HardSynthPause();
        } else {
            HardSynthResume();
        }
        break;
    case 0x100:
        HardSynthSetRemix(pArgs[0]);
        break;
    case 0x110:
        HardSynthSetMono(pArgs[0]);
        break;
    case 0xd0:
        HardSynthInfo(pArgs[0]);
        break;
    case 0x120:
        HardSynthInvalidateHd((struct BankHeader *)pArgs[0]);
        break;
    case 0x8130:
        gRpcReply = HardSynthInvalidateBank(pArgs[0]);
        break;
    default:
        printf(gRpcError, nCommand, pArgs[0]);
        break;
    }
    return &gRpcReply;
}

// EZMIDI 0x0
int start(int nArgc, char **pArgv) {
    struct ThreadParam param;
    int nThread;

    (void)nArgc; // Yes, the binary never reads its arguments.
    (void)pArgv;
    CpuEnableIntr();
    if (sceSifCheckInit() != 0) {
        sceSifInit();
    }
    sceSifInitRpc(0);
    param.mAttr = 0x02000000;
    param.mOption = 0;
    param.mEntry = sce_midi_loop;
    param.mStackSize = 0x800;
    param.mPriority = 0x1e;
    nThread = CreateThread(&param);
    if (nThread <= 0) {
        return 1;
    }
    StartThread(nThread, 0);
    return 0;
}
