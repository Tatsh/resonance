#include "os/fileio.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "os/log.h"

// Declaration matches compat/libmc.h. The header is not included here
// because the syntax check lacks a compatible memory card header.
extern "C" int sceMcInitLibrary(void);

// The console initialised flag at 0x76F014 in the image.
static int *ConsoleInitialisedFlag(void) {
    return reinterpret_cast<int *>(static_cast<uintptr_t>(0x76F014U));
}

// 0x00627C38
extern "C" int sceDeci2Sub00627C38(const void *pBuffer);
// Console output state, shared by the write paths below.
typedef struct {
    int mUnknown00;   // +0x00
    int mUnknown04;   // +0x04
    int mUnknown08;   // +0x08
    int mUnknown0C;   // +0x0c: nonzero while busy.
    int mUnknown10;   // +0x10
    int mUnknown14;   // +0x14
    void *mUnknown18; // +0x18: console queue.
} DeciOutState;

// 0x00627A18
extern "C" int sceDeci2Sub00627A18(const void *pBuffer, int nLength) {
    DeciOutState *pState = (DeciOutState *)(uintptr_t)0x8e7e50;
    const unsigned char *pSource = (const unsigned char *)pBuffer;
    int nRemaining = nLength;
    int nCount = 0;
    int nTotal = 0;
    void *pDest;
    int nResult;

    if (pState->mUnknown0C != 0) {
        return -1;
    }
    SpinDisableInterrupts();
    pDest = (void *)((uintptr_t)0x8e7e80u | 0x20000000u);
    pState->mUnknown0C = 1;
    *(void **)((uintptr_t)0x8e7e80u | 0x20000000u | 0x10u) = pDest;
    for (;;) {
        int nByte;
        if (nRemaining == -1) {
            break;
        }
        nByte = *pSource;
        if (nByte == 0xa) {
            *(unsigned char *)pDest = 0xd;
            nCount++;
            if (nCount >= 0x100) {
                break;
            }
            pDest = (void *)((uintptr_t)pDest + 1u);
            continue;
        }
        *(unsigned char *)pDest = (unsigned char)nByte;
        nCount++;
        pSource++;
        pDest = (void *)((uintptr_t)pDest + 1u);
        if (nCount < 0x100) {
            nRemaining--;
            nTotal++;
            continue;
        }
        break;
    }
    *(int *)((uintptr_t)0x8e7e50u + 4u) = nCount + 0xc;
    nResult =
        sceDeci2Sub0061D868((void *)(uintptr_t)pState->mUnknown00,
                            *(volatile unsigned char *)((uintptr_t)0x8e7e80u | 0x20000000u | 7u));
    if (nResult < 0) {
        ReenableInterrupts();
        pState->mUnknown0C = 0;
        return -1;
    }
    for (;;) {
        if (pState->mUnknown0C == 0) {
            break;
        }
        sceDeci2Sub0061D898((void *)(uintptr_t)pState->mUnknown00);
        if (pState->mUnknown0C != 0) {
            continue;
        }
        break;
    }
    ReenableInterrupts();
    return nTotal;
}
// 0x00627B68
extern "C" int sceDeci2Sub00627B68(void *pBuffer, int nLength);

