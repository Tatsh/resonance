#include <stddef.h>
#include <stdint.h>

#include <libmpeg.h>
#include <os/spinlock.h>

// Layout facts recovered from the disassembly of the routines in this file. The decoder
// (sceMpeg) lives in the caller and points at the work area through pContext (+0x40). The work
// area holds seven callback slots (+0x0c), the stream table pointer (+0x44), and the stream
// count (+0x48), followed by decoder state and the input ring (+0x108).
enum {
    kAlignMask = 3,
    kPictureAlignMask = 0x3f,
    kPhysicalAddressMask = 0x0fffffff,
    kUncachedSegment = 0x20000000,
    kMaxStreamCallbacks = 0x40,
    kStreamEntrySize = 0x18,
    kMinWorkSize = 0x118,
    kStreamAllocSize = 0x600,
    kStreamAllocAlign = 8,
    kSlotCount = 7,
    kTableCount = 9
};

// One stream entry, 0x18 bytes. The key is compared as eight bytes, the template copies the
// eight bytes for the stream type, and the callback returns on duplicate registration.
typedef struct {
    unsigned long long key; // +0x00: combined key, compared as a pair.
    unsigned long long templateBits; // +0x08: template for the stream type.
    void *callback; // +0x10: stream callback, returned on duplicate registration.
    void *data; // +0x14: stream data.
} StreamEntry;

// One callback slot, 8 bytes. Seven slots run from +0x0c to +0x44, where the table pointer
// sits. Slots two and three start with default callbacks.
typedef struct {
    void *callback; // +0x00: slot callback; the old one returns on replacement.
    void *data; // +0x04: slot data.
} MpegSlot;

// The input ring at +0x108. The base and size bound the buffer while the write pointer doubles
// as the bump allocator cursor and the commit pointer saves it.
typedef struct {
    int mBase; // +0x00: buffer base, set at reset.
    int mSize; // +0x04: buffer size, set at reset.
    int mWrite; // +0x08: write position, bumped by allocation.
    int mCommit; // +0x0c: committed write position.
} MpegRing;

// The work area behind the decoder context pointer. Members with no observed reader keep
// placeholder titles with their offsets.
typedef struct {
    int mCompleted; // +0x00: 1 when the picture is done; cleared to arm or fail.
    int mClear; // +0x04: read as unsigned below one by the clear query.
    int mUnknown08; // +0x08: cleared by the drain path.
    MpegSlot mSlots[kSlotCount]; // +0x0c: callback slots.
    StreamEntry *mStreamTable; // +0x44: stream entries, bump-allocated from the ring.
    int mStreamCount; // +0x48: entries used; duplicates overwrite and still count.
    int mReserved4C[9]; // +0x4c: untouched at creation.
    int mUnknown70; // +0x70.
    int mUnknown74; // +0x74: untouched at creation.
    int mUnknown78; // +0x78.
    int mUnknown7C; // +0x7c.
    int mUnknown80; // +0x80: -1 at creation and on drain.
    int mUnknown84; // +0x84: untouched at creation.
    int mUnknown88; // +0x88.
    int mUnknown8C; // +0x8c.
    int mUnknown90; // +0x90.
    int mResetArgA; // +0x94: first reset value. Inferred.
    int mResetArgB; // +0x98: second reset value. Inferred.
    int mResetArgC; // +0x9c: third reset value. Inferred.
    int mReservedA0[3]; // +0xa0: untouched at creation.
    int mUnknownAC; // +0xac.
    int mPictureBusy; // +0xb0: 1 while a picture is armed. Inferred.
    int mUnknownB4; // +0xb4.
    int mUnknownB8; // +0xb8.
    int mUnknownBC; // +0xbc.
    int mUnknownC0; // +0xc0.
    int mUnknownC4; // +0xc4.
    int mUnknownC8; // +0xc8.
    int mUnknownCC; // +0xcc.
    int mUnknownD0; // +0xd0.
    int mUnknownD4; // +0xd4: compared against the compare word on the picture path.
    int mPictureAddress; // +0xd8: picture under decode. Inferred.
    int mPictureClearA; // +0xdc. Inferred.
    int mPictureClearB; // +0xe0. Inferred.
    int mPictureMode; // +0xe4. Inferred.
    int mUnknownE8; // +0xe8.
    int mUnknownF0; // +0xf0: -1 at creation, set as a pair with the next word.
    int mUnknownF4; // +0xf4: -1 at creation.
    int mUnknownF8; // +0xf8.
    int mUnknownFC; // +0xfc.
    int mUnknown100; // +0x100.
    int mUnknown104; // +0x104.
    MpegRing mRing; // +0x108: input ring.
} MpegWork;

