#include "ezmidi/ezmidi.h"

#include "ezmidi/imports.h"

// This file mirrors the original `midi_ent.c`: the module entry, server thread,
// and RPC command handler.

// EZMIDI 0x8570
unsigned char gRpcBuf[0xc0];

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