namespace {

// The request code for the file service control path.
constexpr int kFsIoctlFunction = 5;
// The send image covers 0x400 bytes of argument data.
constexpr int kFsIoctlSendSize = 0x400;
constexpr int kFsIoctlRpcSize = 0x420;
constexpr int kFsIoctlReceiveSize = 4;
// Values returned on the failure paths.
constexpr int kFsNoClient = -9;
constexpr int kFsRpcFailed = -11;
// Request values with dedicated handling.
constexpr int kFsRequestAllocHandle = 1;
constexpr int kFsRequestLastResult = 2;
constexpr int kFsRequestLastResultWide = 3;
// Handle table size scanned for a free slot.
constexpr int kFsHandleCount = 0x20;
// The client entry stride is 0x10 bytes.
constexpr int kFsClientStride = 0x10;

// The SIF RPC client as used by the file and memory card paths.
typedef struct {
    void *mUnknown00;            // +0x00
    int mUnknown04;              // +0x04
    int mUnknown08;              // +0x08
    unsigned char mUnknown0C[8]; // +0x0C
    int mUnknown14;              // +0x14
    unsigned char mUnknown18[4]; // +0x18
    void *mUnknown1C;            // +0x1C
    int mUnknown20;              // +0x20
    int mUnknown24;              // +0x24
} SifRpcClient;

// The SIF RPC packet queued for the IOP.
typedef struct {
    unsigned char mUnknown00[0x14]; // +0x00
    void *mUnknown14;               // +0x14
    int mUnknown18;                 // +0x18
    void *mUnknown1C;               // +0x1C
    int mUnknown20;                 // +0x20
    int mUnknown24;                 // +0x24
    void *mUnknown28;               // +0x28
    int mUnknown2C;                 // +0x2C
    int mUnknown30;                 // +0x30
    int mUnknown34;                 // +0x34
} SifRpcPacket;

// One file service client entry. The table stride is 0x10 bytes.
typedef struct {
    unsigned char mUnknown00[4]; // +0x00
    int mUnknown04;              // +0x04
    unsigned char mUnknown08[8]; // +0x08
} FsClientEntry;

// The 0x400 byte send image as passed by the caller.
typedef struct {
    unsigned char mBytes[0x400]; // +0x00
} FsSendImage;

// The send image viewed as quadwords for the aligned path.
typedef struct {
    unsigned long long mWords[0x80]; // +0x00
} FsSendImageWide;

// The handle table at 0x762B88 in the image.
typedef struct {
    int mHandles[0x20]; // +0x00
} FsHandleTable;

// The execution result at 0x8E3590 in the image.
typedef struct {
    union {
        unsigned int mWord;       // +0x00
        unsigned long long mWide; // +0x00
    } mUnknown00;
} FsExecResult;

// The word destination for the last result request.
typedef struct {
    unsigned int mUnknown00; // +0x00
} FsWordDest;

// The wide destination for the wide result request.
typedef struct {
    unsigned long long mUnknown00; // +0x00
} FsWideDest;

// The saved argument word cleared when no handle is free.
typedef struct {
    int mUnknown00; // +0x00
} FsSavedWord;

// The control packet at 0x8E2900 in the image.
typedef struct {
    int mUnknown00;                  // +0x00
    void *mUnknown04;                // +0x04
    int mUnknown08;                  // +0x08
    int mUnknown0C;                  // +0x0C
    int mUnknown10;                  // +0x10
    unsigned char mUnknown14[0x400]; // +0x14
    int mUnknown414;                 // +0x414
    int mUnknown418;                 // +0x418
    int mUnknown41C;                 // +0x41C
} FsIoctlPacket;

// The semaphore parameter block built on the stack.
typedef struct {
    int mUnknown00;               // +0x00
    int mUnknown04;               // +0x04
    int mUnknown08;               // +0x08
    unsigned char mUnknown0C[12]; // +0x0C
    int mUnknown18;               // +0x18
} SemaParam;

// The memory card initialisation receive area at 0x8E1740.
typedef struct {
    int mUnknown00; // +0x00
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
} McInitReceive;

// Addresses from the disassembly.
constexpr uintptr_t kFsIoctlPacketAddress = 0x8E2900U;
constexpr uintptr_t kFsIoctlArgAddress = 0x8E28C0U;
constexpr uintptr_t kFsInitialisedAddress = 0x762C08U;
constexpr uintptr_t kFsClientBaseAddress = 0x8E39C0U;
constexpr uintptr_t kFsExecResultAddress = 0x8E3590U;
constexpr uintptr_t kFsHandleBaseAddress = 0x762B88U;
constexpr uintptr_t kFsReceiveAddress = 0x8E3540U;
constexpr uintptr_t kFsClientAddress = 0x8E3BC0U;
constexpr uintptr_t kSifPoolAddress = 0x8E0140U;
constexpr uintptr_t kMcSemaAddress = 0x761734U;
constexpr uintptr_t kMcClientAddress = 0x8E0180U;
constexpr uintptr_t kMcSendAddress = 0x8E0200U;
constexpr uintptr_t kMcReceiveAddress = 0x8E1740U;

FsIoctlPacket *FsIoctlPacketBlock(void) {
    return reinterpret_cast<FsIoctlPacket *>(kFsIoctlPacketAddress);
}

void **FsIoctlArgSlot(void) {
    return reinterpret_cast<void **>(kFsIoctlArgAddress);
}

int *FsInitialisedFlag(void) {
    return reinterpret_cast<int *>(kFsInitialisedAddress);
}

FsHandleTable *FsHandleTableBlock(void) {
    return reinterpret_cast<FsHandleTable *>(kFsHandleBaseAddress);
}

FsExecResult *FsExecResultBlock(void) {
    return reinterpret_cast<FsExecResult *>(kFsExecResultAddress);
}

} // namespace

// Forward declarations for helpers defined at the end of this file.
extern "C" void sceFileioRpcRoutine0056a4f0(void);
extern "C" void sceFileioRpcRoutine0056aa08(void);
extern "C" int sceSifCheckStatRpc(void *pClient);
extern "C" int sceMcSifRpcRoutine005663A0(int nValue);
extern "C" int sceDeci2Sub0061D7F8(int nKind, void *pControl, void *pName);
extern "C" int sceDeci2Sub0061D868(void *pPacket, int nChar);
extern "C" int sceDeci2Sub0061D898(void *pPacket);
extern "C" void LibkFlushCache(void);