// One pending table cleared by the flag path. Only the word at +0x28 is observed.
typedef struct {
    unsigned char mReserved00[0x28];
    int mUnknown28; // +0x28: cleared when the table is present.
} PendingTable;

// Picture handler for the dispatch inside the picture path. The table stands in for the words
// near 0x00837430, whose targets are not yet recovered, so every slot starts empty.
typedef void (*PictureHandler)(void *pDecoder);
static PictureHandler g_pictureHandlers[5];

// Scratch areas standing in for the pointer table near 0x007a2b00. Each area hosts at least
// the +0x28 word the flag path clears; the true sizes are not yet recovered.
static unsigned char g_mpegTableArea[kTableCount][0x30];
static void *g_mpegTables[kTableCount];

// The decoder instance address kept for the interrupt handlers, the end flag the drain path
// clears, and the wait, compare, and poll words the picture path shares. These stand in for
// the fixed addresses near 0x007a0000.
static void *g_decoderInstance;
static int g_decodeEndFlag;
static int g_pictureWaitFlag;
static int g_mpegCompareWord;
static int g_mpegPollFlag;

// The default callback pointers installed for slots two and three, standing in for the words
// near 0x0062dd58. Their values are not yet recovered.
static void *g_defaultSlotTwo;
static void *g_defaultSlotThree;

// Template bits copied into each new stream entry, standing in for the table near 0x007798b8.
// Word zero feeds the key and word one picks the channel shift.
static unsigned long long g_streamTemplates[10][2];

// 0x005e0ad8
void sceMpegResetRingPointers(void *pRing, void *pBase, int nSize) {
    MpegRing *ring;

    ring = (MpegRing *)pRing;
    ring->mCommit = (int)(uintptr_t)pBase;
    ring->mSize = nSize;
    ring->mBase = (int)(uintptr_t)pBase;
    ring->mWrite = (int)(uintptr_t)pBase;
}

// 0x005e0af0
int sceMpegCommitWritePointer(void *pRing) {
    MpegRing *ring;
    int value;

    ring = (MpegRing *)pRing;
    value = ring->mWrite;
    ring->mCommit = value;
    return value;
}

// 0x005e0b00
void sceMpegRewindWritePointer(void *pRing) {
    MpegRing *ring;
    int value;

    ring = (MpegRing *)pRing;
    value = ring->mCommit;
    ring->mWrite = value;
}

// 0x005e0b10
void *sceMpegCheckWorkAreaSize(void *pRing, int nNeed, int nAlign) {
    MpegRing *ring;
    unsigned int write;
    unsigned int size;
    unsigned int base;
    unsigned int aligned;
    unsigned int end;
    unsigned int needed;

    ring = (MpegRing *)pRing;
    if (nAlign == 0) {
        sceMpegRaiseError(NULL); // Format near 0x008373c8 not yet recovered.
        return NULL;
    }
    write = (unsigned int)ring->mWrite;
    size = (unsigned int)ring->mSize;
    base = (unsigned int)ring->mBase;
    aligned = ((write + (unsigned int)nAlign) - 1u) / (unsigned int)nAlign;
    end = base + size;
    aligned = aligned * (unsigned int)nAlign;
    needed = aligned + (unsigned int)nNeed;
    if (end < needed) {
        sceMpegRaiseError(NULL); // Format near 0x008373c8 not yet recovered.
        return NULL;
    }
    ring->mWrite = (int)needed;
    return (void *)(uintptr_t)aligned;
}

