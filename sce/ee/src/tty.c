
#include <deci2.h>
#include <eekernel.h>
#include <libcconsole.h>

// The DECI2 TTY the original C library writes standard output to, and the console device built on
// it.

enum {
    kTtyProtocol = 0x210,
    kTtySourceEe = 'E',
    kTtyDestinationHost = 'H',
    kTtyHeaderSize = 0xc,
    kTtyPayloadSize = 0x100,
    kTtyBufferSize = 0x140,
    kTtyQueueSize = 0x100,

    kDeci2EventRead = 1,
    kDeci2EventReadDone = 2,
    kDeci2EventWrite = 3,
    kDeci2EventWriteDone = 4,

    kConsoleInput = 0,
    kConsoleOutput = 1,
    kConsoleError = 2,
};

// A DECI2 packet, a header and the TTY payload.
typedef struct {
    unsigned short nLength;
    unsigned short nReserved;
    unsigned short nProtocol;
    unsigned char nSource;
    unsigned char nDestination;
    unsigned int nTtyId;
    unsigned char mPayload[kTtyBufferSize - kTtyHeaderSize];
} TtyPacket;

// The bytes the host sent, drained by reads.
typedef struct {
    int nCapacity;
    volatile int nCount;
    unsigned char *pHead;
    unsigned char *pTail;
    unsigned char mData[kTtyQueueSize];
} TtyQueue;

// The state the DECI2 manager receives as its option pointer. The manager reads the length and the
// send pointer, and clears the busy flag when a send completes.
typedef struct {
    int mSocket;
    int nSendLength;
    int nReceived;
    volatile int bBusy;
    unsigned char *pSend;
    unsigned char *pReceive;
    TtyQueue *pQueue;
} TtyState;

// NTSC-U/C: 0x008e7d40, PAL: 0x0092cd40
static TtyQueue g_ttyQueue;

// NTSC-U/C: 0x008e7e50, PAL: 0x0092ce50
static TtyState g_ttyState;

// NTSC-U/C: 0x008e7e80, PAL: 0x0092ce80
static TtyPacket g_ttySendPacket __attribute__((aligned(64)));

// NTSC-U/C: 0x008e7fc0, PAL: 0x0092cfc0
static TtyPacket g_ttyReceivePacket __attribute__((aligned(64)));

// NTSC-U/C: 0x0076f014, PAL: 0x007b2d74
static int g_bConsoleOpen;

// NTSC-U/C: 0x006277d8, PAL: 0x00668368
static TtyQueue *TtyQueueInit(int nCapacity) {
    g_ttyQueue.nCapacity = nCapacity;
    g_ttyQueue.pHead = g_ttyQueue.mData;
    g_ttyQueue.nCount = 0;
    g_ttyQueue.pTail = g_ttyQueue.mData;
    return &g_ttyQueue;
}

// NTSC-U/C: 0x00627800, PAL: 0x00668390
static void TtyQueuePush(TtyQueue *pQueue) {
    ++pQueue->nCount;
    ++pQueue->pTail;
    if (pQueue->pTail == &pQueue->mData[pQueue->nCapacity]) {
        pQueue->pTail = pQueue->mData;
    }
}

// NTSC-U/C: 0x00627840, PAL: 0x006683d0
static void TtyQueuePop(TtyQueue *pQueue) {
    --pQueue->nCount;
    ++pQueue->pHead;
    if (pQueue->pHead == &pQueue->mData[pQueue->nCapacity]) {
        pQueue->pHead = pQueue->mData;
    }
}

// NTSC-U/C: 0x00627880, PAL: 0x00668410
static void TtyHandler(int nEvent, int nParam, void *pOpt) {
    TtyState *pState = (TtyState *)pOpt;
    int nDone;

    switch (nEvent) {
    case kDeci2EventRead:
    case kDeci2EventReadDone:
        if (nParam != 0) {
            if ((unsigned int)(pState->nReceived + nParam) > kTtyBufferSize) {
                PrintfToSioRaw("TTY: packet size larger than expect\n");
            }
            nDone = sceDeci2ExRecv(
                pState->mSocket, pState->pReceive + pState->nReceived, (unsigned short)nParam);
            if (nDone < 0) {
                PrintfToSioRaw("TTY: receive error");
            }
            pState->nReceived += nDone; // Yes, the binary adds a failed receive's result.
        } else {
            const TtyPacket *pPacket = (const TtyPacket *)pState->pReceive;
            int i;

            for (i = kTtyHeaderSize; i < pPacket->nLength; ++i) {
                *pState->pQueue->pTail = pState->pReceive[i];
                TtyQueuePush(pState->pQueue);
            }
            pState->nReceived = 0;
        }
        break;
    case kDeci2EventWrite:
        nDone = sceDeci2ExSend(pState->mSocket, pState->pSend, (unsigned short)pState->nSendLength);
        if (nDone < 0) {
            PrintfToSioRaw("TTY: send err %d\n", nDone);
            pState->bBusy = 0;
            break;
        }
        pState->pSend += nDone;
        pState->nSendLength -= nDone;
        break;
    case kDeci2EventWriteDone:
        if (pState->nSendLength != 0) {
            PrintfToSioRaw("TTY: err ti->wlen=%08x\n", pState->nSendLength);
        }
        pState->bBusy = 0;
        break;
    default:
        break;
    }
}