// 0x0056A5D8
extern "C" void *sceFileioRpcRoutine0056A5D8(int nFile) {
    sceFileioRpcRoutine0056a4f0();
    LibkWaitSema(*(volatile int *)(uintptr_t)0x762c10);
    if ((unsigned int)nFile < 0x20u) {
        LibkSignalSema(*(volatile int *)(uintptr_t)0x762c10);
        return &FsClientTable()[nFile];
    }
    LibkSignalSema(*(volatile int *)(uintptr_t)0x762c10);
    return NULL;
}
// 0x0056AA58
extern "C" int sceFileioRpcRoutine0056AA58(int nValue) {
    (void)nValue;
    sceFileioRpcRoutine0056aa08();
    LibkWaitSema(*(volatile int *)(uintptr_t)0x762c0c);
    return 0;
}
// 0x0056AA98
extern "C" int sceFileioRpcRoutine0056AA98(void) {
    int nBound;
    unsigned i;
    uintptr_t sendWord;
    int nRpc;
    unsigned v;

    sceSifInitRpc(0);
    SpinDisableInterrupts();
    sceSifAddCmdHandler(0x80000011, (void *)(uintptr_t)0x56a648, (void *)(uintptr_t)0x8e3c00);
    ReenableInterrupts();
    nBound = sceSifBindRpc((void *)(uintptr_t)0x8e3bc0, 0x80000001, 0);
    if (nBound < 0) {
        return -1;
    }
    if (*(volatile int *)(uintptr_t)0x8e3be4 == 0) {
        // A micro-delay the image spins through unconditionally on this path.
        volatile int nSpin = 0;
        do {
            nSpin--;
        } while (nSpin != -1);
    }
    sceFileioRpcRoutine0056a4f0();
    LibkWaitSema(*(volatile int *)(uintptr_t)0x762c10);
    for (i = 0; i < 0x20; ++i) {
        FsClientTable()[i].mUnknown04 = 0;
    }
    LibkSignalSema(*(volatile int *)(uintptr_t)0x762c10);
    sendWord = 0x8e3580u;
    nRpc = sceSifCallRpcInternal((void *)(uintptr_t)0x8e3bc0,
                                 0xff,
                                 0,
                                 &sendWord,
                                 4,
                                 (void *)(uintptr_t)0x8e3540,
                                 4,
                                 NULL,
                                 0);
    if (nRpc < 0) {
        return -2;
    }
    // The reply reads back through the uncached segment for coherence.
    v = *(volatile unsigned *)((uintptr_t)0x8e3540 | 0x20000000u);
    *(volatile unsigned *)(uintptr_t)0x8e3be8 = v;
    *(volatile int *)(uintptr_t)0x762c08 = 1;
    return 0;
}
// 0x0056AA88
extern "C" int sceFileioRpcRoutine0056AA88(void) {
    return LibkSignalSema(*(volatile int *)(uintptr_t)0x762c0c);
}
// 0x00564C50
extern "C" void *sceMcSifRpcRoutine00564C50(void *pPool);
// 0x00564CF8
extern "C" void sceMcSifRpcRoutine00564CF8(void *pPacket);
// 0x00564A88
extern "C" void sceSifInitRpc(int nMode);
// 0x005650F8
extern "C" int sceSifBindRpc(void *pClient, int nRpcId, int nMode);
// 0x005663E8
extern "C" int sceMcSync(int nMode, int *pnCmd, int *pnResult);
// 0x00536AE0
extern "C" int LibkCreateSema(void *pParam) {
    register void *pBlock __asm__("a0") = pParam;
    register int nNumber __asm__("v1") = 0x40;
    register int nResult __asm__("v0");

    __asm__ volatile("syscall" : "=r"(nResult) : "r"(nNumber), "r"(pBlock) : "memory");
    return nResult;
}
// 0x00536AF0
extern "C" int LibkDeleteSema(int nSema) {
    register int nId __asm__("a0") = nSema;
    register int nNumber __asm__("v1") = 0x41;
    register int nResult __asm__("v0");

    __asm__ volatile("syscall" : "=r"(nResult) : "r"(nNumber), "r"(nId) : "memory");
    return nResult;
}
// 0x00536B00
extern "C" int LibkSignalSema(int nSema) {
    register int nId __asm__("a0") = nSema;
    register int nNumber __asm__("v1") = 0x42;
    register int nResult __asm__("v0");

    __asm__ volatile("syscall" : "=r"(nResult) : "r"(nNumber), "r"(nId) : "memory");
    return nResult;
}
// 0x00536B20
extern "C" int LibkWaitSema(int nSema) {
    register int nId __asm__("a0") = nSema;
    register int nNumber __asm__("v1") = 0x44;
    register int nResult __asm__("v0");

    __asm__ volatile("syscall" : "=r"(nResult) : "r"(nNumber), "r"(nId) : "memory");
    return nResult;
}
// 0x005369d0
extern "C" int LibkGetThreadId(void) {
    register int nNumber __asm__("v1") = 0x2f;
    register int nResult __asm__("v0");

    __asm__ volatile("syscall" : "=r"(nResult) : "r"(nNumber) : "memory");
    return nResult;
}
// 0x00536860
extern "C" int LibkSetAlarm(int nA0, int nA1, int nA2) {
    register int nReg0 __asm__("a0") = nA0;
    register int nReg1 __asm__("a1") = nA1;
    register int nReg2 __asm__("a2") = nA2;
    register int nNumber __asm__("v1") = 0xfc;
    register int nResult __asm__("v0");

    __asm__ volatile("syscall"
                     : "=r"(nResult)
                     : "r"(nNumber), "r"(nReg0), "r"(nReg1), "r"(nReg2)
                     : "memory");
    return nResult;
}
// 0x00536a00
extern "C" int LibkSleepThread(void) {
    register int nNumber __asm__("v1") = 0x32;
    register int nResult __asm__("v0");

    __asm__ volatile("syscall" : "=r"(nResult) : "r"(nNumber) : "memory");
    return nResult;
}
// 0x00536f10
extern "C" int LibkDeci2Call(int nCommand, void *pPacket) {
    register int nCmd __asm__("a0") = nCommand;
    register void *pPkt __asm__("a1") = pPacket;
    register int nNumber __asm__("v1") = 0x7c;
    register int nResult __asm__("v0");

    __asm__ volatile("syscall" : "=r"(nResult) : "r"(nNumber), "r"(nCmd), "r"(pPkt) : "memory");
    return nResult;
}
// 0x005D3BF0
extern "C" void sceSifWriteBackDCache(void *pAddress, int nSize);
// 0x005D3A48
extern "C" int
sceSifSendCmd(int nCommand, void *pPacket, int nSize, void *pSend, int nUnk1, int nUnk2);

// Sema packet built on the stack for creation.
typedef struct {
    int mUnknown00; // +0x00
    int mUnknown04; // +0x04: set to one.
    int mUnknown08; // +0x08: set to one.
    int mUnknown0C; // +0x0c
    int mUnknown10; // +0x10
    int mUnknown14; // +0x14: cleared.
} FsSemaPacket;

// 0x0056A4F0
extern "C" void sceFileioRpcRoutine0056A4F0(void) {
    FsSemaPacket packet = {0, 1, 1, 0, 0, 0};

    if (*(volatile int *)(uintptr_t)0x762c10 != -1) {
        return;
    }
    *(volatile int *)(uintptr_t)0x762c10 = LibkCreateSema(&packet);
    *(volatile int *)(uintptr_t)0x762c14 = LibkCreateSema(&packet);
}

// 0x0056AA08
extern "C" void sceFileioRpcRoutine0056AA08(void) {
    FsSemaPacket packet = {0, 1, 1, 0, 0, 0};

    if (*(volatile int *)(uintptr_t)0x762c0c != -1) {
        return;
    }
    *(volatile int *)(uintptr_t)0x762c0c = LibkCreateSema(&packet);
}

// Client record table at 0x8E39C0 in the image, thirty-two sixteen-byte records.
typedef struct {
    int mUnknown00; // +0x00
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0C; // +0x0c
} FsClientRecord;

static FsClientRecord *FsClientTable(void) {
    return (FsClientRecord *)(uintptr_t)0x8e39c0U;
}

// Console output queue at 0x8E7D40 in the image.
typedef struct {
    void *mUnknown00; // +0x00: queued character.
    int mUnknown04;   // +0x04: pending count.
    void *mUnknown08; // +0x08: read cursor.
    void *mUnknown0C; // +0x0c: write cursor.
} DeciQueue;

