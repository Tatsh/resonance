#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <eekernel.h>
#include <ezmpeg.h>
#include <ezmpeg/vibuf.h>
#include <libdma.h>

#include "os/log.h"

// Sony libdma is reproduced here from the disassembly.
// Register words match the disassembly packing.

// The control register occupies 0x1000E000. Bit zero enables transfers.
#define DMAC_CTRL (*(volatile unsigned int *)(uintptr_t)0x1000E000U)
// The status register occupies 0x1000E010. Low bits report channel interrupts.
#define DMAC_STAT (*(volatile unsigned int *)(uintptr_t)0x1000E010U)
// The protocol control register occupies 0x1000E020. The put path writes combined halfwords.
#define DMAC_PCR (*(volatile unsigned int *)(uintptr_t)0x1000E020U)
// The stall control register occupies 0x1000E030. The put path writes combined halfwords.
#define DMAC_SQWC (*(volatile unsigned int *)(uintptr_t)0x1000E030U)
// The register at 0x1000E040 receives word sixteen from the put path.
#define DMAC_REG_E040 (*(volatile unsigned int *)(uintptr_t)0x1000E040U)
// The register at 0x1000E050 receives word twelve from the put path.
#define DMAC_REG_E050 (*(volatile unsigned int *)(uintptr_t)0x1000E050U)
// The image transfer out channel control word occupies 0x1000B400.
#define IPU_TO_CHCR (*(volatile unsigned int *)(uintptr_t)0x1000B400U)
// The image transfer out channel address word occupies 0x1000B410.
#define IPU_TO_MADR (*(volatile unsigned int *)(uintptr_t)0x1000B410U)
// The image transfer out channel count word occupies 0x1000B420.
#define IPU_TO_QWC (*(volatile unsigned int *)(uintptr_t)0x1000B420U)
// The image transfer out channel tag word occupies 0x1000B430.
#define IPU_TO_TADR (*(volatile unsigned int *)(uintptr_t)0x1000B430U)
// The image transfer in channel control word occupies 0x1000B000.
#define IPU_FROM_CHCR (*(volatile unsigned int *)(uintptr_t)0x1000B000U)
// The image transfer in channel address word occupies 0x1000B010.
#define IPU_FROM_MADR (*(volatile unsigned int *)(uintptr_t)0x1000B010U)
// The image transfer in channel count word occupies 0x1000B020.
#define IPU_FROM_QWC (*(volatile unsigned int *)(uintptr_t)0x1000B020U)
// The image processing unit command word occupies 0x10002000.
#define IPU_CMD (*(volatile unsigned int *)(uintptr_t)0x10002000U)
// The image processing unit control word occupies 0x10002010.
#define IPU_CTRL (*(volatile unsigned int *)(uintptr_t)0x10002010U)
// The image processing unit buffer pointer occupies 0x10002020.
#define IPU_BP (*(volatile unsigned int *)(uintptr_t)0x10002020U)
// The interrupt enabler word occupies 0x1000F520.
#define DMA_ENABLER (*(volatile unsigned int *)(uintptr_t)0x1000F520U)
// The interrupt enable word occupies 0x1000F590.
#define DMA_ENABLEW (*(volatile unsigned int *)(uintptr_t)0x1000F590U)
// The put path indexes this table with the opening byte.
// 0x0077fc70
static const unsigned char FIRST_TABLE[] = {0, 0, 0, 3, 0, 1, 0, 0, 2, 0};
// The put path indexes this table with the second byte.
// 0x0077fc80
static const unsigned char SECOND_TABLE[] = {0, 1, 2, 0, 0, 0, 3, 0, 0, 0};
// The put path indexes this table with the third byte.
// 0x0077fc90
static const unsigned char THIRD_TABLE[] = {0, 2, 3, 0, 0, 0, 0, 0, 0, 0};

// Spin budgets bound the busy waits.
enum {
    kBusySpin = 0xFFFFFF,
    kSyncDefault = 0x1000000,
    kChannelCount = 10,
    kSectorShift = 11,
    kTagStride = 0x10,
    kStampStride = 0x18
};

// Channel register block. Offsets match the image. Gaps preserve the hardware spacing.
typedef struct {
    volatile unsigned int mChcr; // The control word resides at offset 0x00.
    unsigned char mUnknown04[12]; // Padding occupies offsets 0x04 to 0x0F.
    volatile unsigned int mMadr; // The address word resides at offset 0x10.
    unsigned char mUnknown14[12]; // Padding occupies offsets 0x14 to 0x1F.
    volatile unsigned int mQwc; // The count word resides at offset 0x20.
    unsigned char mUnknown24[12]; // Padding occupies offsets 0x24 to 0x2F.
    volatile unsigned int mTadr; // The tag word resides at offset 0x30.
    unsigned char mUnknown34[12]; // Padding occupies offsets 0x34 to 0x3F.
    volatile unsigned int mUnknown40; // The word resides at offset 0x40.
    unsigned char mUnknown44[12]; // Padding occupies offsets 0x44 to 0x4F.
    volatile unsigned int mUnknown50; // The word resides at offset 0x50.
    unsigned char mUnknown54[44]; // Padding occupies offsets 0x54 to 0x7F.
    volatile unsigned int mUnknown80; // The word resides at offset 0x80.
} DmaChannelRegs;

