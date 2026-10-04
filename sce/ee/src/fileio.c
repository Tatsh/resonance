#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <eekernel.h>
#include <sifcmd.h>
#include <sifdev.h>
#include <sifrpc.h>

// The client of the multi-threaded file server the IOP replacement image installs. A call sends its
// packet and returns once the server accepts it. The server later writes the outcome into the
// result area by DMA and raises the completion command, whose handler copies the outcome to the
// caller and signals the caller's semaphore. The ROM file server that ps2sdk's own client expects
// does not speak this protocol.

enum {
    kFsServerId = 0x80000001,
    kFsCompletionCommand = 0x80000011,

    kFsFunctionOpen = 0,
    kFsFunctionClose = 1,
    kFsFunctionRead = 2,
    kFsFunctionWrite = 3,
    kFsFunctionLseek = 4,
    kFsFunctionIoctl = 5,
    kFsFunctionInit = 0xff,

    kFsHandleCount = 32,
    kFsPendingCount = 32,
    kFsPathSize = 0x400,
    kFsIoctlArgSize = 0x400,
    kFsWriteHeadSize = 16,
    kFsReadEdgeSize = 64,
    kFsStatSize = 0x40,
    kFsDirentSize = 0x144,
    kFsCopyLimit = 0x400,
    kFsBindDelay = 0x100000,

    // Handle flags. The client marks a handle in use, the caller's open flags join it, and two of
    // those flags change how the calls behave.
    kFsHandleInUse = 0x10000000,
    kFsHandleNoWait = 0x8000,
    kFsHandleNoWriteBack = 0x20000000,
    kFsOpenFlagMask = 0x0fffffff,

    // The requests sceIoctl() resolves locally.
    kFsIoctlPending = 1,
    kFsIoctlLastResult = 2,
    kFsIoctlLastResultWide = 3,

    // The completion kinds whose handler copies more than the result word.
    kFsKindRead = 2,
    kFsKindDirent = 11,
    kFsKindStat = 12,
    kFsKindDevctl = 23,
    kFsKindReadlink = 25,
    kFsKindIoctl2 = 26,

    kFsErrorNotBound = -1,
    kFsErrorBadHandle = -9,
    kFsErrorRpc = -11,
    kFsErrorNoHandle = -19,
    kFsErrorInitRpc = -0x10001,
    kFsErrorVersion = -0x10004,
};

// One open file. The index of the record is the descriptor the calls take.
typedef struct {
    int mIopFd;          // +0x00
    unsigned int mFlags; // +0x04
    int mReserved08;     // +0x08
    int mReserved0C;     // +0x0c
} FsHandle;

// The words every call packet starts with. The server echoes the result pointer and size into the
// completion, and the handler copies the result there.
typedef struct {
    int mSema;
    void *pResult;
    int nResultSize;
} FsCallHeader;

typedef struct {
    FsCallHeader mHeader;
    int mFlags;
    int mMode;
    char mName[kFsPathSize];
    int mHandle;
} FsOpenPacket;

typedef struct {
    FsCallHeader mHeader;
    int mIopFd;
    int mHandle;
} FsClosePacket;

typedef struct {
    FsCallHeader mHeader;
    int mIopFd;
    void *pBuffer;
    int nSize;
    int mReserved18;
    int mHandle;
} FsReadPacket;

typedef struct {
    FsCallHeader mHeader;
    int mIopFd;
    const void *pBuffer;
    int nSize;
    int nHeadSize;
    unsigned char mHead[kFsWriteHeadSize];
    int mHandle;
} FsWritePacket;

typedef struct {
    FsCallHeader mHeader;
    int mIopFd;
    int mOffset;
    int mWhence;
    int mHandle;
} FsLseekPacket;

typedef struct {
    FsCallHeader mHeader;
    int mIopFd;
    int mRequest;
    unsigned char mArg[kFsIoctlArgSize];
    // The reply buffer and its size. sceIoctl2() at 0x0056bbc0 fills them and sceIoctl()
    // clears them.
    int mReplyBuffer;
    int mReplySize;
    int nArgSize;
} FsIoctlPacket;