// 0x006277D8
extern "C" void *sceDeci2Sub006277D8(int nValue) {
    DeciQueue *pQueue = (DeciQueue *)(uintptr_t)0x8e7d40;

    pQueue->mUnknown00 = (void *)(uintptr_t)nValue;
    pQueue->mUnknown08 = (void *)((uintptr_t)pQueue + 0x10u);
    pQueue->mUnknown04 = 0;
    pQueue->mUnknown0C = (void *)((uintptr_t)pQueue + 0x10u);
    return pQueue;
}

// 0x00627840
extern "C" void sceDeci2Sub00627840(void *pQueue) {
    DeciQueue *pState = (DeciQueue *)pQueue;
    int nCount = pState->mUnknown04 - 1;
    void *pRead = (void *)((uintptr_t)pState->mUnknown08 + 1u);
    void *pLimit = (void *)((uintptr_t)pState + (uintptr_t)pState->mUnknown00 + 0x10u);

    pState->mUnknown04 = nCount;
    // The cursor store falls in the branch delay slot, so it lands in both cases.
    pState->mUnknown08 = pRead;
    if (pRead == pLimit) {
        pState->mUnknown08 = (void *)((uintptr_t)pState + 0x10u);
    }
}

// Client pair checked by the status query.
typedef struct {
    void *mUnknown00; // +0x00: target or null.
    int mUnknown04;   // +0x04: expected word.
} RpcStatClient;

// Status target with flag words.
typedef struct {
    unsigned char mReserved00[0x10]; // +0x00
    int mUnknown10;                  // +0x10: flag bit.
    int mUnknown14;                  // +0x14
    int mUnknown18;                  // +0x18: compared word.
} RpcStatTarget;

// 0x005654B8
extern "C" int sceSifCheckStatRpc(void *pClient) {
    RpcStatClient *pState = (RpcStatClient *)pClient;
    RpcStatTarget *pTarget;

    if (pState->mUnknown00 == NULL) {
        return 0;
    }
    pTarget = (RpcStatTarget *)pState->mUnknown00;
    if (pState->mUnknown04 != pTarget->mUnknown18) {
        return 0;
    }
    if ((pTarget->mUnknown10 & 1) == 0) {
        return 0;
    }
    return 1;
}

// 0x005663A0
extern "C" int sceMcSifRpcRoutine005663A0(int nValue) {
    int nId = nValue & 0xffff;
    int nThread = LibkGetThreadId();

    LibkSetAlarm(nId, 0x566378, nThread);
    return LibkSleepThread();
}

// 0x00596480
extern "C" int LibcConsoleWrite(int nFile, const void *pBuffer, int nLength) {
    int *pInitialised = ConsoleInitialisedFlag();
    const void *pSource = pBuffer;
    int nCount = nLength;

    // Only standard output and standard error reach the debug channel.
    if (static_cast<unsigned>(nFile - 1) >= 2U) {
        return -1;
    }
    if (*pInitialised == 0) {
        if (sceDeci2Sub00627C38(pSource) == 0) {
            return -1;
        }
        *pInitialised = 1;
    }
    return sceDeci2Sub00627A18(pSource, nCount);
}

// 0x00596500
extern "C" int LibcConsoleRead(int nFile, void *pBuffer, int nLength) {
    int *pInitialised = ConsoleInitialisedFlag();
    void *pDest = pBuffer;
    int nCount = nLength;

    // Only standard input reaches the debug channel.
    if (nFile != 0) {
        return -1;
    }
    if (*pInitialised == 0) {
        if (sceDeci2Sub00627C38(pDest) == 0) {
            return -1;
        }
        *pInitialised = 1;
    }
    return sceDeci2Sub00627B68(pDest, nCount);
}

// 0x005965A0
extern "C" int LibcConsoleClose(int nFile) {
    (void)nFile;
    // Console handles cannot be closed. The behaviour is fixed.
    return -1;
}

// 0x005965B0
extern "C" int LibcConsoleLseek(int nFile, int nOffset, int nOrigin) {
    (void)nFile;
    (void)nOffset;
    (void)nOrigin;
    // Console handles cannot be repositioned. The behaviour is fixed.
    return -1;
}

// 0x00596668
extern "C" int LibcConsoleIsatty(int nFile) {
    (void)nFile;
    // Console handles are always terminals. The behaviour is fixed.
    return 1;
}