// The register block of each channel, VIF0 through the scratchpad input.
// 0x0077fc08
static DmaChannelRegs *g_apDmacChannelRegs[kChannelCount] = {
    (DmaChannelRegs *)(uintptr_t)0x10008000U,
    (DmaChannelRegs *)(uintptr_t)0x10009000U,
    (DmaChannelRegs *)(uintptr_t)0x1000A000U,
    (DmaChannelRegs *)(uintptr_t)0x1000B000U,
    (DmaChannelRegs *)(uintptr_t)0x1000B400U,
    (DmaChannelRegs *)(uintptr_t)0x1000C000U,
    (DmaChannelRegs *)(uintptr_t)0x1000C400U,
    (DmaChannelRegs *)(uintptr_t)0x1000C800U,
    (DmaChannelRegs *)(uintptr_t)0x1000D000U,
    (DmaChannelRegs *)(uintptr_t)0x1000D400U};

// Whether the reset clears each channel. The three SIF channels are excluded, as the IOP link
// runs over them.
// 0x0077fc48
static int g_anDmacChannelEnabled[kChannelCount] = {1, 1, 1, 1, 1, 0, 0, 0, 1, 1};

// The environment sceDmaPutEnv() last applied, which sceDmaGetEnv() copies out.
// 0x0077fca0
static sceDmaEnv g_dmaSavedEnv;

// Raw environment view. Offsets match the image. Members expose every byte the put path validates.
typedef struct {
    unsigned char mByte00; // The opening byte resides at offset 0x00.
    unsigned char mByte01; // The second byte resides at offset 0x01.
    unsigned char mByte02; // The third byte resides at offset 0x02.
    unsigned char mByte03; // The fourth byte resides at offset 0x03.
    unsigned short mHalf04; // The halfword resides at offset 0x04.
    unsigned short mHalf06; // The channel mask resides at offset 0x06.
    unsigned short mHalf08; // The halfword resides at offset 0x08.
    unsigned short mHalf0A; // The halfword resides at offset 0x0A.
    unsigned int mWord0C; // The word resides at offset 0x0C.
    unsigned int mWord10; // The word resides at offset 0x10.
} DmaEnvRaw;

// Queue record. Offsets match the video input buffer. Saved words preserve channel positions.
typedef struct {
    unsigned char *mData; // The ring base resides at offset 0x00.
    unsigned int mTagBase; // The tag base resides at offset 0x04.
    int mCapacitySectors; // The sector budget resides at offset 0x08.
    int mReadSectors; // The read position resides at offset 0x0C.
    int mBufferedSectors; // The buffered sectors reside at offset 0x10.
    int mBufferedBytes; // The buffered remainder resides at offset 0x14.
    int mSize; // The ring size resides at offset 0x18.
    int mSavedMadr; // The saved address resides at offset 0x1C.
    int mSavedTadr; // The saved tag resides at offset 0x20.
    int mSavedQwc; // The saved count resides at offset 0x24.
    int mSavedChcr; // The saved control resides at offset 0x28.
    int mSavedFromMadr; // The saved input address resides at offset 0x2C.
    int mSavedFromQwc; // The saved input count resides at offset 0x30.
    int mSavedFromChcr; // The saved input control resides at offset 0x34.
    int mSavedIpuBp; // The saved buffer pointer resides at offset 0x38.
    int mSavedIpuCtrl; // The saved control resides at offset 0x3C.
    int mSemaId; // The semaphore identifier resides at offset 0x40.
    int mUnknown44; // The active flag resides at offset 0x44.
    long long mTotalPut; // The lifetime total resides at offset 0x48.
    ViTimeStamp *mTimeStamps; // The stamp ring resides at offset 0x50.
    int mTimeStampCapacity; // The stamp capacity resides at offset 0x54.
    int mTimeStampCount; // The stamp count resides at offset 0x58.
    int mTimeStampIndex; // The stamp index resides at offset 0x5C.
} DmaQueue;

// Tag slot. Each slot spans sixteen bytes. Only the low word carries an entry.
typedef struct {
    unsigned long long mLow; // The low word resides at offset 0x00.
    unsigned long long mHigh; // The high word resides at offset 0x08.
} DmaTag;

// 0x005f36b8
// Clears a byte range one byte at a time.
static void DmaClearBytes(unsigned char *pBytes, int nCount) {
    while (nCount-- > 0) {
        *pBytes++ = 0U;
    }
}