typedef union {
    FsCallHeader mHeader;
    FsOpenPacket mOpen;
    FsClosePacket mClose;
    FsReadPacket mRead;
    FsWritePacket mWrite;
    FsLseekPacket mLseek;
    FsIoctlPacket mIoctl;
} FsPacket;

// The completion the server writes. A negative semaphore marks a call made without waiting, whose
// pending slot the handler releases instead of signalling.
typedef struct {
    int mSema;
    int mKind;
    void *pDest;
    int nDestSize;
    union {
        unsigned char mData[0x430];
        struct {
            int mValue;
            int nHeadSize;
            int nTailSize;
            unsigned char *pHead;
            unsigned char *pTail;
            unsigned char mHead[kFsReadEdgeSize];
            unsigned char mTail[kFsReadEdgeSize];
        } mRead;
        struct {
            int mValue;
            void *pDest;
            unsigned char mData[kFsDirentSize];
        } mFixed;
        struct {
            int mValue;
            void *pDest;
            unsigned int nSize;
            unsigned char mData[kFsCopyLimit];
        } mCopy;
    } mBody;
} FsResult;

// NTSC-U/C: 0x0071ea7c, PAL: 0x007624fc, the version the library reports, the last four bytes of
// its build tag.
static const char g_abFsLibraryVersion[4] = {'2', '3', '0', '0'};

// NTSC-U/C: 0x0082cad8, PAL: 0x0086f728, a second server version the check accepts.
static const char g_abFsAltVersion[4] = {'.', '.', '.', '.'};

// NTSC-U/C: 0x00762c18, PAL: 0x007a5bb0
static const char *g_pFsAltVersion = g_abFsAltVersion;

// NTSC-U/C: 0x00762b88, PAL: 0x007a5b20, the semaphores of calls made without waiting, or -1 for
// a free slot.
static int g_anFsPending[kFsPendingCount] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

// NTSC-U/C: 0x00762c08, PAL: 0x007a5ba0
static int g_bFsBound;

// NTSC-U/C: 0x00762c0c, PAL: 0x007a5ba4, serialises calls.
static int g_nFsCallSema = -1;

// NTSC-U/C: 0x00762c10, PAL: 0x007a5ba8, guards the handle records.
static int g_nFsHandleSema = -1;

// NTSC-U/C: 0x00762c14, PAL: 0x007a5bac, guards the pending slots.
static int g_nFsPendingSema = -1;

// NTSC-U/C: 0x008e28c0, PAL: 0x00927c40
static void *g_pFsIoctlArg;

// NTSC-U/C: 0x008e2900, PAL: 0x00927c80
static FsPacket g_fsPacket __attribute__((aligned(64)));

// NTSC-U/C: 0x008e3540, PAL: 0x009288c0, the word the server returns when it accepts a call.
static int g_anFsReceive[16] __attribute__((aligned(64)));

// NTSC-U/C: 0x008e3580, PAL: 0x00928900
static FsResult g_fsResult __attribute__((aligned(64)));

// NTSC-U/C: 0x008e39c0, PAL: 0x00928d40
static FsHandle g_aFsHandles[kFsHandleCount] __attribute__((aligned(64)));

// NTSC-U/C: 0x008e3bc0, PAL: 0x00928f40
static sceSifClientData g_fsClient __attribute__((aligned(64)));

// NTSC-U/C: 0x008e3be8, PAL: 0x00928f68, the version the server reported when the client bound it.
static char g_abFsServerVersion[4];

static int FsReceivedWord(void) {
    return *(volatile int *)UNCACHED_SEG(&g_anFsReceive[0]);
}

// NTSC-U/C: 0x0056aa08, PAL: 0x005aaed0
static void FsCreateCallSema(void) {
    struct SemaParam param;

    if (g_nFsCallSema != -1) {
        return;
    }
    param.maxCount = 1;
    param.initCount = 1;
    param.option = 0;
    g_nFsCallSema = CreateSema(&param);
}

