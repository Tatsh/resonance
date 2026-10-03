#include <stddef.h>
#include <stdint.h>

#include <sifdev.h>
#include <sifrpc.h>

// The client of the IOP heap server.

enum {
    kHeapServerId = 0x80000003,
};

enum {
    kHeapFunctionAlloc = 1,
    kHeapFunctionFree = 2,
    kHeapFunctionLoad = 3,
    kHeapFunctionAllocSysMemory = 4,

    kHeapPathSize = 252,
    kBindRetryDelay = 0x100000,
};

// The argument of kHeapFunctionAllocSysMemory, in the order of the IOP allocator.
typedef struct {
    unsigned int nSize;
    int nType;
    void *pAddr;
} HeapAllocSysMemoryArgs;

// The argument of kHeapFunctionLoad. Only the path up to its terminator is sent.
typedef struct {
    void *pAddr;
    char szPath[kHeapPathSize];
} HeapLoadArgs;

// NTSC-U/C: 0x0077d68c, PAL: 0x007c1484, negative until the client is bound.
static int g_nHeapBound = -1;

// NTSC-U/C: 0x008e5180, PAL: 0x0092a180
static sceSifClientData g_heapClient __attribute__((aligned(64)));

// NTSC-U/C: 0x008e51c0, PAL: 0x0092a1c0. SIF DMA needs the 64-byte alignment retail gives it.
static int g_nHeapReceive __attribute__((aligned(64)));

// NTSC-U/C: 0x008e5200, PAL: 0x0092a200. SIF DMA needs the 64-byte alignment retail gives it.
static union {
    unsigned int nSize;
    void *pAddr;
    HeapAllocSysMemoryArgs allocSysMemory;
} g_heapSend __attribute__((aligned(64)));

// NTSC-U/C: 0x008e5240, PAL: 0x0092a240. SIF DMA needs the 64-byte alignment retail gives it.
static HeapLoadArgs g_heapLoad __attribute__((aligned(64)));

// NTSC-U/C: 0x005e5f88, PAL: 0x00628170
int sceSifInitIopHeap(void) {
    int nDelay;

    for (;;) {
        if (sceSifBindRpc(&g_heapClient, kHeapServerId, 0) < 0) {
            return -1;
        }
        if (g_heapClient.serve != NULL) {
            break;
        }
        for (nDelay = kBindRetryDelay; nDelay != -1; --nDelay) {
            __asm__ volatile("nop");
        }
    }
    g_nHeapBound = 0;
    return 0;
}

// NTSC-U/C: 0x005e6010, PAL: 0x006281f8
void *sceSifAllocIopHeap(unsigned int size) {
    if (g_nHeapBound < 0) {
        return NULL;
    }
    g_heapSend.nSize = size;
    if (sceSifCallRpc(&g_heapClient,
                      kHeapFunctionAlloc,
                      0,
                      &g_heapSend,
                      sizeof(g_heapSend.nSize),
                      &g_nHeapReceive,
                      sizeof(g_nHeapReceive),
                      NULL,
                      NULL) < 0) {
        return NULL;
    }
    return (void *)(uintptr_t)g_nHeapReceive;
}

// NTSC-U/C: 0x005e6080, PAL: 0x00628268
void *sceSifAllocSysMemory(int type, unsigned int size, void *addr) {
    if (g_nHeapBound < 0) {
        return NULL;
    }
    g_heapSend.allocSysMemory.nSize = size;
    g_heapSend.allocSysMemory.nType = type;
    g_heapSend.allocSysMemory.pAddr = addr;
    if (sceSifCallRpc(&g_heapClient,
                      kHeapFunctionAllocSysMemory,
                      0,
                      &g_heapSend,
                      sizeof(g_heapSend.allocSysMemory),
                      &g_nHeapReceive,
                      sizeof(g_nHeapReceive),
                      NULL,
                      NULL) < 0) {
        return NULL;
    }
    return (void *)(uintptr_t)g_nHeapReceive;
}

// NTSC-U/C: 0x005e6100, PAL: 0x006282e8
int sceSifFreeIopHeap(void *addr) {
    if (g_nHeapBound < 0) {
        return 0;
    }
    g_heapSend.pAddr = addr;
    if (sceSifCallRpc(&g_heapClient,
                      kHeapFunctionFree,
                      0,
                      &g_heapSend,
                      sizeof(g_heapSend.pAddr),
                      &g_nHeapReceive,
                      sizeof(g_nHeapReceive),
                      NULL,
                      NULL) < 0) {
        return -1;
    }
    return g_nHeapReceive;
}

// NTSC-U/C: 0x005e6178, PAL: 0x00628360
int sceSifFreeSysMemory(void *addr) {
    return sceSifFreeIopHeap(addr);
}

// NTSC-U/C: 0x005e6198, PAL: 0x00628380
int sceSifLoadIopHeap(const char *filename, void *addr) {
    int nLength;

    if (g_nHeapBound < 0) {
        return 0;
    }
    for (nLength = 0; nLength < kHeapPathSize; ++nLength) {
        g_heapLoad.szPath[nLength] = filename[nLength];
        if (filename[nLength] == '\0') {
            break;
        }
    }
    if (nLength == kHeapPathSize) {
        nLength = kHeapPathSize - 1;
    }
    g_heapLoad.pAddr = addr;
    g_heapLoad.szPath[kHeapPathSize - 1] = '\0';
    if (sceSifCallRpc(&g_heapClient,
                      kHeapFunctionLoad,
                      0,
                      &g_heapLoad,
                      (int)(offsetof(HeapLoadArgs, szPath) + nLength + 1),
                      &g_nHeapReceive,
                      sizeof(g_nHeapReceive),
                      NULL,
                      NULL) < 0) {
        return -1;
    }
    return g_nHeapReceive;
}