// 0x005652C8
extern "C" int sceSifCallRpcInternal(void *pClient,
                                     int nFunction,
                                     int nMode,
                                     void *pSend,
                                     int nSendSize,
                                     void *pReceive,
                                     int nReceiveSize,
                                     void *pExtra,
                                     int nReserved) {
    SifRpcClient *pRpcClient = static_cast<SifRpcClient *>(pClient);
    int nFunc = nFunction;
    int nMd = nMode;
    void *pSnd = pSend;
    int nSndSize = nSendSize;
    void *pRcv = pReceive;
    int nRcvSize = nReceiveSize;
    void *pExt = pExtra;
    int nRes = nReserved;
    SifRpcPacket *pPacket;
    int nClientWord;
    int nSendLen;
    int nReceiveLen;

    pPacket = static_cast<SifRpcPacket *>(
        sceMcSifRpcRoutine00564C50(reinterpret_cast<void *>(kSifPoolAddress)));
    if (pPacket == nullptr) {
        return -1;
    }
    pRpcClient->mUnknown20 = nRes;
    pRpcClient->mUnknown00 = pPacket;
    pRpcClient->mUnknown04 = pPacket->mUnknown18;
    pRpcClient->mUnknown1C = pExt;
    pPacket->mUnknown20 = nFunc;
    pPacket->mUnknown24 = nSndSize;
    pPacket->mUnknown28 = pRcv;
    pPacket->mUnknown2C = nRcvSize;
    pPacket->mUnknown14 = pPacket;
    pPacket->mUnknown1C = pRpcClient;
    nClientWord = pRpcClient->mUnknown24;
    pPacket->mUnknown34 = nClientWord;
    if ((nMd & 2) == 0) {
        if (pSnd != pRcv) {
            int nSmaller = nSndSize;
            if (nSndSize >= nRcvSize) {
                nSmaller = nRcvSize;
            }
            nSendLen = nSmaller;
            nReceiveLen = nSmaller;
            sceSifWriteBackDCache(pSnd, nSendLen);
            (void)nReceiveLen;
        } else {
            if (nSndSize > 0) {
                sceSifWriteBackDCache(pSnd, nSndSize);
            }
            if (nRcvSize > 0) {
                sceSifWriteBackDCache(pRcv, nRcvSize);
            }
        }
    }
    if ((nMd & 1) != 0) {
        int nFlag = 1;
        if (pExt == nullptr) {
            pPacket->mUnknown30 = 0;
            nFlag = 1;
        } else {
            pPacket->mUnknown30 = nFlag;
        }
        pRpcClient->mUnknown08 = -1;
        nClientWord = pRpcClient->mUnknown14;
        if (sceSifSendCmd(0x8000000A, pPacket, 0x40, pSnd, nClientWord, nSndSize) != 0) {
            return 0;
        }
        sceMcSifRpcRoutine00564CF8(pPacket);
        return -2;
    }
    {
        SemaParam nSemaParam = {};
        int nSema;
        int nSendResult;

        nSemaParam.mUnknown04 = 1;
        nSemaParam.mUnknown00 = 0;
        nSemaParam.mUnknown08 = 0;
        nSemaParam.mUnknown18 = 0;
        nSema = LibkCreateSema(&nSemaParam);
        if (nSema < 0) {
            pRpcClient->mUnknown08 = nSema;
            sceMcSifRpcRoutine00564CF8(pPacket);
            return -3;
        }
        pPacket->mUnknown30 = 1;
        pRpcClient->mUnknown08 = -1;
        nClientWord = pRpcClient->mUnknown14;
        nSendResult = sceSifSendCmd(0x8000000A, pPacket, 0x40, pSnd, nClientWord, nSndSize);
        if (nSendResult != 0) {
            LibkDeleteSema(nSema);
            return -2;
        }
        LibkWaitSema(nSema);
        LibkDeleteSema(nSema);
        return 0;
    }
}

// 0x005659E8
extern "C" int sceMcInitLibrary(void) {
    int *pSema = reinterpret_cast<int *>(kMcSemaAddress);
    int nSema = *pSema;
    SemaParam nParam = {};
    void *pClient = reinterpret_cast<void *>(kMcClientAddress);
    void *pSend = reinterpret_cast<void *>(kMcSendAddress);
    McInitReceive *pReceive = reinterpret_cast<McInitReceive *>(kMcReceiveAddress);
    int nBound;
    int nResult;

    if (nSema < 0) {
        nParam.mUnknown04 = 1;
        nParam.mUnknown00 = 0;
        nParam.mUnknown08 = 0;
        nParam.mUnknown18 = 0;
        nSema = LibkCreateSema(&nParam);
        *pSema = nSema;
    }
    sceMcSync(0, nullptr, nullptr);
    LibkWaitSema(nSema);
    sceSifInitRpc(0);
    for (;;) {
        // The bind uses the memory card server identifier.
        nBound = sceSifBindRpc(pClient, 0x80000400, 0);
        if (nBound >= 0) {
            break;
        }
        {
            volatile unsigned int nSpin = 0x10000000U;
            while (nSpin != 0U) {
                nSpin--;
            }
        }
    }
    for (;;) {
        SifRpcClient *pBoundClient = static_cast<SifRpcClient *>(pClient);
        if (pBoundClient->mUnknown24 != 0) {
            break;
        }
        {
            volatile unsigned int nSpin = 0x10000000U;
            while (nSpin != 0U) {
                nSpin--;
            }
        }
    }
    nResult = sceSifCallRpcInternal(pClient, 0xFE, 0, pSend, 0x30, pReceive, 0x0C, nullptr, 0);
    LibkSignalSema(nSema);
    if (nResult < 0) {
        SifRpcClient *pBoundClient = static_cast<SifRpcClient *>(pClient);
        pBoundClient->mUnknown24 = 0;
        return nResult - 0x64;
    }
    if (pReceive->mUnknown04 < 0x20A) {
        if (pReceive->mUnknown04 == -0x78) {
            return pReceive->mUnknown04;
        }
        LogPrintf("libmc: too old release of mcserv.irx\n");
        SifRpcClient *pBoundClient = static_cast<SifRpcClient *>(pClient);
        pBoundClient->mUnknown24 = 0;
        return pReceive->mUnknown04;
    }
    if (pReceive->mUnknown08 < 0x20E) {
        LogPrintf("libmc: too old release of mcman.irx\n");
        SifRpcClient *pBoundClient = static_cast<SifRpcClient *>(pClient);
        pBoundClient->mUnknown24 = 0;
        return -0x79;
    }
    return pReceive->mUnknown00;
}

