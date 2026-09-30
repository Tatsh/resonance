#include <stdio.h>
#include <stdlib.h>

#include <libmtap.h>
#include <sifrpc.h>

enum {
    kMtapServerId = 0x80000901,
};

enum {
    // One server per call, in the order of g_aMtapClients.
    kMtapClientPortOpen = 0,
    kMtapClientPortClose = 1,
    kMtapClientGetConnection = 2,
    kMtapClientChangePriority = 3,
    kMtapClientGetModVersion = 4,
    kMtapClientCount = 5,
    kMtapRpcFunction = 1,
    kMtapBindDelay = 0x10000,
    kMtapBindFailedStatus = -1,
    kMtapModVersionMajor = 3,
    kMtapModVersionMinor = 0,
    kMtapVersionShift = 8,
    kMtapVersionMinorMask = 0xff,
    kMtapRpcBufferSize = 128,
};

// The server reads its arguments from and writes its reply to the same 128 bytes.
typedef union {
    struct {
        int nPort;
        int nResult;
    } port;
    struct {
        int nFirstPriority;
        int nSecondPriority;
        int nResult;
    } priority;
    struct {
        int nVersion;
    } version;
    unsigned char abRaw[kMtapRpcBufferSize];
} __attribute__((aligned(64))) MtapRpcBuffer;

// 0x0089e080
static sceSifClientData g_aMtapClients[kMtapClientCount] __attribute__((aligned(64)));
// SIF DMA moves whole quadwords. The buffer retains retail's 64-byte placement.
// 0x0089e180
static MtapRpcBuffer g_mtapRpc __attribute__((aligned(64)));

// 0x0053a598
static void sceMtapPrintfStub(const char *pszFormat, ...) {
    (void)pszFormat; // Yes, the binary's library prints its RPC errors through an empty routine.
}

static inline int MtapCall(int nClient) {
    return sceSifCallRpc(&g_aMtapClients[nClient],
                         kMtapRpcFunction,
                         0,
                         &g_mtapRpc,
                         sizeof(g_mtapRpc),
                         &g_mtapRpc,
                         sizeof(g_mtapRpc),
                         NULL,
                         NULL);
}

// 0x0053a5c0
int sceMtapInit(void) {
    int nVersion;
    int i;
    int k;

    sceSifInitRpc(0);
    for (k = 0; k < kMtapClientCount; ++k) {
        for (;;) {
            if (sceSifBindRpc(&g_aMtapClients[k], kMtapServerId + k, 0) < 0) {
                sceMtapPrintfStub("libmtap: bind failed\n");
                exit(kMtapBindFailedStatus);
            }
            if (g_aMtapClients[k].serve != NULL) {
                break;
            }
            for (i = kMtapBindDelay; i != -1; --i) {
                __asm__ volatile("nop");
            }
        }
    }

    nVersion = sceMtapGetModVersion();
    if ((nVersion >> kMtapVersionShift) != kMtapModVersionMajor) {
        printf("libmtap: Module version mismatch ");
        printf("[libmtap.a = %d.%d, mtapman.irx = %d.%d]\n",
               kMtapModVersionMajor,
               kMtapModVersionMinor,
               nVersion >> kMtapVersionShift,
               nVersion & kMtapVersionMinorMask);
        return 0;
    }
    return 1;
}

// 0x0053a840
int sceMtapPortOpen(int nPort) {
    g_mtapRpc.port.nPort = nPort;
    if (MtapCall(kMtapClientPortOpen) < 0) {
        sceMtapPrintfStub("sceMtapPortOpen: rpc error\n");
        return 0;
    }
    return g_mtapRpc.port.nResult;
}

// 0x0053a8b0
int sceMtapPortClose(int nPort) {
    g_mtapRpc.port.nPort = nPort;
    if (MtapCall(kMtapClientPortClose) < 0) {
        sceMtapPrintfStub("sceMtapPortClose: rpc error\n");
        return 0;
    }
    return g_mtapRpc.port.nResult;
}

// 0x0053a920
int sceMtapGetConnection(int nPort) {
    g_mtapRpc.port.nPort = nPort;
    if (MtapCall(kMtapClientGetConnection) < 0) {
        sceMtapPrintfStub("sceMtapGetConnection: rpc error\n");
        return 0;
    }
    return g_mtapRpc.port.nResult;
}

// 0x0053a990
int sceMtapChangeThreadPriority(int nFirstPriority, int nSecondPriority) {
    g_mtapRpc.priority.nFirstPriority = nFirstPriority;
    g_mtapRpc.priority.nSecondPriority = nSecondPriority;
    if (MtapCall(kMtapClientChangePriority) < 0) {
        sceMtapPrintfStub("sceMtapChangeThreadPriority: rpc error\n");
        return 0;
    }
    return g_mtapRpc.priority.nResult;
}

// 0x0053aa00
int sceMtapGetModVersion(void) {
    if (MtapCall(kMtapClientGetModVersion) < 0) {
        sceMtapPrintfStub("sceMtapGetModVersion: rpc error\n");
        return 0;
    }
    return g_mtapRpc.version.nVersion;
}