// 0x005ca4e0
static unsigned long long buildStreamKey(int nType, int nChannel) {
    unsigned long long discriminator;
    unsigned long long key;
    int shift;

    key = 0;
    if ((unsigned int)nType >= 10u) {
        return 0;
    }
    discriminator = g_streamTemplates[nType][1];
    if (discriminator == 0x00ff000000000000ULL) {
        shift = 0x20;
    } else {
        shift = 0x18;
    }
    key = g_streamTemplates[nType][0];
    key = key | ((unsigned long long)(unsigned int)nChannel << shift);
    return key;
}

// 0x005e08e8
void sceMpegSub005e08e8(void *pDecoder) {
    sceMpeg *decoder;
    MpegWork *work;

    decoder = (sceMpeg *)pDecoder;
    work = (MpegWork *)decoder->pContext;
    work->mCompleted = 0;
    work->mClear = 0;
    work->mUnknown08 = 0;
    decoder->frameCount = 0;
    work->mUnknown80 = -1;
    // The work word at +0xac falls in the call delay slot, so it lands before the drain body.
    work->mUnknownAC = 0;
    sceMpegSub0060ddc8(pDecoder);
    g_decodeEndFlag = 0;
    sceIpuEnableControlBitTwentyThree();
}

// Flag tables visited in binary order. Only six of the nine table slots participate.
static const int kPendingTableOrder[6] = { 0, 3, 6, 1, 4, 7 };

// 0x005e0928
int sceMpegSub005e0928(void *pDecoder) {
    int i;
    int index;

    (void)pDecoder;
    for (i = 0; i < 6; ++i) {
        index = kPendingTableOrder[i];
        if (g_mpegTables[index] != NULL) {
            ((PendingTable *)g_mpegTables[index])->mUnknown28 = 0;
        }
    }
    return 1;
}

// 0x005e0490
int sceMpegInit(void) {
    volatile unsigned int *pStatus;
    volatile unsigned int *pStatusSet;
    volatile unsigned int *pMaskA;
    volatile unsigned int *pMaskB;
    volatile unsigned int *pClearA;
    volatile unsigned int *pClearB;
    unsigned int value;

    SpinDisableInterrupts();
    pStatus = (volatile unsigned int *)(uintptr_t)0x1000f520;
    pStatusSet = (volatile unsigned int *)(uintptr_t)0x1000f590;
    value = *pStatus;
    value = value | 0x00010000u;
    *pStatusSet = value;
    pMaskA = (volatile unsigned int *)(uintptr_t)0x1000b000;
    value = *pMaskA;
    value = value & 0xfffffeffu;
    *pMaskA = value;
    pMaskB = (volatile unsigned int *)(uintptr_t)0x1000b400;
    value = *pMaskB;
    value = value & 0xfffffeffu;
    *pMaskB = value;
    value = *pStatus;
    value = value & 0xfffeffffu;
    *pStatusSet = value;
    pClearA = (volatile unsigned int *)(uintptr_t)0x1000b020;
    // The first clear falls in the call delay slot, so it lands before the reenable body.
    *pClearA = 0;
    ReenableInterrupts();
    pClearB = (volatile unsigned int *)(uintptr_t)0x1000b420;
    *pClearB = 0;
    return sceMpegSub0061da40();
}

// 0x00610268
static int initBitReader(void *pState,
                         unsigned char *pStart,
                         int nSize,
                         unsigned char *pBuffer,
                         int nBufferSize) {
    (void)pState;
    (void)pStart;
    (void)nSize;
    (void)pBuffer;
    (void)nBufferSize;
    return 0;
}

// 0x006102a0
static int peekBits(void *pState, int nBits) {
    (void)pState;
    (void)nBits;
    return 0;
}

// 0x00610448
static int skipBits(void *pState, int nBits) {
    (void)pState;
    (void)nBits;
    return 0;
}

// 0x005cab70
static int parsePackHeader(void *pState, void *pOut) {
    (void)pState;
    (void)pOut;
    return 1;
}

// 0x005cad30
static int parsePacket(void *pState, void *pOut) {
    (void)pState;
    (void)pOut;
    return 1;
}