// 0x005f36f0
// Returns the channel block for the identifier. An out of range identifier yields a null pointer.
sceDmaChan *sceDmaGetChan(int nChannel) {
    if ((unsigned int)nChannel < kChannelCount) {
        return (sceDmaChan *)g_apDmacChannelRegs[nChannel];
    }
    return NULL;
}

// 0x005f3718
// Resets every channel and returns the previous enable flag. The routine clears channel words
// and status bits and applies a cleared environment.
int sceDmaReset(int nMode) {
    unsigned int oldCtrl;
    unsigned int stat;
    int index;
    unsigned char clearEnv[20];

    oldCtrl = DMAC_CTRL;
    for (index = 0; index < kChannelCount; index++) {
        if (g_anDmacChannelEnabled[index] != 0) {
            DmaChannelRegs *regs;

            regs = g_apDmacChannelRegs[index];
            regs->mUnknown80 = 0U;
            regs->mChcr = 0U;
            regs->mTadr = 0U;
            regs->mMadr = 0U;
            regs->mUnknown50 = 0U;
            regs->mUnknown40 = 0U;
        }
    }
    DMAC_STAT = 0xFF1FU;
    stat = DMAC_STAT;
    DMAC_STAT = stat & 0xFF1F0000U;
    DmaClearBytes(clearEnv, sizeof(clearEnv));
    sceDmaPutEnv((sceDmaEnv *)clearEnv);
    if (nMode == 1) {
        unsigned int ctrl;

        ctrl = DMAC_CTRL;
        DMAC_CTRL = ctrl | 1U;
    }
    return (int)(oldCtrl & 1U);
}

// 0x005f3808
// Validates the environment and writes the controller words.
// Out of range fields produce negative results.
int sceDmaPutEnv(sceDmaEnv *pEnv) {
    DmaEnvRaw *raw;
    unsigned int ctrl;

    raw = (DmaEnvRaw *)pEnv;
    ctrl = DMAC_CTRL;
    // Yes, the binary reads the other four registers here and discards them.
    (void)DMAC_PCR;
    (void)DMAC_SQWC;
    (void)DMAC_REG_E050;
    (void)DMAC_REG_E040;
    if (raw->mByte00 > 9U) {
        return -1;
    }
    if (raw->mByte01 > 9U) {
        return -2;
    }
    if (raw->mByte02 > 9U) {
        return -3;
    }
    if (raw->mByte03 > 6U) {
        return -4;
    }
    ctrl = (ctrl & 0xFFFFFFCFU) | ((unsigned int)FIRST_TABLE[raw->mByte00] << 4);
    if (raw->mByte03 == 0U) {
        ctrl = (ctrl & 0xFFFFFF31U) | ((unsigned int)SECOND_TABLE[raw->mByte01] << 6);
        ctrl = ctrl | ((unsigned int)THIRD_TABLE[raw->mByte02] << 2);
    } else {
        ctrl = (ctrl & 0xFFFFFF33U) | ((unsigned int)SECOND_TABLE[raw->mByte01] << 6);
        ctrl = ctrl | ((unsigned int)THIRD_TABLE[raw->mByte02] << 2);
        ctrl = (ctrl & 0xFFFFFCFFU) | 2U | ((unsigned int)(raw->mByte03 - 1U) << 8);
    }
    DMAC_CTRL = ctrl;
    DMAC_PCR = ((unsigned int)raw->mHalf04 << 16) | (unsigned int)raw->mHalf06;
    DMAC_SQWC = ((unsigned int)raw->mHalf0A << 16) | (unsigned int)raw->mHalf08;
    DMAC_REG_E050 = raw->mWord0C;
    DMAC_REG_E040 = raw->mWord10;
    g_dmaSavedEnv = *pEnv;
    return 0;
}

// 0x005f39e0
// Copies the saved environment into the caller buffer and returns the caller buffer.
sceDmaEnv *sceDmaGetEnv(sceDmaEnv *pEnv) {
    *pEnv = g_dmaSavedEnv;
    return pEnv;
}

// 0x005f3a40
// Waits for the channel to idle and starts a source chain transfer. A busy channel spins
// with a timeout and reports an expiry.
void sceDmaSend(sceDmaChan *pChannel, void *pTag) {
    DmaChannelRegs *channel;
    unsigned int tag;
    int timeout;

    channel = (DmaChannelRegs *)pChannel;
    tag = (unsigned int)(uintptr_t)pTag;
    if ((channel->mChcr & 0x100U) != 0U) {
        timeout = kBusySpin;
        do {
            if (timeout < 0) {
                LogPrintf("libdma: sync timeout\n");
                if (((channel->mChcr >> 8) & 1U) != 0U) {
                    channel->mChcr &= 0xFFFFFEFFU;
                }
            }
            timeout--;
        } while ((channel->mChcr & 0x100U) != 0U);
    }
    if (channel->mTadr != 0xFFFFFFFFU) {
        channel->mTadr = tag;
    }
    channel->mQwc = 0U;
    channel->mChcr = (channel->mChcr & 0xFFFFFFF3U) | 0x105U;
}