// NTSC-U/C: 0x0056a4f0, PAL: 0x005aa9b8
static void FsCreateTableSemaphores(void) {
    struct SemaParam param;

    if (g_nFsHandleSema != -1) {
        return;
    }
    param.maxCount = 1;
    param.initCount = 1;
    param.option = 0;
    g_nFsHandleSema = CreateSema(&param);
    g_nFsPendingSema = CreateSema(&param);
}

// NTSC-U/C: 0x0056aa58, PAL: 0x005aaf20
// The binary passes the function number of each call, which the lock does not read.
static int FsLock(int nFunction) {
    (void)nFunction;
    FsCreateCallSema();
    WaitSema(g_nFsCallSema);
    return 0;
}

// NTSC-U/C: 0x0056aa88, PAL: 0x005aaf50
static int FsUnlock(void) {
    return SignalSema(g_nFsCallSema);
}

// NTSC-U/C: 0x0056a550, PAL: 0x005aaa18
static FsHandle *FsAllocHandle(void) {
    FsHandle *pHandle;

    FsCreateTableSemaphores();
    WaitSema(g_nFsHandleSema);
    for (pHandle = g_aFsHandles; pHandle < &g_aFsHandles[kFsHandleCount]; ++pHandle) {
        if (pHandle->mFlags == 0) {
            pHandle->mFlags = kFsHandleInUse;
            SignalSema(g_nFsHandleSema);
            return pHandle;
        }
    }
    SignalSema(g_nFsHandleSema);
    return NULL;
}

// NTSC-U/C: 0x0056a5d8, PAL: 0x005aaaa0
static FsHandle *FsFindHandle(int nDescriptor) {
    FsCreateTableSemaphores();
    WaitSema(g_nFsHandleSema);
    if ((unsigned int)nDescriptor >= kFsHandleCount) {
        SignalSema(g_nFsHandleSema);
        return NULL;
    }
    SignalSema(g_nFsHandleSema);
    return &g_aFsHandles[nDescriptor];
}

static int FsHandleIndex(const FsHandle *pHandle) {
    return (int)(pHandle - g_aFsHandles);
}

// NTSC-U/C: 0x0056a648, PAL: 0x005aab10
static void FsCompletionHandler(void *pData, void *pArg) {
    volatile FsResult *pResult = (volatile FsResult *)UNCACHED_SEG(&g_fsResult);
    int nSema = pResult->mSema;
    int nKind = pResult->mKind;
    void *pDest = pResult->pDest;
    int nDestSize = pResult->nDestSize;
    int i;

    (void)pData;
    (void)pArg;
    if (nSema >= 0) {
        memcpy(pDest, (const void *)pResult->mBody.mData, (size_t)nDestSize);
    }
    switch (nKind) {
    case kFsKindRead:
        for (i = 0; i < pResult->mBody.mRead.nHeadSize; ++i) {
            pResult->mBody.mRead.pHead[i] = pResult->mBody.mRead.mHead[i];
        }
        for (i = 0; i < pResult->mBody.mRead.nTailSize; ++i) {
            pResult->mBody.mRead.pTail[i] = pResult->mBody.mRead.mTail[i];
        }
        break;
    case kFsKindDirent:
        memcpy(pResult->mBody.mFixed.pDest,
               (const void *)pResult->mBody.mFixed.mData,
               kFsDirentSize);
        break;
    case kFsKindStat:
        memcpy(pResult->mBody.mFixed.pDest, (const void *)pResult->mBody.mFixed.mData, kFsStatSize);
        break;
    case kFsKindDevctl:
    case kFsKindReadlink:
    case kFsKindIoctl2: {
        unsigned int nSize = pResult->mBody.mCopy.nSize;

        if (nSize > kFsCopyLimit) {
            nSize = kFsCopyLimit;
        }
        memcpy(pResult->mBody.mCopy.pDest, (const void *)pResult->mBody.mCopy.mData, nSize);
        break;
    }
    default:
        break;
    }
    if (nSema >= 0) {
        iSignalSema(nSema);
        return;
    }
    nSema = -nSema;
    for (i = 0; i < kFsPendingCount; ++i) {
        if (g_anFsPending[i] == nSema) {
            g_anFsPending[i] = -1;
            return;
        }
    }
}