// 0x005ca768
// The body below is not yet verified against the disassembly; only the table references have
// been repointed at the work area.
int sceMpegDemuxPssRing(sceMpeg *pMpeg,
                        unsigned char *pStart,
                        int nSize,
                        unsigned char *pBuffer,
                        int nBufferSize) {
    MpegWork *work;
    StreamEntry *table;
    int tableCount;
    unsigned char state[0x30];
    unsigned char header[0x48];
    int selected;
    int callbackIndex;
    int consumed;
    int code;
    int i;

    work = (MpegWork *)pMpeg->pContext;
    table = work->mStreamTable;
    tableCount = work->mStreamCount;
    initBitReader(state, pStart, nSize, pBuffer, nBufferSize);
    selected = 0;
    callbackIndex = 0;
    consumed = 0;
    for (i = 0; i < tableCount; ++i) {
        if (table[i].key == 0xbdff000000000000ULL) {
            selected = (int)(table[i].callback != NULL);
            callbackIndex = i;
            break;
        }
    }
    (void)selected;
    (void)callbackIndex;
    code = peekBits(state, 0x20);
    if (code == 0x1ba) {
        parsePackHeader(state, header);
        consumed = nSize;
        return consumed;
    }
    skipBits(state, 0x80);
    skipBits(state, 0x20);
    if (parsePacket(state, header) != 0) {
        consumed = nSize;
    }
    if (consumed == 0) {
        consumed = nSize;
    }
    return consumed;
}

// 0x005e08c8
int sceMpegGetContextWordZero(void *pDecoder) {
    MpegWork *work;

    work = (MpegWork *)((sceMpeg *)pDecoder)->pContext;
    return work->mCompleted;
}

// 0x005e0b90
// Only the alignment check, the wait loop, and the dispatch shape are recovered. The handler
// table targets near 0x00837430 are not, so an empty slot falls through to the wait check
// instead of dispatching.
static int decodePictureInner(void *pDecoder) {
    MpegWork *work;
    unsigned int picture;
    int state;
    int waited;

    work = (MpegWork *)((sceMpeg *)pDecoder)->pContext;
    picture = (unsigned int)work->mPictureAddress;
    if ((picture & (unsigned int)kPictureAlignMask) != 0) {
        work->mCompleted = 0;
        sceMpegReportErrorFormatted(NULL); // Format near 0x008373e8 not yet recovered.
        return -1;
    }
    g_pictureWaitFlag = 0;
    state = 1;
    waited = 0;
    do {
        if (waited != -1) {
            do {
                state = sceMpegSub0060ba60();
                if (state == 0) {
                    break;
                }
                if (g_mpegCompareWord != work->mUnknownD4) {
                    break;
                }
                if (g_mpegPollFlag == 0) {
                    break;
                }
            } while (g_mpegPollFlag != 0);
            if ((unsigned int)state < 5u && g_pictureHandlers[state] != NULL) {
                g_pictureHandlers[state](pDecoder);
            }
        }
    } while (g_pictureWaitFlag == 0);
    return 1;
}

// 0x005e07b0
int sceMpegSub005e07b0(void *pDecoder, void *pPicture, int nMode) {
    MpegWork *work;
    uintptr_t picture;

    picture = (uintptr_t)pPicture;
    picture = picture & (uintptr_t)kPhysicalAddressMask;
    picture = picture | (uintptr_t)kUncachedSegment;
    work = (MpegWork *)((sceMpeg *)pDecoder)->pContext;
    work->mPictureBusy = 1;
    work->mPictureAddress = (int)picture;
    work->mPictureMode = nMode;
    work->mPictureClearA = 0;
    // The second clear falls in the call delay slot, so it lands before the decode body.
    work->mPictureClearB = 0;
    return decodePictureInner(pDecoder);
}

// 0x005e07f8
int sceMpegSub005e07f8(void *pDecoder, void *pPicture, int nMode) {
    MpegWork *work;
    uintptr_t picture;

    picture = (uintptr_t)pPicture;
    picture = picture & (uintptr_t)kPhysicalAddressMask;
    picture = picture | (uintptr_t)kUncachedSegment;
    work = (MpegWork *)((sceMpeg *)pDecoder)->pContext;
    work->mPictureMode = nMode;
    work->mPictureAddress = (int)picture;
    work->mPictureClearA = 0;
    work->mPictureBusy = 0;
    // The second clear falls in the call delay slot, so it lands before the decode body.
    work->mPictureClearB = 0;
    return decodePictureInner(pDecoder);
}