// 0x005f3b18
// Waits for the channel to idle and starts a normal transfer. A busy channel spins with
// a timeout and reports an expiry.
void sceDmaSendN(sceDmaChan *pChannel, void *pAddress, int nQuadwords) {
    DmaChannelRegs *channel;
    unsigned int address;
    int timeout;

    channel = (DmaChannelRegs *)pChannel;
    address = (unsigned int)(uintptr_t)pAddress;
    if ((channel->mChcr & 0x100U) != 0U) {
        timeout = kBusySpin;
        do {
            if (timeout < 0) {
                LogPrintf("libdma: sync timeout\n");
                if (((channel->mChcr >> 8) & 1U) != 0U) {
                    channel->mChcr &= 0xFFFFFEFFU;
                }
            }
            timeout--;
        } while ((channel->mChcr & 0x100U) != 0U);
    }
    if (channel->mMadr != 0xFFFFFFFFU) {
        channel->mMadr = address;
    }
    channel->mQwc = (unsigned int)nQuadwords;
    channel->mChcr = (channel->mChcr & 0xFFFFFFF3U) | 0x101U;
}

// 0x005f3f90
// Polls the channel until the busy bit clears and reports zero. Probe mode returns the busy flag.
int sceDmaSync(sceDmaChan *pChannel, int nMode, int nTimeout) {
    DmaChannelRegs *channel;
    int timeout;
    unsigned int word;

    channel = (DmaChannelRegs *)pChannel;
    if (nMode == 1) {
        word = (channel->mChcr >> 8) & 1U;
        return (int)word;
    }
    timeout = nTimeout;
    if (timeout == 0) {
        timeout = kSyncDefault;
    }
    word = channel->mChcr;
    while ((word & 0x100U) != 0U) {
        timeout--;
        if (timeout < 0) {
            LogPrintf("libdma: sync timeout\n");
            if (((channel->mChcr >> 8) & 1U) != 0U) {
                channel->mChcr &= 0xFFFFFEFFU;
            }
        }
        word = channel->mChcr;
    }
    return 0;
}

// 0x00613388
// Prepares the queue record and creates the guarding semaphore.
// The routine stores the data pointer, the tag pointer, the sizes,
// and the stamp ring, then initialises the transfer path.
void sceDmaCreateQueueSemaphore(
    ViBuf *buffer, void *pData, void *pTag, int nTagSize, void *pTimeStamps, int nTimeStamps) {
    DmaQueue *queue;
    struct SemaParam sema;
    int semaId;

    queue = (DmaQueue *)buffer;
    queue->mData = (unsigned char *)pData;
    queue->mTagBase = ((unsigned int)(uintptr_t)pTag & 0x0FFFFFFFU) | 0x20000000U;
    queue->mTimeStamps = (ViTimeStamp *)pTimeStamps;
    queue->mTimeStampCapacity = nTimeStamps;
    queue->mCapacitySectors = nTagSize;
    queue->mSize = nTagSize << kSectorShift;
    sema.maxCount = 1;
    sema.initCount = 1;
    semaId = CreateSema(&sema);
    queue->mSemaId = semaId;
    sceDmaSub006126e8(buffer);
    queue->mTotalPut = 0LL;
}

// 0x00613400
// Stops the transfer channels and deletes the guarding semaphore. The routine disables interrupts
// across the channel updates.
int sceDmaDeleteQueueSemaphore(ViBuf *buffer) {
    DmaQueue *queue;
    unsigned int enabler;

    queue = (DmaQueue *)buffer;
    (void)DIntr();
    enabler = DMA_ENABLER;
    DMA_ENABLEW = enabler | 0x10000U;
    IPU_TO_CHCR = 5U;
    enabler = DMA_ENABLER;
    DMA_ENABLEW = enabler & 0xFFFEFFFFU;
    (void)EIntr();
    IPU_TO_QWC = 0U;
    IPU_TO_MADR = 0U;
    IPU_TO_TADR = 0U;
    DeleteSema(queue->mSemaId);
    return 1;
}