// NTSC-U/C: 0x0056aa98, PAL: 0x005aaf60
static int FsBind(void) {
    FsHandle *pHandle;
    // SIF DMA moves whole quadwords from an aligned address, and the send buffer must start on
    // one. The binary places it 16 bytes into its frame.
    FsResult *pResultArea __attribute__((aligned(16))) = &g_fsResult;
    int nBound;
    int nDelay;

    sceSifInitRpc(0);
    DIntr();
    // The binary passes a buffer at 0x008e3c00 the handler never reads.
    sceSifAddCmdHandler(kFsCompletionCommand, FsCompletionHandler, NULL);
    EIntr();
    for (;;) {
        nBound = sceSifBindRpc(&g_fsClient, kFsServerId, 0);
        if (nBound < 0) {
            return -1;
        }
        if (g_fsClient.serve != NULL) {
            break;
        }
        for (nDelay = kFsBindDelay; nDelay != -1; --nDelay) {
            __asm__ volatile("nop");
        }
    }
    FsCreateTableSemaphores();
    WaitSema(g_nFsHandleSema);
    for (pHandle = g_aFsHandles; pHandle < &g_aFsHandles[kFsHandleCount]; ++pHandle) {
        pHandle->mFlags = 0;
    }
    SignalSema(g_nFsHandleSema);
    if (sceSifCallRpc(&g_fsClient,
                      kFsFunctionInit,
                      0,
                      &pResultArea,
                      sizeof(pResultArea),
                      g_anFsReceive,
                      sizeof(g_anFsReceive[0]),
                      NULL,
                      NULL) < 0) {
        return kFsErrorInitRpc;
    }
    memcpy(g_abFsServerVersion, (const void *)UNCACHED_SEG(&g_anFsReceive[0]), 4);
    g_bFsBound = 1;
    return 0;
}

// NTSC-U/C: 0x0056ac38, PAL: 0x005ab100
// Reports a mismatch only when the server matches neither accepted version and the two accepted
// versions also differ from each other.
static int FsVersionMismatch(void) {
    if (memcmp(g_abFsServerVersion, g_abFsLibraryVersion, 4) == 0) {
        return 0;
    }
    if (memcmp(g_abFsServerVersion, g_pFsAltVersion, 4) == 0) {
        return 0;
    }
    return memcmp(g_abFsLibraryVersion, g_pFsAltVersion, 4) != 0;
}

static int FsCreateCallSemaphore(void) {
    struct SemaParam param;

    param.maxCount = 1;
    param.initCount = 0;
    param.option = 0;
    return CreateSema(&param);
}

// Parks the call's semaphore in a pending slot and sends it negated. The completion handler then
// frees the slot rather than signalling the semaphore.
static void FsParkPending(void) {
    int i;

    WaitSema(g_nFsPendingSema);
    for (i = 0; i < kFsPendingCount; ++i) {
        if (g_anFsPending[i] == -1) {
            g_anFsPending[i] = g_fsPacket.mHeader.mSema;
            g_fsPacket.mHeader.mSema = -g_fsPacket.mHeader.mSema;
            break;
        }
    }
    SignalSema(g_nFsPendingSema);
}

// Sends the packet and waits for the completion. A call made without waiting returns zero once the
// server accepts it.
static int FsCall(int nFunction, int nPacketSize, int nSema, int bNoWait, const int *pResult) {
    int nAccepted;

    if (sceSifCallRpc(&g_fsClient,
                      nFunction,
                      0,
                      &g_fsPacket,
                      nPacketSize,
                      g_anFsReceive,
                      sizeof(g_anFsReceive[0]),
                      NULL,
                      NULL) < 0) {
        DeleteSema(nSema);
        FsUnlock();
        return kFsErrorRpc;
    }
    nAccepted = FsReceivedWord();
    FsUnlock();
    if (nAccepted == 0) {
        DeleteSema(nSema);
        return kFsErrorRpc;
    }
    if (bNoWait) {
        DeleteSema(nSema);
        return 0;
    }
    WaitSema(nSema);
    DeleteSema(nSema);
    return *pResult;
}

