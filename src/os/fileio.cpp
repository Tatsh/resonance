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
// 0x00627A18
extern "C" int sceDeci2Sub00627A18(const void *pBuffer, int nLength);
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

// 0x0056A5D8
extern "C" void *sceFileioRpcRoutine0056A5D8(int nFile);
// 0x0056AA58
extern "C" int sceFileioRpcRoutine0056AA58(int nValue);
// 0x0056AA98
extern "C" int sceFileioRpcRoutine0056AA98(void);
// 0x0056AA88
extern "C" int sceFileioRpcRoutine0056AA88(void);
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
extern "C" int LibkCreateSema(void *pParam);
// 0x00536AF0
extern "C" int LibkDeleteSema(int nSema);
// 0x00536B00
extern "C" int LibkSignalSema(int nSema);
// 0x00536B20
extern "C" int LibkWaitSema(int nSema);
// 0x005D3BF0
extern "C" void sceSifWriteBackDCache(void *pAddress, int nSize);
// 0x005D3A48
extern "C" int
sceSifSendCmd(int nCommand, void *pPacket, int nSize, void *pSend, int nUnk1, int nUnk2);

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