// 0x006126e8
// Initialises the queue counts and builds the tag ring. The routine clears the buffered counts,
// clears the stamp ring, programs the tag entries, and arms the transfer channels.
int sceDmaSub006126e8(ViBuf *buffer) {
    DmaQueue *queue;
    int count;
    int i;
    int offset;

    queue = (DmaQueue *)buffer;
    queue->mUnknown44 = 1;
    queue->mReadSectors = 0;
    queue->mBufferedSectors = 0;
    queue->mBufferedBytes = 0;
    queue->mTimeStampCount = 0;
    queue->mTimeStampIndex = 0;
    count = queue->mTimeStampCapacity;
    if (count > 0) {
        ViTimeStamp *stamps;

        stamps = queue->mTimeStamps;
        offset = 0;
        i = 0;
        do {
            i++;
            stamps->mFirst = -1LL;
            stamps->mSecond = -1LL;
            stamps->mOffset = 0;
            stamps->mSize = 0;
            stamps = (ViTimeStamp *)((unsigned char *)stamps + kStampStride);
            offset += kStampStride;
            (void)offset;
        } while (i < count);
    }
    count = queue->mCapacitySectors;
    i = 0;
    if (count > 0) {
        unsigned int base;
        DmaTag *tags;

        base = (unsigned int)(uintptr_t)queue->mData;
        tags = (DmaTag *)(uintptr_t)queue->mTagBase;
        do {
            unsigned int entry;
            unsigned long long packed;

            entry = (unsigned int)(i * 0x800) + base;
            entry &= 0x0FFFFFFFU;
            packed = (unsigned long long)entry << 32;
            packed |= 0x30000080ULL;
            tags->mLow = packed;
            tags = (DmaTag *)((unsigned char *)tags + kTagStride);
            i++;
        } while (i < count);
    }
    {
        unsigned int tagWord;
        unsigned long long packed;
        DmaTag *tags;
        DmaTag *slot;

        tagWord = queue->mTagBase;
        tagWord &= 0x0FFFFFFFU;
        packed = (unsigned long long)tagWord << 32;
        packed |= 0x20000000ULL;
        tags = (DmaTag *)(uintptr_t)queue->mTagBase;
        slot = (DmaTag *)((unsigned char *)tags + (i * kTagStride));
        slot->mLow = packed;
    }
    IPU_TO_QWC = 0U;
    IPU_TO_MADR = (unsigned int)(uintptr_t)queue->mData & 0x0FFFFFFFU;
    IPU_TO_TADR = queue->mTagBase & 0x0FFFFFFFU;
    (void)DIntr();
    {
        unsigned int enabler;

        enabler = DMA_ENABLER;
        DMA_ENABLEW = enabler | 0x10000U;
        IPU_TO_CHCR = 5U;
        enabler = DMA_ENABLER;
        DMA_ENABLEW = enabler & 0xFFFEFFFFU;
    }
    (void)EIntr();
    return 1;
}

// 0x00612890
// Advances the queue after a stall and restarts the channel when work remains. The routine
// reports an error for an inactive queue.
int sceDmaSub00612890(ViBuf *buffer) {
    DmaQueue *queue;
    unsigned int enabler;
    unsigned int chcr;
    unsigned int madr;
    unsigned int tagBase;
    unsigned int dataBase;
    int capacity;
    int readPos;
    int buffered;
    int writePos;
    int remainder;
    int tags;

    queue = (DmaQueue *)buffer;
    WaitSema(queue->mSemaId);
    if (queue->mUnknown44 == 0) {
        ErrMessage("DMA ADD not active\n");
        return 0;
    }
    (void)DIntr();
    enabler = DMA_ENABLER;
    DMA_ENABLEW = enabler | 0x10000U;
    IPU_TO_CHCR = 5U;
    enabler = DMA_ENABLER;
    DMA_ENABLEW = enabler & 0xFFFEFFFFU;
    (void)EIntr();
    chcr = IPU_TO_CHCR;
    madr = IPU_TO_MADR;
    tagBase = queue->mTagBase;
    dataBase = (unsigned int)(uintptr_t)queue->mData;
    capacity = queue->mCapacitySectors;
    if (capacity == 0) {
        __builtin_trap();
    }
    if (madr == (unsigned int)(capacity * 0x10 + (int)tagBase + 0x10) % 0x10000000U) {
        madr = 0U;
    } else {
        madr = (madr - dataBase) >> 11;
    }
    readPos = queue->mReadSectors;
    {
        int delta;

        delta = (int)(madr + (unsigned int)capacity - (unsigned int)readPos) % capacity;
        buffered = queue->mBufferedSectors - delta;
        delta = (readPos + delta) % capacity;
        queue->mBufferedSectors = buffered;
        queue->mReadSectors = delta;
        writePos = delta + buffered;
    }
    remainder = queue->mBufferedBytes;
    {
        int rounded;

        rounded = remainder + 0x7FF;
        if (remainder >= 0) {
            rounded = remainder;
        }
        rounded >>= 11;
        tags = rounded;
        queue->mBufferedBytes = remainder + rounded * -0x800;
    }
    if (tags > 0) {
        int last;

        if (capacity == 0) {
            __builtin_trap();
        }
        last = (writePos + capacity - 1) % capacity;
        {
            DmaTag *slots;
            unsigned long long packed;
            unsigned int entry;

            entry = (unsigned int)(last * 0x800) + dataBase;
            packed = (unsigned long long)entry << 32;
            packed |= 0x30000080ULL;
            slots = (DmaTag *)(uintptr_t)tagBase;
            slots[last].mLow = packed;
        }
    }
    {
        int slot;
        int done;

        slot = writePos % capacity;
        done = 0;
        if (tags > 0) {
            DmaTag *slots;

            slots = (DmaTag *)(uintptr_t)tagBase;
            do {
                unsigned long long packed;
                unsigned int entry;
                unsigned int kind;

                kind = 0U;
                if (done != tags - 1) {
                    kind = 3U;
                }
                entry = (unsigned int)(slot * 0x800) + dataBase;
                packed = (unsigned long long)entry << 32;
                packed |= (unsigned long long)kind << 28;
                packed |= 0x80ULL;
                slots[slot].mLow = packed;
                done++;
                slot = (slot + 1) % capacity;
                if (capacity == 0) {
                    __builtin_trap();
                }
            } while (done < tags);
        }
    }
    buffered = queue->mBufferedSectors;
    queue->mBufferedSectors = buffered + tags;
    if (buffered + tags != 0) {
        if (tags > 0) {
            chcr &= 0x0FFFFFFFU;
            chcr |= 0x30000000U;
        }
        (void)DIntr();
        enabler = DMA_ENABLER;
        DMA_ENABLEW = enabler | 0x10000U;
        IPU_TO_CHCR = chcr | 0x100U;
        enabler = DMA_ENABLER;
        DMA_ENABLEW = enabler & 0xFFFEFFFFU;
        (void)EIntr();
    }
    SignalSema(queue->mSemaId);
    return 1;
}