int sceOpen(const char *pszPath, int nFlags, ...) {
    va_list args;
    int nMode;
    FsHandle *pHandle;
    int nSema;
    int nAccepted;
    int nResult;
    int i;

    va_start(args, nFlags);
    nMode = va_arg(args, int);
    va_end(args);

    FsLock(kFsFunctionOpen);
    if (g_bFsBound == 0) {
        FsBind();
    }
    if (FsVersionMismatch()) {
        FsUnlock();
        return kFsErrorVersion;
    }
    pHandle = FsAllocHandle();
    if (pHandle == NULL) {
        FsUnlock();
        return kFsErrorNoHandle;
    }
    for (i = 0; i < kFsPathSize; ++i) {
        g_fsPacket.mOpen.mName[i] = pszPath[i];
        if (pszPath[i] == '\0') {
            break;
        }
    }
    if (i == kFsPathSize) {
        g_fsPacket.mOpen.mName[kFsPathSize - 1] = '\0';
    }
    g_fsPacket.mOpen.mFlags = nFlags & kFsOpenFlagMask;
    g_fsPacket.mOpen.mMode = nMode;
    g_fsPacket.mOpen.mHandle = FsHandleIndex(pHandle);
    nSema = FsCreateCallSemaphore();
    g_fsPacket.mHeader.pResult = &nResult;
    g_fsPacket.mHeader.mSema = nSema;
    g_fsPacket.mHeader.nResultSize = sizeof(nResult);
    if (sceSifCallRpc(&g_fsClient,
                      kFsFunctionOpen,
                      0,
                      &g_fsPacket,
                      sizeof(FsOpenPacket),
                      g_anFsReceive,
                      sizeof(g_anFsReceive[0]),
                      NULL,
                      NULL) < 0) {
        DeleteSema(nSema);
        FsUnlock();
        return kFsErrorRpc;
    }
    nAccepted = FsReceivedWord();
    FsUnlock();
    if (nAccepted == 0) {
        DeleteSema(nSema);
        return kFsErrorRpc;
    }
    WaitSema(nSema);
    DeleteSema(nSema);
    WaitSema(g_nFsHandleSema);
    if (nResult < 0) {
        pHandle->mFlags = 0;
        SignalSema(g_nFsHandleSema);
        return nResult;
    }
    pHandle->mFlags |= (unsigned int)nFlags;
    pHandle->mIopFd = nResult;
    SignalSema(g_nFsHandleSema);
    return FsHandleIndex(pHandle);
}

int sceClose(int nDescriptor) {
    FsHandle *pHandle = FsFindHandle(nDescriptor);
    int nSema;
    int nAccepted;
    int nResult;

    FsLock(kFsFunctionClose);
    if (g_bFsBound == 0) {
        FsUnlock();
        return kFsErrorNotBound;
    }
    if (pHandle == NULL || pHandle->mFlags == 0) {
        FsUnlock();
        return kFsErrorBadHandle;
    }
    g_fsPacket.mClose.mIopFd = pHandle->mIopFd;
    g_fsPacket.mClose.mHandle = FsHandleIndex(pHandle);
    nSema = FsCreateCallSemaphore();
    g_fsPacket.mHeader.mSema = nSema;
    g_fsPacket.mHeader.pResult = &nResult;
    g_fsPacket.mHeader.nResultSize = sizeof(nResult);
    if (sceSifCallRpc(&g_fsClient,
                      kFsFunctionClose,
                      0,
                      &g_fsPacket,
                      sizeof(FsClosePacket),
                      g_anFsReceive,
                      sizeof(g_anFsReceive[0]),
                      NULL,
                      NULL) < 0) {
        DeleteSema(nSema);
        FsUnlock();
        return kFsErrorRpc;
    }
    // The binary frees the record without its semaphore, once the server has the call.
    pHandle->mFlags = 0;
    nAccepted = FsReceivedWord();
    FsUnlock();
    if (nAccepted == 0) {
        DeleteSema(nSema);
        return kFsErrorRpc;
    }
    WaitSema(nSema);
    DeleteSema(nSema);
    return nResult >= 0 ? 0 : nResult;
}