// 0x0056B870
extern "C" int sceIoctl(int nFile, int nRequest, void *pArg) {
    FsIoctlPacket *pPacket = FsIoctlPacketBlock();
    void **pArgSlot = FsIoctlArgSlot();
    int *pInitialised = FsInitialisedFlag();
    void *pRequestArg = pArg;
    int nReq = nRequest;
    void *pClient;
    int nSemaCreate;
    int nHandleIndex;
    FsHandleTable *pHandleTable = FsHandleTableBlock();
    int *pReceive = reinterpret_cast<int *>(kFsReceiveAddress);
    void *pRpcClient = reinterpret_cast<void *>(kFsClientAddress);
    FsExecResult *pExecResult = FsExecResultBlock();
    SemaParam nParam = {};
    int nSema;
    int nRpcResult;

    pClient = sceFileioRpcRoutine0056A5D8(nFile);
    nSemaCreate = sceFileioRpcRoutine0056AA58(kFsIoctlFunction);
    (void)nSemaCreate;
    *pArgSlot = pRequestArg;
    if (*pInitialised == 0) {
        sceFileioRpcRoutine0056AA98();
    }
    if (pClient == nullptr) {
        sceFileioRpcRoutine0056AA88();
        return kFsNoClient;
    }
    {
        FsClientEntry *pEntry = static_cast<FsClientEntry *>(pClient);
        if (pEntry->mUnknown04 == 0) {
            sceFileioRpcRoutine0056AA88();
            return kFsNoClient;
        }
        pPacket->mUnknown414 = 0;
    }
    if (nReq == kFsRequestLastResult) {
        unsigned int nValue = pExecResult->mUnknown00.mWord;
        FsWordDest *pDest = static_cast<FsWordDest *>(pRequestArg);
        pDest->mUnknown00 = nValue;
        sceFileioRpcRoutine0056AA88();
        return 0;
    }
    pPacket->mUnknown418 = 0;
    if (nReq < kFsRequestLastResultWide) {
        if (nReq == kFsRequestAllocHandle) {
            int nSemaId = LibkWaitSema(*reinterpret_cast<int *>(static_cast<uintptr_t>(0x762C14U)));
            (void)nSemaId;
            nHandleIndex = 0;
            while (nHandleIndex < kFsHandleCount) {
                if (pHandleTable->mHandles[nHandleIndex] == -1) {
                    break;
                }
                nHandleIndex++;
            }
            if (nHandleIndex == kFsHandleCount) {
                void **pSaved = FsIoctlArgSlot();
                FsSavedWord *pWord = static_cast<FsSavedWord *>(*pSaved);
                pWord->mUnknown00 = 0;
                LibkSignalSema(*reinterpret_cast<int *>(static_cast<uintptr_t>(0x762C14U)));
                sceFileioRpcRoutine0056AA88();
                return 0;
            }
            pHandleTable->mHandles[nHandleIndex] = 1;
            LibkSignalSema(*reinterpret_cast<int *>(static_cast<uintptr_t>(0x762C14U)));
            sceFileioRpcRoutine0056AA88();
            return 0;
        }
        FsClientEntry *pEntry = static_cast<FsClientEntry *>(pClient);
        pPacket->mUnknown0C = pEntry->mUnknown00[0];
        (void)pEntry;
    } else if (nReq == kFsRequestLastResultWide) {
        unsigned long long nWide = pExecResult->mUnknown00.mWide;
        FsWideDest *pDestWide = static_cast<FsWideDest *>(pRequestArg);
        pDestWide->mUnknown00 = nWide;
        sceFileioRpcRoutine0056AA88();
        return 0;
    }
    {
        FsClientEntry *pEntry = static_cast<FsClientEntry *>(pClient);
        pPacket->mUnknown10 = nReq;
        pPacket->mUnknown0C = pEntry->mUnknown00[0];
        (void)pEntry;
    }
    if (pRequestArg != nullptr) {
        FsSendImage *pSrcImage = static_cast<FsSendImage *>(pRequestArg);
        FsSendImageWide *pSrcWide = static_cast<FsSendImageWide *>(pRequestArg);
        uintptr_t nSrcAddr = reinterpret_cast<uintptr_t>(pRequestArg);
        uintptr_t nDestAddr = reinterpret_cast<uintptr_t>(&pPacket->mUnknown14[0]);
        int nUnaligned = static_cast<int>((nSrcAddr | nDestAddr) & 7U);
        if (nUnaligned != 0) {
            int nOffset = 0;
            pPacket->mUnknown41C = kFsIoctlSendSize;
            while (nOffset < kFsIoctlSendSize) {
                int nByte = 0;
                while (nByte < 32) {
                    int nPos = nOffset + nByte;
                    pPacket->mUnknown14[nPos] = pSrcImage->mBytes[nPos];
                    nByte++;
                }
                nOffset += 32;
            }
        } else {
            FsSendImageWide *pDestWide =
                reinterpret_cast<FsSendImageWide *>(&pPacket->mUnknown14[0]);
            int nWord = 0;
            while (nWord < 0x80) {
                pDestWide->mWords[nWord] = pSrcWide->mWords[nWord];
                pDestWide->mWords[nWord + 1] = pSrcWide->mWords[nWord + 1];
                pDestWide->mWords[nWord + 2] = pSrcWide->mWords[nWord + 2];
                pDestWide->mWords[nWord + 3] = pSrcWide->mWords[nWord + 3];
                nWord += 4;
            }
        }
    } else {
        pPacket->mUnknown41C = 0;
    }
    nParam.mUnknown04 = 1;
    nParam.mUnknown00 = 0;
    nParam.mUnknown08 = 0;
    nParam.mUnknown18 = 0;
    nSema = LibkCreateSema(&nParam);
    pPacket->mUnknown00 = nSema;
    pPacket->mUnknown04 = reinterpret_cast<void *>(static_cast<uintptr_t>(0x8E3540U + 0x00U));
    pPacket->mUnknown08 = kFsIoctlReceiveSize;
    sceSifWriteBackDCache(pPacket, kFsIoctlRpcSize);
    nRpcResult = sceSifCallRpcInternal(pRpcClient,
                                       kFsIoctlFunction,
                                       0,
                                       pPacket,
                                       kFsIoctlRpcSize,
                                       pReceive,
                                       kFsIoctlReceiveSize,
                                       nullptr,
                                       0);
    if (nRpcResult < 0) {
        LibkDeleteSema(nSema);
        sceFileioRpcRoutine0056AA88();
        return kFsRpcFailed;
    }
    {
        int nStored = *pReceive;
        sceFileioRpcRoutine0056AA88();
        if (nStored == 0) {
            LibkDeleteSema(nSema);
            return kFsRpcFailed;
        }
        LibkWaitSema(nSema);
        LibkDeleteSema(nSema);
        return *reinterpret_cast<int *>(static_cast<uintptr_t>(0x8E3540U + 0x30U));
    }
}

// 0x00536D60
extern "C" void LibkFlushCache(void) {
    register int nNumber __asm__("v1") = 0x64;

    __asm__ volatile("syscall" : : "r"(nNumber) : "memory");
}