// 0x00612b40
// Stops the transfer channels and saves the hardware positions. The routine disables interrupts
// across the channel updates.
int sceDmaSub00612b40(ViBuf *buffer) {
    DmaQueue *queue;
    unsigned int enabler;
    unsigned int ctrl;

    queue = (DmaQueue *)buffer;
    WaitSema(queue->mSemaId);
    queue->mUnknown44 = 0;
    (void)DIntr();
    enabler = DMA_ENABLER;
    DMA_ENABLEW = enabler | 0x10000U;
    IPU_TO_CHCR = 5U;
    enabler = DMA_ENABLER;
    DMA_ENABLEW = enabler & 0xFFFEFFFFU;
    (void)EIntr();
    queue->mSavedMadr = (int)IPU_TO_MADR;
    queue->mSavedTadr = (int)IPU_TO_TADR;
    queue->mSavedQwc = (int)IPU_TO_QWC;
    queue->mSavedChcr = (int)IPU_TO_CHCR;
    do {
        ctrl = IPU_CTRL;
    } while ((ctrl & 0xF0U) != 0U);
    (void)DIntr();
    enabler = DMA_ENABLER;
    DMA_ENABLEW = enabler | 0x10000U;
    IPU_FROM_CHCR = 0U;
    enabler = DMA_ENABLER;
    DMA_ENABLEW = enabler & 0xFFFEFFFFU;
    (void)EIntr();
    queue->mSavedFromMadr = (int)IPU_FROM_MADR;
    queue->mSavedFromQwc = (int)IPU_FROM_QWC;
    queue->mSavedFromChcr = (int)IPU_FROM_CHCR;
    queue->mSavedIpuBp = (int)IPU_BP;
    queue->mSavedIpuCtrl = (int)IPU_CTRL;
    SignalSema(queue->mSemaId);
    return 1;
}