int sceLseek(int nDescriptor, int nOffset, int nWhence) {
    FsHandle *pHandle = FsFindHandle(nDescriptor);
    unsigned int nFlags;
    int nSema;
    int nResult;

    FsLock(kFsFunctionLseek);
    if (g_bFsBound == 0) {
        FsUnlock();
        return kFsErrorNotBound;
    }
    if (pHandle == NULL || (nFlags = pHandle->mFlags) == 0) {
        FsUnlock();
        return kFsErrorBadHandle;
    }
    g_fsPacket.mLseek.mOffset = nOffset;
    g_fsPacket.mLseek.mIopFd = pHandle->mIopFd;
    g_fsPacket.mLseek.mWhence = nWhence;
    g_fsPacket.mLseek.mHandle = FsHandleIndex(pHandle);
    nSema = FsCreateCallSemaphore();
    g_fsPacket.mHeader.nResultSize = sizeof(nResult);
    g_fsPacket.mHeader.pResult = &nResult;
    g_fsPacket.mHeader.mSema = nSema;
    if ((nFlags & kFsHandleNoWait) != 0) {
        FsParkPending();
    }
    return FsCall(kFsFunctionLseek,
                  sizeof(FsLseekPacket),
                  nSema,
                  (nFlags & kFsHandleNoWait) != 0,
                  &nResult);
}

int sceRead(int nDescriptor, void *pBuffer, int nBytes) {
    FsHandle *pHandle = FsFindHandle(nDescriptor);
    unsigned int nFlags;
    int nSema;
    int nResult;

    FsLock(kFsFunctionRead);
    if (g_bFsBound == 0) {
        FsUnlock();
        return kFsErrorNotBound;
    }
    if (pHandle == NULL || (nFlags = pHandle->mFlags) == 0) {
        FsUnlock();
        return kFsErrorBadHandle;
    }
    g_fsPacket.mRead.mIopFd = pHandle->mIopFd;
    g_fsPacket.mRead.mHandle = FsHandleIndex(pHandle);
    g_fsPacket.mRead.pBuffer = pBuffer;
    g_fsPacket.mRead.nSize = nBytes;
    nSema = FsCreateCallSemaphore();
    g_fsPacket.mHeader.nResultSize = sizeof(nResult);
    g_fsPacket.mHeader.pResult = &nResult;
    g_fsPacket.mHeader.mSema = nSema;
    if ((nFlags & kFsHandleNoWait) != 0) {
        FsParkPending();
    }
    if ((nFlags & kFsHandleNoWriteBack) == 0) {
        sceSifWriteBackDCache(pBuffer, nBytes);
    }
    // 0xa4 bytes cover the read's head and tail blocks.
    sceSifWriteBackDCache(&g_fsResult, 0xa4);
    sceSifWriteBackDCache(&g_fsPacket, sizeof(FsReadPacket));
    return FsCall(kFsFunctionRead,
                  sizeof(FsReadPacket),
                  nSema,
                  (nFlags & kFsHandleNoWait) != 0,
                  &nResult);
}