// 0x0061D7F8
extern "C" int sceDeci2Sub0061D7F8(int nKind, void *pControl, void *pName) {
    struct {
        int mKind;
        void *mControl;
        void *mName;
        void *mBase;
    } packet;

    packet.mKind = nKind & 0xffff;
    packet.mControl = pControl;
    packet.mName = pName;
    packet.mBase = (void *)((uintptr_t)0x8e6950u | 0x20000000u);
    return LibkDeci2Call(1, &packet);
}

// 0x0061D868
extern "C" int sceDeci2Sub0061D868(void *pPacket, int nChar) {
    struct {
        void *mPacket;
        int mChar;
    } packet;

    packet.mPacket = pPacket;
    packet.mChar = (signed char)nChar;
    return LibkDeci2Call(3, &packet);
}

// 0x0061D898
extern "C" int sceDeci2Sub0061D898(void *pPacket) {
    void *slot = pPacket;

    return LibkDeci2Call(4, &slot);
}

// Console state block at 0x8E7E50 in the image.
typedef struct {
    int mUnknown00;   // +0x00: negative while initialised.
    int mUnknown04;   // +0x04
    int mUnknown08;   // +0x08
    int mUnknown0C;   // +0x0c
    int mUnknown10;   // +0x10
    int mUnknown14;   // +0x14
    void *mUnknown18; // +0x18: console queue.
} DeciState;

// Uncached register block for console setup.
typedef struct {
    unsigned char mReserved00[2]; // +0x00
    unsigned short mUnknown02;    // +0x02: cleared.
    unsigned short mUnknown04;    // +0x04: set to 0x210.
    unsigned char mUnknown06;     // +0x06: set to 0x45.
    unsigned char mUnknown07;     // +0x07: set to 0x48.
    int mUnknown08;               // +0x08: cleared.
} DeciRegs;

// 0x00627C38
extern "C" int sceDeci2Sub00627C38(const void *pBuffer) {
    DeciState *pState = (DeciState *)(uintptr_t)0x8e7e50;
    volatile DeciRegs *pRegs = (volatile DeciRegs *)((uintptr_t)0x8e7e80u | 0x20000000u);

    (void)pBuffer;
    LibkFlushCache();
    if (pState->mUnknown00 < 0) {
        return 0;
    }
    if (sceDeci2Sub0061D7F8(0x210, pState, (void *)(uintptr_t)0x627880) < 0) {
        return 0;
    }
    pState->mUnknown0C = 0;
    pState->mUnknown04 = 0;
    pState->mUnknown08 = 0;
    pState->mUnknown14 = (void *)(uintptr_t)0x8e7fc0u | 0x20000000u;
    pState->mUnknown10 = (void *)(uintptr_t)0x8e7e80u | 0x20000000u;
    pRegs->mUnknown02 = 0;
    pRegs->mUnknown04 = 0x210;
    pRegs->mUnknown06 = 0x45;
    pRegs->mUnknown07 = 0x48;
    pRegs->mUnknown08 = 0;
    pState->mUnknown18 = sceDeci2Sub006277D8(0x100);
    return 1;
}

// Pool of 0x40-byte blocks the memory card RPC layer allocates from.
typedef struct {
    int mUnknown00;   // +0x00: allocation count.
    void *mUnknown04; // +0x04: first block.
    int mUnknown08;   // +0x08: block limit.
} McSifPool;

// One pooled block with status words.
typedef struct {
    unsigned char mReserved00[0x10]; // +0x00
    int mUnknown10;                  // +0x10: busy flag bit.
    void *mUnknown14;                // +0x14: self pointer.
    int mUnknown18;                  // +0x18
    unsigned char mReserved1C[0x24]; // +0x1c
} McSifBlock;

// 0x00564C50
extern "C" void *sceMcSifRpcRoutine00564C50(void *pPool) {
    McSifPool *pool = (McSifPool *)pPool;
    McSifBlock *block;
    int v1 = 0;

    SpinDisableInterrupts();
    if (pool->mUnknown08 <= 0) {
        ReenableInterrupts();
        return NULL;
    }
    block = (McSifBlock *)pool->mUnknown04;
    for (;;) {
        if ((block->mUnknown10 & 1) == 0) {
            int count = pool->mUnknown00;
            block->mUnknown10 = (v1 << 16) | 5;
            v1 = count + 1;
            pool->mUnknown00 = v1;
            if (v1 == 1) {
                pool->mUnknown00 = count + 2;
                v1 = 1;
            }
            block->mUnknown14 = block;
            block->mUnknown18 = v1;
            ReenableInterrupts();
            return block;
        }
        v1++;
        block = (McSifBlock *)((uintptr_t)block + 0x40u);
        if (v1 >= 1) {
            ReenableInterrupts();
            return NULL;
        }
    }
}

// 0x00564CF8
extern "C" void sceMcSifRpcRoutine00564CF8(void *pPacket) {
    McSifBlock *block = (McSifBlock *)pPacket;

    block->mUnknown18 = 0;
    block->mUnknown10 &= 0xfffffffeu;
}

// 0x005663E8
extern "C" int sceMcSync(int nMode, int *pnCmd, int *pnResult) {
    int nStat;
    int bReady;

    if (*(volatile int *)(uintptr_t)0x761730 == 0) {
        return -1;
    }
    nStat = sceSifCheckStatRpc((void *)(uintptr_t)0x8e0180);
    if (nMode == 0 && nStat != 0) {
        do {
            sceMcSifRpcRoutine005663A0(0x3c);
            nStat = sceSifCheckStatRpc((void *)(uintptr_t)0x8e0180);
        } while (nStat != 0);
        nStat = 0;
    }
    bReady = (nStat < 1);
    if (pnCmd != NULL) {
        *pnCmd = *(volatile int *)(uintptr_t)0x761730;
    }
    if (bReady == 0) {
        return 0;
    }
    *(volatile int *)(uintptr_t)0x761730 = 0;
    if (pnResult != NULL) {
        *pnResult = *(volatile int *)(uintptr_t)0x8e1740;
    }
    LibkSignalSema(*(volatile int *)(uintptr_t)0x761734);
    return bReady;
}