// 0x00612cc0
// Restarts both IPU channels from the positions sceDmaSub00612b40() saved. The input channel is
// rewound by the words the IPU FIFO had, the read position and buffered count are corrected for
// whatever the rewind crossed, the saved IPU command is reissued, and IPU_CTRL is restored.
int sceDmaSub00612cc0(ViBuf *buffer) {
    DmaQueue *queue = (DmaQueue *)buffer;
    const unsigned int savedBp = (unsigned int)queue->mSavedIpuBp;
    const unsigned int command = savedBp & 0x7FU;
    const unsigned int fifoWords = ((savedBp >> 16) & 3U) + ((savedBp >> 8) & 0xFU);
    unsigned int madr = (unsigned int)queue->mSavedMadr - (fifoWords << 4);
    unsigned int qwc = (unsigned int)queue->mSavedQwc + fifoWords;
    unsigned int chcr = (unsigned int)queue->mSavedChcr | 0x100U;
    unsigned int tadr = (unsigned int)queue->mSavedTadr;
    const unsigned int base = (unsigned int)(uintptr_t)queue->mData;
    const int capacity = queue->mCapacitySectors;
    int crossed = 0;

    WaitSema(queue->mSemaId);
    if (madr < base) {
        // The rewind extends back past the ring start, into the last sector.
        const unsigned int ringBytes = (unsigned int)capacity << kSectorShift;
        unsigned int tagId = 0;
        int remaining;

        if ((unsigned int)queue->mSavedMadr != base) {
            tagId = ((unsigned int)queue->mSavedMadr == base + ringBytes) ? 0U : 3U;
        }
        qwc = (base - madr) >> 4;
        tadr = queue->mTagBase & 0x0FFFFFFFU;
        madr += ringBytes;
        chcr = ((unsigned int)queue->mSavedChcr & 0x0FFFFFFFU) | (tagId << 28) | 0x100U;
        remaining = (capacity - queue->mReadSectors) % capacity;
        if (remaining < 0 || remaining >= queue->mBufferedSectors) {
            queue->mReadSectors = capacity - 1;
            crossed = 1;
        }
    } else {
        const unsigned int endTag =
            (((unsigned int)capacity << 4) + queue->mTagBase + 0x10U) & 0x0FFFFFFFU;
        const unsigned int savedMadr = (unsigned int)queue->mSavedMadr;
        const int savedSector = (savedMadr == endTag) ? 0 : (int)((savedMadr - base) >> kSectorShift);
        const int rewoundSector = (madr == endTag) ? 0 : (int)((madr - base) >> kSectorShift);

        if (savedSector != rewoundSector) {
            const unsigned int ringBytes = (unsigned int)capacity << kSectorShift;
            const unsigned int savedWrapped = base + (savedMadr - base) % ringBytes;
            const unsigned int nextRead =
                base +
                ((unsigned int)((queue->mReadSectors + queue->mBufferedSectors) % capacity)
                 << kSectorShift);
            const unsigned int tagId = (savedWrapped == nextRead) ? 0U : 3U;
            const int behind = (rewoundSector + capacity - queue->mReadSectors) % capacity;

            qwc = (base + ((unsigned int)savedSector << kSectorShift) - madr) >> 4;
            tadr = (((unsigned int)savedSector << 4) + queue->mTagBase) & 0x0FFFFFFFU;
            chcr = ((unsigned int)queue->mSavedChcr & 0x0FFFFFFFU) | (tagId << 28) | 0x100U;
            if (behind < 0 || behind >= queue->mBufferedSectors) {
                queue->mReadSectors = rewoundSector;
                crossed = 1;
            }
        }
    }
    if (crossed != 0) {
        ++queue->mBufferedSectors;
    }

    if (queue->mSavedFromMadr != 0 && queue->mSavedFromQwc != 0) {
        unsigned int enabler;

        IPU_FROM_MADR = (unsigned int)queue->mSavedFromMadr;
        IPU_FROM_QWC = (unsigned int)queue->mSavedFromQwc;
        (void)DIntr();
        enabler = DMA_ENABLER;
        DMA_ENABLEW = enabler | 0x10000U;
        IPU_FROM_CHCR = (unsigned int)queue->mSavedFromChcr | 0x100U;
        enabler = DMA_ENABLER;
        DMA_ENABLEW = enabler & 0xFFFEFFFFU;
        (void)EIntr();
    }
    if (queue->mBufferedSectors != 0) {
        while ((int)IPU_CTRL < 0) {
        }
        IPU_CMD = command;
        while ((int)IPU_CTRL < 0) {
        }
    }
    IPU_TO_MADR = madr;
    IPU_TO_TADR = tadr;
    IPU_TO_QWC = qwc;
    if (queue->mBufferedSectors != 0) {
        unsigned int enabler;

        (void)DIntr();
        enabler = DMA_ENABLER;
        DMA_ENABLEW = enabler | 0x10000U;
        IPU_TO_CHCR = chcr;
        enabler = DMA_ENABLER;
        DMA_ENABLEW = enabler & 0xFFFEFFFFU;
        (void)EIntr();
    }
    IPU_CTRL = (unsigned int)queue->mSavedIpuCtrl;
    queue->mUnknown44 = 1;
    SignalSema(queue->mSemaId);
    return 1;
}