int sceWrite(int nDescriptor, const void *pBuffer, int nBytes) {
    FsHandle *pHandle = FsFindHandle(nDescriptor);
    const unsigned char *pUncached;
    unsigned int nFlags;
    int nHead;
    int nSema;
    int nResult;
    int i;

    FsLock(kFsFunctionWrite);
    if (g_bFsBound == 0) {
        FsUnlock();
        return kFsErrorNotBound;
    }
    if (pHandle == NULL || (nFlags = pHandle->mFlags) == 0) {
        FsUnlock();
        return kFsErrorBadHandle;
    }
    g_fsPacket.mWrite.mIopFd = pHandle->mIopFd;
    g_fsPacket.mWrite.mHandle = FsHandleIndex(pHandle);
    g_fsPacket.mWrite.nSize = nBytes;
    g_fsPacket.mWrite.pBuffer = pBuffer;
    nSema = FsCreateCallSemaphore();
    g_fsPacket.mHeader.nResultSize = sizeof(nResult);
    g_fsPacket.mHeader.pResult = &nResult;
    g_fsPacket.mHeader.mSema = nSema;
    if ((nFlags & kFsHandleNoWait) != 0) {
        FsParkPending();
    }
    // The bytes before the first 16-byte boundary travel in the packet.
    nHead = ((uintptr_t)pBuffer & 0xf) != 0 ? 16 - (int)((uintptr_t)pBuffer & 0xf) : 0;
    if (nBytes < nHead) {
        nHead = nBytes;
    }
    if ((nFlags & kFsHandleNoWriteBack) == 0) {
        sceSifWriteBackDCache((void *)pBuffer, nBytes);
    }
    pUncached = (const unsigned char *)UNCACHED_SEG(pBuffer);
    g_fsPacket.mWrite.nHeadSize = nHead;
    for (i = 0; i < nHead; ++i) {
        g_fsPacket.mWrite.mHead[i] = pUncached[i];
    }
    return FsCall(kFsFunctionWrite,
                  sizeof(FsWritePacket),
                  nSema,
                  (nFlags & kFsHandleNoWait) != 0,
                  &nResult);
}

int sceIoctl(int nDescriptor, int nRequest, void *pArg) {
    FsHandle *pHandle = FsFindHandle(nDescriptor);
    int nSema;
    int nResult;
    int i;

    FsLock(kFsFunctionIoctl);
    g_pFsIoctlArg = pArg;
    if (g_bFsBound == 0) {
        FsBind();
    }
    if (pHandle == NULL || pHandle->mFlags == 0) {
        FsUnlock();
        return kFsErrorBadHandle;
    }
    g_fsPacket.mIoctl.mReplyBuffer = 0;
    g_fsPacket.mIoctl.mReplySize = 0;
    if (nRequest == kFsIoctlLastResult) {
        *(int *)pArg = *(volatile int *)UNCACHED_SEG(&g_fsResult.mBody.mData[0]);
        FsUnlock();
        return 0;
    }
    if (nRequest == kFsIoctlLastResultWide) {
        *(unsigned long long *)pArg =
            *(volatile unsigned long long *)UNCACHED_SEG(&g_fsResult.mBody.mData[0]);
        FsUnlock();
        return 0;
    }
    if (nRequest == kFsIoctlPending) {
        WaitSema(g_nFsPendingSema);
        for (i = 0; i < kFsPendingCount; ++i) {
            if (g_anFsPending[i] != -1) {
                break;
            }
        }
        *(int *)g_pFsIoctlArg = i != kFsPendingCount;
        SignalSema(g_nFsPendingSema);
        FsUnlock();
        return 0;
    }
    g_fsPacket.mIoctl.mIopFd = pHandle->mIopFd;
    g_fsPacket.mIoctl.mRequest = nRequest;
    if (pArg == NULL) {
        g_fsPacket.mIoctl.nArgSize = 0;
    } else {
        g_fsPacket.mIoctl.nArgSize = kFsIoctlArgSize;
        memcpy(g_fsPacket.mIoctl.mArg, pArg, kFsIoctlArgSize);
    }
    nSema = FsCreateCallSemaphore();
    g_fsPacket.mHeader.pResult = &nResult;
    g_fsPacket.mHeader.nResultSize = sizeof(nResult);
    g_fsPacket.mHeader.mSema = nSema;
    sceSifWriteBackDCache(&g_fsPacket, sizeof(FsIoctlPacket));
    return FsCall(kFsFunctionIoctl, sizeof(FsIoctlPacket), nSema, 0, &nResult);
}

int sceFsReset(void) {
    g_bFsBound = 0;
    memset(g_abFsServerVersion, 0, sizeof(g_abFsServerVersion));
    return 0;
}