// 0x0056B340
extern "C" int sceFileioRpcRoutine0056B340(int nFile, void *pBuffer, int nSize) {
    void *pClient;
    int nResult;

    (void)pBuffer;
    (void)nSize;
    pClient = sceFileioRpcRoutine0056A5D8(nFile);
    if (pClient == NULL) {
        return -9;
    }
    nResult = sceFileioRpcRoutine0056AA58(2);
    if (nResult != 0) {
        return -11;
    }
    return 0;
}

// Sema packet built on the stack for creation.
typedef struct {
    int mUnknown00; // +0x00
    int mUnknown04; // +0x04: set to one.
    int mUnknown08; // +0x08: set to one.
    int mUnknown0C; // +0x0c
    int mUnknown10; // +0x10
    int mUnknown14; // +0x14: cleared.
} FsSemaPacket;

// 0x0056A4F0
extern "C" void sceFileioRpcRoutine0056a4f0(void) {
    FsSemaPacket packet = {0, 1, 1, 0, 0, 0};

    if (*(volatile int *)(uintptr_t)0x762c10 != -1) {
        return;
    }
    *(volatile int *)(uintptr_t)0x762c10 = LibkCreateSema(&packet);
    *(volatile int *)(uintptr_t)0x762c14 = LibkCreateSema(&packet);
}

// 0x0056AA08
extern "C" void sceFileioRpcRoutine0056aa08(void) {
    FsSemaPacket packet = {0, 1, 1, 0, 0, 0};

    if (*(volatile int *)(uintptr_t)0x762c0c != -1) {
        return;
    }
    *(volatile int *)(uintptr_t)0x762c0c = LibkCreateSema(&packet);
}

// Pool of 0x40-byte blocks the memory card RPC layer allocates from.
typedef struct {
    int mUnknown00;   // +0x00: allocation count.
    void *mUnknown04; // +0x04: first block.
    int mUnknown08;   // +0x08: block limit.
} McSifPool;

// One pooled block with status words.
typedef struct {
    unsigned char mReserved00[0x10]; // +0x00
    int mUnknown10;                  // +0x10: busy flag bit.
    void *mUnknown14;                // +0x14: self pointer.
    int mUnknown18;                  // +0x18
    unsigned char mReserved1C[0x24]; // +0x1c
} McSifBlock;

// 0x00564C50
extern "C" void *sceMcSifRpcRoutine00564C50(void *pPool) {
    McSifPool *pool = (McSifPool *)pPool;
    McSifBlock *block;
    int v1 = 0;

    SpinDisableInterrupts();
    if (pool->mUnknown08 <= 0) {
        ReenableInterrupts();
        return NULL;
    }
    block = (McSifBlock *)pool->mUnknown04;
    for (;;) {
        if ((block->mUnknown10 & 1) == 0) {
            int count = pool->mUnknown00;
            block->mUnknown10 = (v1 << 16) | 5;
            v1 = count + 1;
            pool->mUnknown00 = v1;
            if (v1 == 1) {
                pool->mUnknown00 = count + 2;
                v1 = 1;
            }
            block->mUnknown14 = block;
            block->mUnknown18 = v1;
            ReenableInterrupts();
            return block;
        }
        v1++;
        block = (McSifBlock *)((uintptr_t)block + 0x40u);
        if (v1 >= 1) {
            ReenableInterrupts();
            return NULL;
        }
    }
}

// 0x00564CF8
extern "C" void sceMcSifRpcRoutine00564CF8(void *pPacket) {
    McSifBlock *block = (McSifBlock *)pPacket;

    block->mUnknown18 = 0;
    block->mUnknown10 &= 0xfffffffeu;
}

// 0x005663E8
extern "C" int sceMcSync(int nMode, int *pnCmd, int *pnResult) {
    int nStat;
    int bReady;

    if (*(volatile int *)(uintptr_t)0x761730 == 0) {
        return -1;
    }
    nStat = sceSifCheckStatRpc((void *)(uintptr_t)0x8e0180);
    if (nMode == 0 && nStat != 0) {
        do {
            sceMcSifRpcRoutine005663A0(0x3c);
            nStat = sceSifCheckStatRpc((void *)(uintptr_t)0x8e0180);
        } while (nStat != 0);
        nStat = 0;
    }
    bReady = (nStat < 1);
    if (pnCmd != NULL) {
        *pnCmd = *(volatile int *)(uintptr_t)0x761730;
    }
    if (bReady == 0) {
        return 0;
    }
    *(volatile int *)(uintptr_t)0x761730 = 0;
    if (pnResult != NULL) {
        *pnResult = *(volatile int *)(uintptr_t)0x8e1740;
    }
    LibkSignalSema(*(volatile int *)(uintptr_t)0x761734);
    return bReady;
}

// 0x00627B68
extern "C" int sceDeci2Sub00627B68(void *pBuffer, int nLength) {
    DeciOutState *pState = (DeciOutState *)(uintptr_t)0x8e7e50;
    unsigned char *pDest = (unsigned char *)pBuffer;
    int nLeft = nLength;
    int nIndex = 0;

    if (nLeft <= 0) {
        return 0;
    }
    for (;;) {
        int nByte;
        DeciQueue *pQueue;
        DeciQueue *pEntry;
        nIndex++;
        pQueue = *(DeciQueue **)(uintptr_t)0x8e7e68;
        while (pQueue->mUnknown04 == 0) {
        }
        pEntry = pState->mUnknown18;
        pEntry = *(DeciQueue **)pEntry;
        nByte = *(unsigned char *)pEntry->mUnknown08;
        *pDest = (unsigned char)nByte;
        sceDeci2Sub00627840(pEntry);
        nByte = *pDest;
        if (nByte == 0xa) {
            return nIndex;
        }
        // A count of ten with any other byte ends input, which mirrors the image comparing the
        // count rather than reloading the byte.
        if (nIndex == 0xa) {
            return nIndex;
        }
        if (nIndex < nLeft) {
            pDest++;
            continue;
        }
        return nIndex;
    }
}