// 0x00613088
// Discards the stamps the reader has passed. The routine advances the stamp ring past
// the supplied offset.
void sceDmaSub00613088(ViBuf *buffer, ViTimeStamp *timeStamp) {
    DmaQueue *queue;
    int capacity;
    int held;
    int index;
    int period;
    int valid;

    queue = (DmaQueue *)buffer;
    valid = 1;
    period = queue->mTimeStampCapacity;
    if (period == 0) {
        __builtin_trap();
    }
    index = (queue->mTimeStampIndex - queue->mTimeStampCount + period) % period;
    capacity = queue->mCapacitySectors * 0x800;
    if (capacity == 0) {
        __builtin_trap();
    }
    held = queue->mTimeStampCount;
    if ((held > 0) && (queue->mTimeStamps[index].mSize != 0)) {
        int want;

        want = timeStamp->mSize;
        while (want != 0) {
            ViTimeStamp *slot;
            int base;
            int span;

            slot = &queue->mTimeStamps[index];
            base = slot->mOffset;
            if (capacity == 0) {
                __builtin_trap();
            }
            if (((base + capacity - timeStamp->mOffset) % capacity) < want) {
                int avail;
                int total;

                if (capacity == 0) {
                    __builtin_trap();
                }
                avail = slot->mSize;
                total = (timeStamp->mOffset + want - base) % capacity;
                span = total;
                if (avail < total) {
                    span = avail;
                }
                slot->mSize = avail - span;
                slot->mOffset = (base + span) % capacity;
                if (avail - span == 0) {
                    if (slot->mFirst < 0LL) {
                        held = queue->mTimeStampCount;
                    } else {
                        slot->mSize = 0;
                        slot->mFirst = -1LL;
                        slot->mSecond = -1LL;
                        slot->mOffset = 0;
                        held = queue->mTimeStampCount;
                    }
                    held--;
                    if (held < 0) {
                        held = 0;
                    }
                    queue->mTimeStampCount = held;
                }
            } else {
                valid = 0;
            }
            index = (index + 1) % queue->mTimeStampCapacity;
            if (queue->mTimeStampCapacity == 0) {
                __builtin_trap();
            }
            if (valid == 0) {
                return;
            }
            slot = &queue->mTimeStamps[index];
            if (slot->mSize == 0) {
                return;
            }
            want = timeStamp->mSize;
        }
    }
}

// 0x006131e0
// Retrieves the stamp for the completed span.
// The routine scans the stamp ring for the matching entry.
int sceDmaSub006131e0(ViBuf *buffer, long long *pStamps) {
    DmaQueue *queue;
    unsigned int madr;
    unsigned int bp;
    unsigned int saved;
    int stride;
    int done;
    int found;
    int count;
    int trials;

    queue = (DmaQueue *)buffer;
    found = 0;
    madr = IPU_TO_MADR;
    bp = IPU_BP;
    saved = (unsigned int)queue->mSavedIpuBp;
    stride = queue->mCapacitySectors * 0x800;
    WaitSema(queue->mSemaId);
    pStamps[1] = -1LL;
    pStamps[0] = -1LL;
    if (stride == 0) {
        __builtin_trap();
    }
    done = 0;
    {
        int baseData;

        baseData = (int)(uintptr_t)queue->mData;
        count = queue->mTimeStampCount;
        trials = queue->mTimeStampIndex;
        if (count > 0) {
            int capacity;

            capacity = queue->mTimeStampCapacity;
            while (1) {
                ViTimeStamp *slots;
                int slotAddr;
                int span;
                int bias;
                int adjust;

                if (capacity == 0) {
                    __builtin_trap();
                }
                slotAddr = ((trials - count + capacity + done) % capacity) * kStampStride;
                slots = (ViTimeStamp *)((unsigned char *)queue->mTimeStamps + slotAddr);
                bias = (int)((bp >> 16 & 3U) + (bp >> 8 & 0xFU)) * -0x10;
                adjust = (int)((saved & 0x7FU) >> 3);
                span = (int)madr + bias + adjust + stride - baseData;
                span = (int)((unsigned int)span % (unsigned int)stride);
                span += stride;
                span -= slots->mOffset;
                span %= stride;
                if (span < slots->mSize) {
                    found = 1;
                    pStamps[0] = slots->mFirst;
                    pStamps[1] = slots->mSecond;
                    slots->mFirst = -1LL;
                    slots->mSecond = -1LL;
                    {
                        int left;

                        left = queue->mTimeStampCount;
                        if (left > 1) {
                            left = 1;
                        }
                        queue->mTimeStampCount = count - left;
                    }
                }
                done++;
                if ((count <= done) || (found != 0)) {
                    break;
                }
                capacity = queue->mTimeStampCapacity;
            }
        }
    }
    SignalSema(queue->mSemaId);
    return 1;
}

// 0x00613798
// Rounds the buffered byte count up to the sector boundary. The routine waits on the semaphore
// across the update.
void sceDmaSub00613798(ViBuf *buffer) {
    DmaQueue *queue;
    int bytes;
    int rounded;
    int aligned;

    queue = (DmaQueue *)buffer;
    WaitSema(queue->mSemaId);
    bytes = queue->mBufferedBytes;
    rounded = bytes + 0x7FF;
    aligned = bytes + 0xFFE;
    if (rounded >= 0) {
        aligned = rounded;
    }
    queue->mBufferedBytes = (aligned >> 11) << 11;
    SignalSema(queue->mSemaId);
}