// NTSC-U/C: 0x00627c38, PAL: 0x006687c8
static int TtyOpen(void) {
    TtyPacket *pSend;

    FlushCache(0);
    g_ttyState.mSocket = sceDeci2Open(kTtyProtocol, &g_ttyState, TtyHandler);
    if (g_ttyState.mSocket < 0) {
        return 0;
    }
    g_ttyState.bBusy = 0;
    g_ttyState.nSendLength = 0;
    g_ttyState.nReceived = 0;
    g_ttyState.pReceive = (unsigned char *)UNCACHED_SEG(&g_ttyReceivePacket);
    pSend = (TtyPacket *)UNCACHED_SEG(&g_ttySendPacket);
    g_ttyState.pSend = (unsigned char *)pSend;
    pSend->nProtocol = kTtyProtocol;
    pSend->nSource = kTtySourceEe;
    pSend->nDestination = kTtyDestinationHost;
    pSend->nTtyId = 0;
    pSend->nReserved = 0;
    g_ttyState.pQueue = TtyQueueInit(kTtyQueueSize);
    return 1;
}

// NTSC-U/C: 0x00627a18, PAL: 0x006685a8
// Converts each newline to a carriage return and newline, and returns the number of source bytes
// that fitted in the packet.
static int TtyWrite(const char *pBuffer, int nLength) {
    TtyPacket *pSend;
    int nOut = 0;
    int nConsumed = 0;

    if (g_ttyState.bBusy != 0) {
        return -1;
    }
    DIntr();
    pSend = (TtyPacket *)UNCACHED_SEG(&g_ttySendPacket);
    g_ttyState.bBusy = 1;
    g_ttyState.pSend = (unsigned char *)pSend;
    while (nLength-- != 0) {
        if (*pBuffer == '\n') {
            pSend->mPayload[nOut++] = '\r';
            if (nOut >= kTtyPayloadSize) {
                break;
            }
        }
        pSend->mPayload[nOut++] = (unsigned char)*pBuffer++;
        ++nConsumed;
        if (nOut >= kTtyPayloadSize) {
            break;
        }
    }
    g_ttyState.nSendLength = nOut + kTtyHeaderSize;
    pSend->nLength = (unsigned short)g_ttyState.nSendLength;
    if (sceDeci2ReqSend(g_ttyState.mSocket, (char)pSend->nDestination) < 0) {
        g_ttyState.bBusy = 0;
        EIntr();
        return -1;
    }
    while (g_ttyState.bBusy != 0) {
        sceDeci2Poll(g_ttyState.mSocket);
    }
    EIntr();
    return nConsumed;
}

// NTSC-U/C: 0x00627b68, PAL: 0x006686f8
// Returns once a newline or a carriage return arrives, or when the buffer is full.
static int TtyRead(char *pBuffer, int nLength) {
    int nRead = 0;

    while (nRead < nLength) {
        TtyQueue *pQueue = g_ttyState.pQueue;
        char c;

        while (pQueue->nCount == 0) {
        }
        c = (char)*g_ttyState.pQueue->pHead;
        pBuffer[nRead++] = c;
        TtyQueuePop(g_ttyState.pQueue);
        if (c == '\n' || c == '\r') {
            break;
        }
    }
    return nRead;
}

// NTSC-U/C: 0x00596480, PAL: 0x005d9888
int LibcConsoleWrite(int nFile, const void *pBuffer, int nLength) {
    if (nFile != kConsoleOutput && nFile != kConsoleError) {
        return -1;
    }
    if (g_bConsoleOpen == 0) {
        if (TtyOpen() == 0) {
#ifdef ENABLE_PATCHES
            // Without a DECI2 host (after an IOP reboot, or on a retail console) the original drops
            // the text. The serial port shows it instead.
            const unsigned char *pBytes = (const unsigned char *)pBuffer;
            int i;

            for (i = 0; i < nLength; ++i) {
                PutSioByte(pBytes[i]);
            }
            return nLength;
#else
            return -1;
#endif
        }
        g_bConsoleOpen = 1;
    }
    return TtyWrite((const char *)pBuffer, nLength);
}

// NTSC-U/C: 0x00596500, PAL: 0x005d9908
int LibcConsoleRead(int nFile, void *pBuffer, int nLength) {
    if (nFile != kConsoleInput) {
        return -1;
    }
    if (g_bConsoleOpen == 0) {
        if (TtyOpen() == 0) {
            return -1;
        }
        g_bConsoleOpen = 1;
    }
    return TtyRead((char *)pBuffer, nLength);
}

// NTSC-U/C: 0x005965a0, PAL: 0x005d99a8
int LibcConsoleClose(int nFile) {
    (void)nFile;
    return -1;
}

// NTSC-U/C: 0x005965b0, PAL: 0x005d99b8
int LibcConsoleLseek(int nFile, int nOffset, int nOrigin) {
    (void)nFile;
    (void)nOffset;
    (void)nOrigin;
    return -1;
}

// NTSC-U/C: 0x00596668, PAL: 0x005d9a70
int LibcConsoleIsatty(int nFile) {
    (void)nFile;
    return 1;
}

// NTSC-U/C: 0x005963d0, PAL: 0x005d97d8
void LibcConsoleReset(void) {
    g_bConsoleOpen = 0;
}