// 0x005e0840
int sceMpegSub005e0840(void *pDecoder, void *pPicture, int nA, int nB) {
    MpegWork *work;
    uintptr_t picture;

    picture = (uintptr_t)pPicture;
    picture = picture & (uintptr_t)kPhysicalAddressMask;
    picture = picture | (uintptr_t)kUncachedSegment;
    work = (MpegWork *)((sceMpeg *)pDecoder)->pContext;
    work->mPictureClearB = nB << 4;
    work->mPictureAddress = (int)picture;
    work->mPictureMode = nA * nB;
    work->mPictureClearA = nA << 4;
    // The busy clear falls in the call delay slot, so it lands before the decode body.
    work->mPictureBusy = 0;
    return decodePictureInner(pDecoder);
}

// 0x005e0530
void *sceMpegCreateDecoderContext(void *pDecoder, void *pWork, int nWorkSize) {
    sceMpeg *decoder;
    uintptr_t work;
    uintptr_t aligned;
    int rest;
    MpegWork *context;
    MpegRing *ring;
    int i;

    decoder = (sceMpeg *)pDecoder;
    work = (uintptr_t)pWork;
    aligned = (work + (uintptr_t)kAlignMask) & ~(uintptr_t)kAlignMask;
    rest = nWorkSize - (int)(aligned - work);
    if (rest < kMinWorkSize) {
        sceMpegRaiseError(NULL); // Format near 0x008373a0 not yet recovered.
        return NULL;
    }
    context = (MpegWork *)aligned;
    ring = &context->mRing;
    sceMpegResetRingPointers(ring, (void *)(aligned + (uintptr_t)kMinWorkSize), rest - kMinWorkSize);
    decoder->mUnknown00[0] = 0;
    decoder->mUnknown00[1] = 0;
    decoder->frameCount = 0;
    decoder->mUnknown10 = -1;
    decoder->mUnknown14 = -1;
    decoder->mUnknown18 = -1;
    decoder->mUnknown1C = -1;
    decoder->mUnknown20 = 0;
    decoder->mUnknown24 = 0;
    decoder->mUnknown28 = -1;
    decoder->mUnknown2C = -1;
    decoder->mUnknown30 = -1;
    decoder->mUnknown34 = -1;
    decoder->mUnknown38 = 0;
    decoder->mUnknown3C = 0;
    decoder->pContext = context;
    context->mUnknownB4 = 0;
    context->mUnknownB8 = 0;
    context->mUnknownBC = 0;
    context->mUnknownC0 = 0;
    context->mUnknownC4 = 0;
    context->mUnknownC8 = 0;
    context->mUnknownCC = 0;
    context->mUnknownD0 = 0;
    context->mUnknownD4 = 0;
    context->mPictureAddress = 0;
    context->mPictureClearA = 0;
    context->mPictureClearB = 0;
    context->mPictureMode = 0;
    context->mUnknownE8 = 0;
    context->mUnknownF8 = 0;
    context->mSlots[0].callback = NULL;
    context->mSlots[1].callback = NULL;
    context->mSlots[4].callback = NULL;
    context->mSlots[5].callback = NULL;
    context->mSlots[6].callback = NULL;
    context->mUnknownF0 = -1;
    context->mUnknownF4 = -1;
    context->mSlots[2].callback = g_defaultSlotTwo;
    context->mSlots[3].callback = g_defaultSlotThree;
    // The second default falls in the allocator call delay slot and still lands here.
    context->mStreamTable = (StreamEntry *)sceMpegCheckWorkAreaSize(ring, kStreamAllocSize, kStreamAllocAlign);
    context->mStreamCount = 0;
    context->mUnknownFC = 0;
    context->mUnknown100 = 0;
    context->mUnknown104 = 0;
    context->mUnknown70 = 0;
    context->mUnknown78 = 0;
    context->mUnknown7C = 0;
    context->mUnknown88 = 0;
    context->mUnknown8C = 0;
    context->mUnknown90 = 0;
    context->mUnknownAC = 0;
    context->mResetArgC = -1;
    context->mPictureBusy = 1;
    g_decoderInstance = decoder;
    context->mUnknown80 = -1;
    context->mResetArgA = -1;
    context->mResetArgB = -1;
    // The last busy clear falls in the disable call delay slot and still lands here.
    sceMpegDisableIpuControlBit((void *)(uintptr_t)0x7a0000, 1);
    sceMpegSub005e08e8(decoder);
    sceMpegSub005e0928(decoder);
    for (i = 0; i < kTableCount; ++i) {
        g_mpegTables[i] = &g_mpegTableArea[i];
    }
    return (void *)(uintptr_t)sceMpegCommitWritePointer(ring);
}

// 0x005e0990
void *sceMpegSetCallbackSlot(void *pDecoder, int nSlot, void *pfnCallback, void *pData) {
    MpegWork *work;
    void *old;

    work = (MpegWork *)((sceMpeg *)pDecoder)->pContext;
    work->mSlots[nSlot].data = pData;
    old = work->mSlots[nSlot].callback;
    work->mSlots[nSlot].callback = pfnCallback;
    return old;
}

// 0x005e09b8
int sceMpegInvokeCallbackSlot(void *pDecoder, void *pEntry) {
    MpegWork *work;
    StreamEntry *entry;
    unsigned int key;
    MpegSlot *slot;
    int (*callback)(void *pDecoder, void *pEntry, void *pData);

    if (pDecoder == NULL) {
        return 0;
    }
    work = (MpegWork *)((sceMpeg *)pDecoder)->pContext;
    if (work->mStreamTable == NULL) {
        return 0;
    }
    entry = (StreamEntry *)pEntry;
    key = (unsigned int)entry->key;
    // The image scales the key with no bounds check, so the slot index is equally unchecked.
    slot = &work->mSlots[key];
    if (slot->callback == NULL) {
        return 0;
    }
    callback = (int (*)(void *, void *, void *))slot->callback;
    return callback(pDecoder, pEntry, slot->data);
}

// 0x005e0770
int sceMpegReturnOne(void *pDecoder) {
    (void)pDecoder;
    return 1;
}

// 0x005e0890
void sceMpegSub005e0890(void *pDecoder, int nArgA, int nArgB, int nArgC) {
    MpegWork *work;

    work = (MpegWork *)((sceMpeg *)pDecoder)->pContext;
    work->mResetArgC = nArgC;
    work->mResetArgA = nArgA;
    // The second value falls in the return delay slot and still lands here.
    work->mResetArgB = nArgB;
}

// 0x005e08d8
int sceMpegIsContextWordFourClear(void *pDecoder) {
    MpegWork *work;
    unsigned int value;

    work = (MpegWork *)((sceMpeg *)pDecoder)->pContext;
    value = (unsigned int)work->mClear;
    return value < 1u;
}

// 0x005caa78
void *sceMpegAddStrCallback(void *pDecoder,
                            int nType,
                            int nChannel,
                            void *pfnCallback,
                            void *pData) {
    MpegWork *work;
    StreamEntry *table;
    int count;
    unsigned long long key;
    unsigned long long templateBits;
    StreamEntry *entry;
    int index;
    void *found;

    work = (MpegWork *)((sceMpeg *)pDecoder)->pContext;
    key = buildStreamKey(nType, nChannel);
    table = work->mStreamTable;
    count = work->mStreamCount;
    index = 0;
    found = NULL;
    if (count > 0) {
        for (index = 0; index < count; ++index) {
            if (table[index].key == key) {
                found = table[index].callback;
                break;
            }
        }
    }
    if (index >= kMaxStreamCallbacks) {
        return found;
    }
    if (nType >= 0 && nType < 10) {
        templateBits = g_streamTemplates[nType][1];
    } else {
        templateBits = 0;
    }
    entry = &table[index];
    entry->key = key;
    entry->templateBits = templateBits;
    entry->callback = pfnCallback;
    entry->data = pData;
    work->mStreamCount = count + 1;
    return found;
}
