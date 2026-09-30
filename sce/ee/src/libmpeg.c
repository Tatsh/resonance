#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <eekernel.h>
#include <libmpeg.h>

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
    sceMpegCallback callback; // +0x10: stream callback, returned on duplicate registration.
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

// IPU table at 0x007a30f8, written by the disable path. Only the observed words are named.
typedef struct {
    int mUnknown00; // +0x00: base address.
    int mUnknown04; // +0x04: base + 0x1800.
    int mReserved08[78]; // +0x08.
    int mUnknown140; // +0x140: base + 0x1b00.
    int mUnknown144; // +0x144: base + 0x3300.
    int mReserved148[77]; // +0x148.
    int mUnknown280; // +0x280: cleared by the disable path.
} MpegIpuTable;
static MpegIpuTable g_mpegIpuTable;

// One sequence table, 0x68 bytes. The picture setup writes the first five words, the flag path
// clears the word at +0x28, and the rest is not yet observed.
typedef struct {
    int mUnknown00; // +0x00.
    int mUnknown04; // +0x04.
    int mUnknown08; // +0x08.
    int mUnknown0C; // +0x0c: set from the first width word.
    int mUnknown10; // +0x10: set from the second width word.
    int mReserved14[5]; // +0x14: untouched by the observed writers.
    int mUnknown28; // +0x28: cleared when the table is present.
    int mReserved2C[15]; // +0x2c: untouched by the observed writers.
} MpegSeqTable;

// Picture handler for the dispatch inside the picture path. The table stands in for the words
// near 0x00837430, whose targets are not yet recovered, so every slot starts empty.
typedef void (*PictureHandler)(void *pDecoder);
static PictureHandler g_pictureHandlers[5];

// Sequence table arenas at 0x007a2d50, nine of 0x68 bytes ending where the IPU table below
// begins. Creation points the table slots at them in order.
static MpegSeqTable g_mpegSeqAreas[kTableCount];
static MpegSeqTable *g_mpegTables[kTableCount];

// Nibble dispatch words at 0x007a3408, read in full. The last two are code addresses the image
// stores as data; no call passes through them here.
static unsigned int g_mpegNibbleTable[16] = {
    0x00000001u, 0x00000001u, 0x00000000u, 0x00000000u,
    0x00000000u, 0x00000001u, 0x00000001u, 0x00000001u,
    0x00000001u, 0x00000001u, 0x00000000u, 0xffffffffu,
    0x00000000u, 0x00000000u, 0x0060e880u, 0x0060e668u,
};

// Indirect sequence kernels at 0x007a3440, read in full. Each word is the image address of a
// variable-length decode kernel the reconstruction has not recovered yet, so the declarations
// below name them for the next wave and the table calls through them.
int sceMpegSub0060e880(void);
int sceMpegSub0060e668(void);
int sceMpegSub0060e7d0(void);
int sceMpegSub0060c138(void);
int sceMpegSub0060c2d8(void);
int sceMpegSub0060e870(void);
int sceMpegSub0060e890(void);
int sceMpegSub0060c1e8(void);
int sceMpegSub0060bd08(void);
int sceMpegSub0060e8a0(void);
typedef int (*MpegKernelFunc)(void);
static MpegKernelFunc g_mpegIndirectTable[11] = {
    sceMpegSub0060e880, sceMpegSub0060e668, sceMpegSub0060e7d0, sceMpegSub0060c138,
    sceMpegSub0060c2d8, sceMpegSub0060e870, sceMpegSub0060e880, sceMpegSub0060c1e8,
    sceMpegSub0060bd08, sceMpegSub0060e890, sceMpegSub0060e8a0,
};

// IPU words the poll cluster shares, named by address. Roles follow the observed use.
static int g_mpegIpuBase; // Word at 0x007a38b0, the base the disable path derives pointers from.
static int g_mpegIpuBusyFlag; // Word at 0x007a2b24, nonzero while an IPU command is outstanding.
static int g_nMpegIsMpeg2;    // Word at 0x007a33b0, set once a sequence extension marks MPEG-2.

// The default quantiser matrices in zigzag order. The IPU reads them by DMA when a sequence header
// does not load its own.
// 0x007a2b80
static const unsigned char g_abMpegDefaultIntraMatrix[] __attribute__((aligned(16))) = {
    8,  16, 16, 19, 16, 19, 22, 22, 22, 22, 22, 22, 26, 24, 26, 27, 27, 27, 26, 26, 26, 26,
    27, 27, 27, 29, 29, 29, 34, 34, 34, 29, 29, 29, 27, 27, 29, 29, 32, 32, 34, 34, 37, 38,
    37, 35, 35, 34, 35, 38, 38, 40, 40, 40, 48, 48, 46, 46, 56, 56, 58, 69, 69, 83};
// 0x007a2bc0
static const unsigned char g_abMpegDefaultNonIntraMatrix[] __attribute__((aligned(16))) = {
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};
static int g_mpegShiftAccum; // Word at 0x007a3398, shifted down by the poll readers.
static int g_mpegShiftBudget; // Word at 0x007a339c, compared against the poll argument.

// Sequence words the poll cluster shares, named by address.
static int g_mpeg2c78; // Word at 0x007a2c78.
static int g_mpeg2c7c; // Word at 0x007a2c7c, returned by the poll loop.
static int g_mpeg2c80; // Word at 0x007a2c80.
static int g_mpeg2c84; // Word at 0x007a2c84.
static int g_mpeg2c88; // Word at 0x007a2c88.
static int g_mpeg2c8c; // Word at 0x007a2c8c.
static int g_mpeg2c90; // Word at 0x007a2c90.
static int g_mpeg3430; // Word at 0x007a3430.
static int g_mpeg3434; // Word at 0x007a3434.
static int g_mpeg3438; // Word at 0x007a3438.
static int g_mpeg2d1c; // Word at 0x007a2d1c.
static int g_mpeg2d20; // Word at 0x007a2d20.
static int g_mpeg2d24; // Word at 0x007a2d24.
static int g_mpeg2d28; // Word at 0x007a2d28.
static int g_mpeg2d2c; // Word at 0x007a2d2c.
static int g_mpeg2d30; // Word at 0x007a2d30.
static int g_mpeg2d34; // Word at 0x007a2d34.
static unsigned long long g_mpeg3388; // Words at 0x007a3388, saved entry pair.
static unsigned long long g_mpeg3390; // Words at 0x007a3390, saved entry pair.
static int g_mpeg346c; // Word at 0x007a346c.
static int g_mpeg3470; // Word at 0x007a3470.
static int g_mpeg2d38; // Word at 0x007a2d38.
static int g_mpeg2d3c; // Word at 0x007a2d3c, cleared by the drain path.

// Sequence setup words the picture setup shares, named by address.
static int g_mpeg2c0c; // Word at 0x007a2c0c.
static int g_mpeg2c10; // Word at 0x007a2c10.
static int g_mpeg2c14; // Word at 0x007a2c14.
static int g_mpeg2c18; // Word at 0x007a2c18.
static int g_mpeg2c20; // Word at 0x007a2c20.
static int g_mpeg2c24; // Word at 0x007a2c24.
static int g_mpeg2c28; // Word at 0x007a2c28.
static int g_mpeg2c2c; // Word at 0x007a2c2c.
static int g_mpeg2c30; // Word at 0x007a2c30.
static int g_mpeg2c34; // Word at 0x007a2c34.
static int g_mpeg2c38; // Word at 0x007a2c38.
static int g_mpeg2c3c; // Word at 0x007a2c3c.
static int g_mpeg2c40; // Word at 0x007a2c40.
static int g_mpeg2c48; // Word at 0x007a2c48.
static int g_mpeg2c4c; // Word at 0x007a2c4c.
static int g_mpeg2c6c; // Word at 0x007a2c6c.
static int g_mpeg2cb4; // Word at 0x007a2cb4.
static int g_mpeg2cc8; // Word at 0x007a2cc8.
static int g_mpeg33a0; // Word at 0x007a33a0.
static int g_mpeg33a4; // Word at 0x007a33a4.

// Forward declarations for the poll cluster, whose routines call one another in an order the
// file layout does not match.
static void sceMpegSub0060bc58(void);
static int sceMpegSub0060bf70(void);
static int sceMpegSub0060e020(void);
static int sceMpegSub0060b290(unsigned int nValue);
static int sceMpegSub0060e5a8(unsigned int nCommand, const unsigned char *pMatrix);
static int sceMpegSub0060e000(MpegSeqTable *pTable, int nA, int nB);
static void sceMpegSub0060e4c0(MpegSeqTable *pT0,
                               MpegSeqTable *pT1,
                               MpegSeqTable *pT2,
                               MpegSeqTable *pT3,
                               MpegSeqTable *pT4,
                               MpegSeqTable *pT5,
                               MpegSeqTable *pT6,
                               MpegSeqTable *pT7,
                               MpegSeqTable *pT8,
                               int nA,
                               int nB,
                               int nC);
static int sceMpegSub005e0a08(void *pDecoder);
static int sceMpegSub0060b2c0(void);
static int sceMpegSub0060b368(void);
static int sceMpegSub0060b708(int nCommand);
static int sceMpegSub0060b5d0(int nArg);
static int sceMpegSub0060b988(void);
static void sceMpegSub0060bf38(void);
static void *g_decoderInstance;
static int g_mpegPollFlag;
static int g_mpegCompareWord;

// IPU register words and the watchdog limit the poll loops share.
enum {
    kIpuCommandAddress = 0x10002000,
    kIpuControlAddress = 0x10002010,
    kIpuBusyMask = 0x80004000u,
    kIpuBusyValue = 0x80000000u,
    kIpuDataReadyBit = 0x4000u,
    kIpuWatchdogLimit = 0x1389u
};

// 0x0060b820
static int sceMpegSub0060b820(int nArg) {
    volatile unsigned int *pControl;
    volatile unsigned int *pData;
    unsigned int count;
    unsigned int command;
    unsigned int index;
    int shifted;

    pControl = (volatile unsigned int *)(uintptr_t)kIpuControlAddress;
    pData = (volatile unsigned int *)(uintptr_t)kIpuCommandAddress;
    if ((*pControl & kIpuBusyMask) == kIpuBusyValue) {
        count = 0;
        for (;;) {
            if (count >= kIpuWatchdogLimit) {
                sceMpegSub005e0a08(g_decoderInstance);
                count = 0;
            }
            ++count;
            if ((*pControl & kIpuBusyMask) != kIpuBusyValue) {
                break;
            }
        }
    }
    if (g_mpegIpuBusyFlag != 0 || g_mpegShiftBudget < nArg) {
        *pData = 0x40000000u;
        g_mpegIpuBusyFlag = (int)g_mpegNibbleTable[4];
        g_mpegShiftAccum = sceMpegSub0060b368();
    }
    g_mpegShiftBudget = 0x20;
    command = (unsigned int)nArg | 0x40000000u;
    *pData = command;
    shifted = (int)((unsigned int)g_mpegShiftAccum >> ((0x20 - nArg) & 31));
    index = (command >> 28) & 0xfu;
    g_mpegIpuBusyFlag = (int)g_mpegNibbleTable[index];
    g_mpegShiftAccum = sceMpegSub0060b368();
    return shifted;
}

// 0x0060bb88
static int sceMpegSub0060bb88(void) {
    g_mpeg2c78 = sceMpegSub0060b820(0xa);
    g_mpeg2c7c = sceMpegSub0060b820(3);
    g_mpeg2c80 = sceMpegSub0060b820(0x10);
    if ((unsigned int)(g_mpeg2c7c - 2) < 2u) {
        g_mpeg2c84 = sceMpegSub0060b820(1);
        g_mpeg2c88 = sceMpegSub0060b820(3);
    }
    if (g_mpeg2c7c == 3) {
        g_mpeg2c8c = sceMpegSub0060b820(1);
        g_mpeg2c90 = sceMpegSub0060b820(3);
    }
    sceMpegSub0060bf38();
    sceMpegSub0060bc58();
    return sceMpegSub0060bf70();
}

// 0x0060c050
static void sceMpegSub0060c050(void) {
    sceMpeg *decoder;
    MpegWork *work;

    decoder = (sceMpeg *)g_decoderInstance;
    work = (MpegWork *)decoder->pContext;
    work->mUnknownE8 = 0;
    g_mpeg3430 = g_mpeg3434 + 1;
    g_mpeg3438 = 1;
    g_mpeg2d1c = sceMpegSub0060b820(1);
    g_mpeg2d20 = sceMpegSub0060b820(5);
    g_mpeg2d24 = sceMpegSub0060b820(6);
    (void)sceMpegSub0060b820(1);
    g_mpeg2d28 = sceMpegSub0060b820(6);
    g_mpeg2d2c = sceMpegSub0060b820(6);
    g_mpeg2d30 = sceMpegSub0060b820(1);
    g_mpeg2d34 = sceMpegSub0060b820(1);
    sceMpegSub0060bc58();
}

// 0x0060bf38
static void sceMpegSub0060bf38(void) {
    for (;;) {
        if (sceMpegSub0060b820(1) == 0) {
            return;
        }
        sceMpegSub0060b708(8);
    }
}

// 0x0060bc58
static void sceMpegSub0060bc58(void) {
    int index;

    for (;;) {
        index = sceMpegSub0060b5d0(0x20);
        if (index == 0x1b5) {
            // The image forwards whatever argument value survives to this point, which varies by
            // caller. The reconstruction passes zero pending recovery of the intended command.
            sceMpegSub0060b708(0);
            index = sceMpegSub0060b820(4);
            if ((unsigned int)index > 10u) {
                index = 0;
            }
            g_mpegIndirectTable[index]();
            sceMpegSub0060b988();
            continue;
        }
        if (index != 0x1b2) {
            return;
        }
        sceMpegSub0060b708(0x20);
        sceMpegSub0060b988();
    }
}

// 0x0060bf70
static int sceMpegSub0060bf70(void) {
    if (g_mpeg2c7c != 3 && g_mpeg2c78 != g_mpeg3470) {
        if (g_mpeg346c != 0) {
            g_mpeg346c = 0;
            g_mpeg3430 += 0x400;
        }
        if (g_mpeg2c78 < g_mpeg3470 && g_mpeg3438 == 0) {
            g_mpeg346c = 1;
        }
        g_mpeg3438 = 0;
        g_mpeg3470 = g_mpeg2c78;
    }
    g_mpeg2d38 = g_mpeg3430 + g_mpeg2c78;
    if (g_mpeg346c != 0 && g_mpeg3470 >= g_mpeg2c78) {
        g_mpeg2d38 += 0x400;
    }
    if (g_mpeg3434 < g_mpeg2d38) {
        g_mpeg3434 = g_mpeg2d38;
    }
    return g_mpeg3434;
}

// 0x0060ba60
int sceMpegSub0060ba60(void) {
    StreamEntry entry;

    for (;;) {
        int status = sceMpegSub0060b988();
        if (status == 0x1b3) {
            sceMpegSub0060e020();
            continue;
        }
        if ((unsigned int)status >= 0x1b4u) {
            if (status == 0x1b7) {
                return g_mpeg2c7c;
            }
            if (status == 0x1b8) {
                sceMpegSub0060c050();
            }
            continue;
        }
        if (status != 0x100) {
            continue;
        }
        sceMpegSub0060bb88();
        entry.key = 5;
        entry.templateBits = ~(unsigned long long)0;
        entry.callback = (void *)~(uintptr_t)0;
        entry.data = (void *)~(uintptr_t)0;
        sceMpegInvokeCallbackSlot(g_decoderInstance, &entry);
        g_mpeg3388 = entry.templateBits;
        g_mpeg3390 = (unsigned long long)(unsigned int)(uintptr_t)entry.data << 32 |
            (unsigned int)(uintptr_t)entry.callback;
        return g_mpeg2c7c;
    }
}

// 0x0060e020
static int sceMpegSub0060e020(void) {
    sceMpeg *decoder;
    MpegWork *work;
    int bits;
    int half;

    decoder = (sceMpeg *)g_decoderInstance;
    work = (MpegWork *)decoder->pContext;
    // The clear falls in the setup call delay slot, so it lands before the setup body.
    work->mUnknownD4 = 0;
    bits = sceMpegSub0060b820(0x20);
    g_mpeg2c34 = bits & 0xf;
    g_mpeg2c30 = (bits >> 4) & 0xf;
    g_mpeg2c20 = (unsigned int)bits >> 0x14;
    if (((bits >> 8) & 0xfff) >= 0xaf1) {
        sceMpegRaiseError("vertical size > 2800");
    }
    g_mpeg2c24 = (bits >> 8) & 0xfff;
    bits = sceMpegSub0060b820(0x1e);
    g_mpeg2c40 = bits & 1;
    g_mpeg2c3c = (bits >> 1) & 0x3ff;
    g_mpeg2c38 = (unsigned int)bits >> 12;
    bits = sceMpegSub0060b820(1);
    g_mpeg33a0 = bits;
    if (bits == 0) {
        sceMpegSub0060e5a8(0x50000000u, g_abMpegDefaultIntraMatrix);
    } else {
        sceMpegSub0060b2c0();
        sceMpegSub0060b290(0x50000000u);
        sceMpegSub0060b2c0();
    }
    bits = sceMpegSub0060b820(1);
    g_mpeg33a4 = bits;
    if (bits == 0) {
        sceMpegSub0060e5a8(0x58000000u, g_abMpegDefaultNonIntraMatrix);
    } else {
        sceMpegSub0060b2c0();
        sceMpegSub0060b290(0x58000000u);
        sceMpegSub0060b2c0();
    }
    sceMpegSub0060bc58();
    decoder = (sceMpeg *)g_decoderInstance;
    work = (MpegWork *)decoder->pContext;
    if (g_mpegPollFlag == 0) {
        g_mpegCompareWord = 3;
        g_mpeg2cb4 = 1;
        g_mpeg2c6c = 5;
        g_mpeg2c48 = 1;
        g_mpeg2c4c = 1;
        g_mpeg2cc8 = 1;
    }
    g_mpeg2c28 = (g_mpeg2c20 + 0xf) >> 4;
    if (g_mpegPollFlag == 0 || g_mpeg2c48 != 0) {
        g_mpeg2c2c = (g_mpeg2c24 + 0xf) >> 4;
    } else {
        g_mpeg2c2c = ((g_mpeg2c24 + 0x1f) >> 5) << 1;
    }
    g_mpeg2c0c = g_mpeg2c28 << 4;
    g_mpeg2c10 = g_mpeg2c2c << 4;
    if (g_mpeg2c0c == work->mCompleted && g_mpeg2c10 == work->mClear) {
        return work->mClear;
    }
    work->mClear = g_mpeg2c10;
    work->mCompleted = g_mpeg2c0c;
    g_mpeg2c14 = g_mpeg2c0c >> 1;
    g_mpeg2c18 = g_mpeg2c10 >> 1;
    sceMpegRewindWritePointer(&work->mRing);
    work->mUnknownFC =
        (int)(uintptr_t)sceMpegCheckWorkAreaSize(&work->mRing, g_mpeg2c0c * 0x180 >> 8, 0x40);
    work->mUnknown100 =
        (int)(uintptr_t)sceMpegCheckWorkAreaSize(&work->mRing, g_mpeg2c0c * 0x180 >> 8, 0x40);
    work->mUnknown104 =
        (int)(uintptr_t)sceMpegCheckWorkAreaSize(&work->mRing, g_mpeg2c0c * 0x180 >> 8, 0x40);
    sceMpegSub0060e4c0(&g_mpegSeqAreas[0],
                        &g_mpegSeqAreas[1],
                        &g_mpegSeqAreas[2],
                        &g_mpegSeqAreas[3],
                        &g_mpegSeqAreas[4],
                        &g_mpegSeqAreas[5],
                        &g_mpegSeqAreas[6],
                        &g_mpegSeqAreas[7],
                        &g_mpegSeqAreas[8],
                        work->mUnknownFC,
                        work->mUnknown100,
                        work->mUnknown104);
    sceMpegSub0060e000(&g_mpegSeqAreas[0], g_mpeg2c0c, g_mpeg2c10);
    sceMpegSub0060e000(&g_mpegSeqAreas[1], g_mpeg2c0c, g_mpeg2c10);
    sceMpegSub0060e000(&g_mpegSeqAreas[2], g_mpeg2c0c, g_mpeg2c10);
    sceMpegSub0060e000(&g_mpegSeqAreas[3], g_mpeg2c0c, g_mpeg2c10);
    sceMpegSub0060e000(&g_mpegSeqAreas[4], g_mpeg2c0c, g_mpeg2c10);
    sceMpegSub0060e000(&g_mpegSeqAreas[5], g_mpeg2c0c, g_mpeg2c10);
    half = g_mpeg2c10 / 2;
    sceMpegSub0060e000(&g_mpegSeqAreas[6], g_mpeg2c0c, half);
    sceMpegSub0060e000(&g_mpegSeqAreas[7], g_mpeg2c0c, half);
    return sceMpegSub0060e000(&g_mpegSeqAreas[8], g_mpeg2c0c, half);
}

// Issues an IPU command word and returns the nibble table word its top nibble selects.
// 0x0060b290
static int sceMpegSub0060b290(unsigned int nValue) {
    unsigned int index;
    int value;

    *(volatile unsigned int *)(uintptr_t)kIpuCommandAddress = nValue;
    index = (nValue >> 28) & 0xfu;
    value = (int)g_mpegNibbleTable[index];
    g_mpegIpuBusyFlag = value;
    return value;
}

// 0x0060e5a8
static int sceMpegSub0060e5a8(unsigned int nCommand, const unsigned char *pMatrix) {
    StreamEntry entry;
    volatile unsigned int *pData;
    volatile unsigned int *pGifA;
    volatile unsigned int *pGifB;

    entry.key = 2;
    entry.templateBits = 0;
    entry.callback = NULL;
    entry.data = NULL;
    sceMpegInvokeCallbackSlot(g_decoderInstance, &entry);
    sceMpegSub0060b2c0();
    pData = (volatile unsigned int *)(uintptr_t)kIpuCommandAddress;
    *pData = 0u;
    pGifA = (volatile unsigned int *)(uintptr_t)0x1000b410;
    *pGifA = (unsigned int)(uintptr_t)pMatrix & 0xffffffu;
    pGifB = (volatile unsigned int *)(uintptr_t)0x1000b420;
    *pGifB = 4u;
    *pData = 0x101u;
    sceMpegSub0060b290(nCommand);
    sceMpegSub0060b2c0();
    entry.key = 3;
    return sceMpegInvokeCallbackSlot(g_decoderInstance, &entry);
}

// Sets picture dimensions into a sequence table and reports one.
// 0x0060e000
static int sceMpegSub0060e000(MpegSeqTable *pTable, int nA, int nB) {
    pTable->mUnknown0C = nA >> 4;
    pTable->mUnknown10 = nB >> 4;
    pTable->mUnknown04 = nA;
    pTable->mUnknown08 = nB;
    return 1;
}

// 0x0060e4c0
static void sceMpegSub0060e4c0(MpegSeqTable *pT0,
                               MpegSeqTable *pT1,
                               MpegSeqTable *pT2,
                               MpegSeqTable *pT3,
                               MpegSeqTable *pT4,
                               MpegSeqTable *pT5,
                               MpegSeqTable *pT6,
                               MpegSeqTable *pT7,
                               MpegSeqTable *pT8,
                               int nA,
                               int nB,
                               int nC) {
    int width = g_mpeg2c0c;
    int s1 = (nA & 0xffffff) | 0x20000000;
    int t7 = (nB & 0xffffff) | 0x20000000;
    int t4 = ((nA + width) & 0xffffff) | 0x20000000;
    int t5 = ((nB + width) & 0xffffff) | 0x20000000;
    int v1 = (nC & 0xffffff) | 0x20000000;
    // The image adds the table address to the third size with plain addition.
    int v0 = (int)(((uintptr_t)pT0 + (unsigned int)nC) & 0xffffffu) | 0x20000000;

    pT0->mUnknown00 = s1;
    pT1->mUnknown00 = t7;
    pT2->mUnknown00 = v1;
    pT3->mUnknown00 = s1;
    pT4->mUnknown00 = t7;
    pT5->mUnknown00 = v1;
    pT6->mUnknown00 = t4;
    pT7->mUnknown00 = t5;
    pT8->mUnknown00 = v0;
}

// 0x0060b708
static int sceMpegSub0060b708(int nCommand) {
    volatile unsigned int *pControl;
    volatile unsigned int *pData;
    unsigned int count;
    unsigned int index;
    int result;

    pControl = (volatile unsigned int *)(uintptr_t)kIpuControlAddress;
    pData = (volatile unsigned int *)(uintptr_t)kIpuCommandAddress;
    if ((*pControl & kIpuBusyMask) == kIpuBusyValue) {
        count = 0;
        for (;;) {
            if (count >= kIpuWatchdogLimit) {
                sceMpegSub005e0a08(g_decoderInstance);
                count = 0;
            }
            ++count;
            if ((*pControl & kIpuBusyMask) != kIpuBusyValue) {
                break;
            }
        }
    }
    *pData = (unsigned int)nCommand | 0x40000000u;
    index = (((unsigned int)nCommand | 0x40000000u) >> 28) & 0xfu;
    g_mpegIpuBusyFlag = (int)g_mpegNibbleTable[index];
    result = sceMpegSub0060b368();
    g_mpegShiftAccum = result;
    g_mpegShiftBudget = 0x20;
    return result;
}

// 0x0060b5d0
static int sceMpegSub0060b5d0(int nArg) {
    volatile unsigned int *pControl;
    volatile unsigned int *pData;
    unsigned int count;

    pControl = (volatile unsigned int *)(uintptr_t)kIpuControlAddress;
    pData = (volatile unsigned int *)(uintptr_t)kIpuCommandAddress;
    if (g_mpegIpuBusyFlag != 0 || g_mpegShiftBudget < nArg) {
        count = 0;
        for (;;) {
            if (count >= kIpuWatchdogLimit) {
                sceMpegSub005e0a08(g_decoderInstance);
                count = 0;
            }
            ++count;
            if ((*pControl & kIpuBusyMask) != kIpuBusyValue) {
                break;
            }
        }
        *pData = 0x40000000u;
        g_mpegIpuBusyFlag = (int)g_mpegNibbleTable[4];
        g_mpegShiftAccum = sceMpegSub0060b368();
        g_mpegShiftBudget = 0x20;
    }
    // The shift is a variable rotate the hardware masks to five bits.
    return (int)((unsigned int)g_mpegShiftAccum >> ((0 - nArg) & 31));
}

// 0x0060b988
static int sceMpegSub0060b988(void) {
    volatile unsigned int *pStatus;
    unsigned int arg;
    int result;

    pStatus = (volatile unsigned int *)(uintptr_t)0x10002020;
    arg = (0u - (*pStatus & 7u)) & 7u;
    if (arg != 0) {
        sceMpegSub0060b708((int)arg);
    }
    for (;;) {
        result = sceMpegSub0060b5d0(0x18);
        if (result == 1) {
            return result;
        }
        sceMpegSub0060b708(8);
    }
}

// 0x005e0a08
static int sceMpegSub005e0a08(void *pDecoder) {
    StreamEntry entry;

    // The image stores only the low key word and leaves the rest of the stack entry as garbage.
    // The reconstruction zeroes it instead, which no observed reader distinguishes.
    entry.key = 1;
    entry.templateBits = 0;
    entry.callback = NULL;
    entry.data = NULL;
    return sceMpegInvokeCallbackSlot(pDecoder, &entry);
}

// 0x0060b2c0
static int sceMpegSub0060b2c0(void) {
    volatile unsigned int *pControl;
    unsigned int count;

    pControl = (volatile unsigned int *)(uintptr_t)kIpuControlAddress;
    count = 0;
    if ((*pControl & kIpuBusyMask) != kIpuBusyValue) {
        return 0;
    }
    for (;;) {
        if (count >= kIpuWatchdogLimit) {
            sceMpegSub005e0a08(g_decoderInstance);
            count = 0;
        }
        ++count;
        if ((*pControl & kIpuBusyMask) == kIpuBusyValue) {
            return (int)count;
        }
    }
}

// 0x0060b368
static int sceMpegSub0060b368(void) {
    volatile unsigned long long *pData;
    volatile unsigned int *pControl;
    long long value;
    unsigned int count;

    pData = (volatile unsigned long long *)(uintptr_t)kIpuCommandAddress;
    pControl = (volatile unsigned int *)(uintptr_t)kIpuControlAddress;
    value = (long long)*pData;
    count = 0;
    for (;;) {
        if (value >= 0) {
            return (int)value;
        }
        if ((*pControl & kIpuDataReadyBit) != 0) {
            return (int)value;
        }
        if (count >= kIpuWatchdogLimit) {
            sceMpegSub005e0a08(g_decoderInstance);
            count = 0;
        }
        ++count;
        value = (long long)*pData;
    }
}

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

// 0x007798b8
// Key and match mask for each stream type. A packet belongs to a stream when its key, masked,
// equals the stream key. The mask also selects where the channel number goes.
static const unsigned long long g_streamTemplates[10][2] = {
    { 0xe000000000ULL, 0xff00000000ULL }, // MPEG-2 video, channel in the stream identifier.
    { 0xbdffc00000ULL, 0xffffffffffULL },
    { 0xbdffa00000ULL, 0xffffffffffULL }, // PCM audio.
    { 0xbdffa10000ULL, 0xffffffffffULL },
    { 0xbdff900000ULL, 0xffffffffffULL },
    { 0xc000000000ULL, 0xff00000000ULL },
    { 0xbd80000000ULL, 0xffff000000ULL },
    { 0xbda0000000ULL, 0xffff000000ULL },
    { 0xbd88000000ULL, 0xffff000000ULL },
    { 0xbd90000000ULL, 0xffff000000ULL },
};

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
        sceMpegRaiseError("work area size is too small");
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
        sceMpegRaiseError("work area size is too small");
        return NULL;
    }
    ring->mWrite = (int)needed;
    return (void *)(uintptr_t)aligned;
}

// 0x0060ded0
void sceMpegRaiseError(const char *pFormat) {
    sceMpeg *decoder;
    MpegWork *work;
    StreamEntry entry;

    decoder = (sceMpeg *)g_decoderInstance;
    if (decoder == NULL) {
        sceMpegPrintErrorLine(pFormat);
        return;
    }
    work = (MpegWork *)decoder->pContext;
    if (work == NULL || work->mSlots[0].callback == NULL) {
        sceMpegPrintErrorLine(pFormat);
        return;
    }
    // The low word carries zero and the high word carries the message, which the slot callback
    // reads back from the entry. The remaining words stay uninitialised, as they do in the image.
    entry.key = (unsigned long long)(unsigned int)(uintptr_t)pFormat << 32;
    sceMpegInvokeCallbackSlot(decoder, &entry);
}

// 0x0060de90
void sceMpegPrintErrorLine(const char *pMessage) {
    printf("[MPEG ERROR]%s\n", pMessage);
}

// 0x0060dea0
void sceMpegReportErrorFormatted(const char *pFormat, ...) {
    char buffer[0x110];
    va_list args;

    va_start(args, pFormat);
    vsprintf(buffer, pFormat, args);
    va_end(args);
    sceMpegRaiseError(buffer);
}

// 0x0060a038
int sceIpuSetControlBitTwentyThree(int nFlag) {
    volatile unsigned int *pControl;
    unsigned int value;

    pControl = (volatile unsigned int *)(uintptr_t)0x10002010;
    value = (*pControl & 0xff7fffffu) | ((unsigned int)nFlag << 23);
    // The store falls in the return delay slot, so it lands before the return either way.
    *pControl = value;
    return (int)value;
}

// 0x00637178
int sceIpuSync(int nMode) {
    volatile unsigned int *pControl;

    pControl = (volatile unsigned int *)(uintptr_t)0x10002010;
    if (nMode == 0) {
        while ((int)*pControl < 0) {
        }
        return 0;
    }
    if (nMode == 1) {
        return (int)(*pControl >> 31);
    }
    return 0;
}

// 0x0060dd78
void sceMpegDisableIpuControlBit(void) {
    int base;

    sceIpuSetControlBitTwentyThree(1);
    base = g_mpegIpuBase;
    g_mpegIpuTable.mUnknown00 = base;
    g_mpegIpuTable.mUnknown04 = base + 0x1800;
    g_mpegIpuTable.mUnknown140 = base + 0x1b00;
    g_mpegIpuTable.mUnknown144 = base + 0x3300;
    g_mpegIpuTable.mUnknown280 = 0;
}

// 0x0060ddc8
void sceMpegSub0060ddc8(void *pDecoder) {
    volatile unsigned int *pStatus;
    volatile unsigned int *pStatusSet;
    volatile unsigned int *pMaskA;
    volatile unsigned int *pMaskB;
    volatile unsigned int *pClearC;
    unsigned int value;

    (void)pDecoder;
    g_mpeg2d3c = 0;
    DIntr();
    // The enable store falls in the disable call delay slot, so it lands first.
    g_mpegIpuBusyFlag = 1;
    pStatus = (volatile unsigned int *)(uintptr_t)0x1000f520;
    pStatusSet = (volatile unsigned int *)(uintptr_t)0x1000f590;
    value = *pStatus;
    value = value | 0x00010000u;
    *pStatusSet = value;
    pMaskA = (volatile unsigned int *)(uintptr_t)0x1000b000;
    *pMaskA = 0;
    pMaskB = (volatile unsigned int *)(uintptr_t)0x1000b400;
    *pMaskB = 0;
    pClearC = (volatile unsigned int *)(uintptr_t)0x1000d400;
    *pClearC = 0;
    value = *pStatus;
    value = value & 0xfffeffffu;
    EIntr();
    // The clear store falls in the reenable call delay slot, so it lands first.
    *pStatusSet = value;
    pMaskA = (volatile unsigned int *)(uintptr_t)0x1000b020;
    *pMaskA = 0;
    pMaskB = (volatile unsigned int *)(uintptr_t)0x1000b420;
    *pMaskB = 0;
    pClearC = (volatile unsigned int *)(uintptr_t)0x1000d420;
    *pClearC = 0;
    *(volatile int *)(uintptr_t)0x10002010 = 0x40000000;
    sceIpuSync(0);
}

// 0x0060dce0
void sceIpuEnableControlBitTwentyThree(void) {
    g_nMpegIsMpeg2 = 0;
    sceIpuSetControlBitTwentyThree(1);
}

// 0x0061d9d8
int sceMpegSub0061d9d8(int nMode) {
    volatile unsigned int *pStatus;
    volatile unsigned int *pStatusSet;
    volatile unsigned int *pClear;
    unsigned int value;

    DIntr();
    pStatus = (volatile unsigned int *)(uintptr_t)0x1000f520;
    pStatusSet = (volatile unsigned int *)(uintptr_t)0x1000f590;
    value = *pStatus;
    value = value | 0x00010000u;
    *pStatusSet = value;
    pClear = (volatile unsigned int *)(uintptr_t)0x1000b400;
    *pClear = (unsigned int)nMode;
    value = *pStatus;
    value = value & 0xfffeffffu;
    *pStatusSet = value;
    return (int)EIntr();
}

// IPU command words copied by the initialiser, read from the image.
static const unsigned long long kIpuInitCommandsA[6] = {
    0x1616131013101008ULL, 0x1b1a181a16161616ULL, 0x1b1b1a1a1a1a1b1bULL,
    0x1d2222221d1d1d1bULL, 0x20201d1d1b1b1d1dULL, 0x2223232526252222ULL,
};
static const unsigned long long kIpuInitCommandsB[2] = {
    0x3e0084204210000ULL, 0x1ce718c614a51084ULL,
};

// 0x0061da40
int sceMpegSub0061da40(void) {
    volatile unsigned int *pReg;
    volatile unsigned long long *pFifo;
    int value;
    int i;
    static const int kCommandWords[6] = {0, 1, 2, 3, 4, 4};

    sceMpegSub0061d9d8(1);
    pReg = (volatile unsigned int *)(uintptr_t)0x10002010;
    *pReg = 0x40000000u;
    while ((int)*pReg < 0) {
    }
    pReg = (volatile unsigned int *)(uintptr_t)0x10002000;
    *pReg = 0u;
    while ((int)*(volatile unsigned int *)(uintptr_t)0x10002010 < 0) {
    }
    pFifo = (volatile unsigned long long *)(uintptr_t)0x10007010;
    for (i = 0; i < 6; ++i) {
        pFifo[0] = kIpuInitCommandsA[kCommandWords[i]];
    }
    *(volatile unsigned int *)(uintptr_t)0x10002000 = 0x60000000u;
    while ((int)*(volatile unsigned int *)(uintptr_t)0x10002010 < 0) {
    }
    pFifo[0] = kIpuInitCommandsB[0];
    pFifo[0] = kIpuInitCommandsB[1];
    *(volatile unsigned int *)(uintptr_t)0x10002000 = 0x58000000u;
    while ((int)*(volatile unsigned int *)(uintptr_t)0x10002010 < 0) {
    }
    *(volatile unsigned int *)(uintptr_t)0x10002000 = 0x90000000u;
    while ((int)*(volatile unsigned int *)(uintptr_t)0x10002010 < 0) {
    }
    *(volatile unsigned int *)(uintptr_t)0x10002010 = 0x40000000u;
    pReg = (volatile unsigned int *)(uintptr_t)0x10002010;
    while ((int)*pReg < 0) {
    }
    *(volatile unsigned int *)(uintptr_t)0x10002000 = 0u;
    pReg = (volatile unsigned int *)(uintptr_t)0x10002010;
    while ((int)*pReg < 0) {
    }
    value = (int)*pReg;
    return value;
}

// 0x005caa58
int sceMpegDemuxPss(sceMpeg *pMpeg, unsigned char *pStart, int nSize) {
    return sceMpegDemuxPssRing(pMpeg, pStart, nSize, NULL, -1);
}

// 0x005ca4e0
static unsigned long long buildStreamKey(int nType, int nChannel) {
    static const unsigned long long kSubstreamByteMask = 0xffffULL << 24;
    static const unsigned long long kStreamIdMask = 0xff00ULL << 24;
    unsigned long long mask;
    int shift;

    if ((unsigned int)nType >= 10u) {
        return 0;
    }
    mask = g_streamTemplates[nType][1];
    if (mask == kSubstreamByteMask) {
        shift = 24;
    } else if (mask > kSubstreamByteMask) {
        shift = 0;
    } else if (mask == kStreamIdMask) {
        shift = 32;
    } else {
        shift = 0;
    }
    return g_streamTemplates[nType][0] | ((unsigned long long)(long long)nChannel << shift);
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
            g_mpegTables[index]->mUnknown28 = 0;
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

    DIntr();
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
    EIntr();
    pClearB = (volatile unsigned int *)(uintptr_t)0x1000b420;
    *pClearB = 0;
    return sceMpegSub0061da40();
}

// Bit reader over the input, 0x30 bytes. The cache has the next bits left-aligned, and each
// advance refills it past 56 valid bits. A read of up to 32 bits therefore never needs a refill. The fetch pointer wraps
// from the ring end back to its base.
typedef struct {
    unsigned long long mCache;
    unsigned char *mStart;
    unsigned char *mFetch;
    int mCached;
    unsigned long long mPosition; // Bits consumed since mStart.
    unsigned char *mRingBase;
    uintptr_t mRingEnd; // All ones without a ring, which the fetch pointer never arrives at.
    int mRingSize;
} BitReader;

// The pack header fields the demultiplexer stores, 0x10 bytes.
typedef struct {
    int mScrExtension;
    unsigned int mScrLow; // System clock reference bits 31 to 0.
    int mScrHigh; // System clock reference bit 32.
    int mHasSystemHeader;
} PackHeader;

// One parsed packet, 0x2c bytes. The key is the stream identifier shifted up by 32 bits, with the
// substream word of a private stream in the low word. Positions are bit positions from the start
// of the input.
typedef struct {
    unsigned long long mKey;
    int mPacketLength;
    int mScrambling;
    long long mPts;
    long long mDts;
    int mDataPosition;
    int mDataLength;
    int mHeaderPosition;
} PesPacket;

enum {
    kCacheRefillBits = 57,
    kCacheTopShift = 56,
    kPacketStartPrefix = 0x000001,
    kPackStartCode = 0x000001ba,
    kSystemHeaderStartCode = 0x000001bb,
    kProgramEndCode = 0x000001b9,
    kStreamIdProgramStreamMap = 0xbc,
    kStreamIdPrivate1 = 0xbd,
    kStreamIdPadding = 0xbe,
    kStreamIdPrivate2 = 0xbf,
    kStreamIdEcm = 0xf0,
    kStreamIdEmm = 0xf1,
    kStreamIdDsmcc = 0xf2,
    kStreamIdH2221TypeE = 0xf8,
    kStreamIdDirectory = 0xff,
    kPtsFlag = 2,
    kPtsDtsFlags = 3,
    kSubstreamHeaderBytes = 4,
    // The packet length counts the three bytes of flags and header length ahead of the header data.
    kPesFlagBytes = 3,
    kPrivateDataBits = 128
};

// Stream key the demultiplexer falls back to when no registered stream matches a packet.
static const unsigned long long kDefaultStreamKey = 0xbdffULL << 24;

// 0x00779958
// Bits the optional PES fields occupy for each combination of the ES rate, trick mode, copy
// information, and CRC flags.
static const unsigned char kOptionalFieldBits[] = {
    0, 16, 8, 24, 8, 24, 16, 32, 24, 40, 32, 48, 32, 48, 40, 56 };

static unsigned long long streamKey(int nStreamId) {
    return (unsigned long long)(unsigned int)nStreamId << 32;
}

// 0x006102c0
static void bitReaderAdvance(BitReader *pReader, int nBits) {
    pReader->mCache <<= nBits;
    pReader->mCached -= nBits;
    // An overdrawn count wraps to a large unsigned value and skips the refill, as in the image.
    while ((unsigned int)pReader->mCached < kCacheRefillBits) {
        pReader->mCache |= (unsigned long long)*pReader->mFetch
                           << (kCacheTopShift - pReader->mCached);
        ++pReader->mFetch;
        if ((uintptr_t)pReader->mFetch >= pReader->mRingEnd) {
            pReader->mFetch = pReader->mRingBase;
        }
        pReader->mCached += 8;
    }
    pReader->mPosition += (unsigned long long)(long long)nBits;
}

// 0x00610268
static void initBitReader(BitReader *pReader,
                          unsigned char *pStart,
                          unsigned char *pRingBase,
                          int nRingSize) {
    pReader->mFetch = pStart;
    pReader->mRingEnd = (uintptr_t)pRingBase + (uintptr_t)(intptr_t)nRingSize;
    pReader->mRingSize = nRingSize;
    pReader->mStart = pStart;
    pReader->mCache = 0;
    pReader->mCached = 0;
    pReader->mPosition = 0;
    pReader->mRingBase = pRingBase;
    bitReaderAdvance(pReader, 0);
}

// 0x006102a0
static int peekBits(const BitReader *pReader, int nBits) {
    return (int)(pReader->mCache >> (64 - nBits));
}

// 0x00610358
static int getBits(BitReader *pReader, int nBits) {
    int value = peekBits(pReader, nBits);
    bitReaderAdvance(pReader, nBits);
    return value;
}

// 0x006103a8
static int getBit(BitReader *pReader) {
    int value = peekBits(pReader, 1);
    bitReaderAdvance(pReader, 1);
    return value;
}

// 0x006103f0
// Moves the reader nBytes ahead of its bit position, rounded down to a byte, and refills.
static void skipBytes(BitReader *pReader, int nBytes) {
    unsigned long long position;
    unsigned char *pFetch;

    position = pReader->mPosition + (unsigned long long)(long long)(nBytes * 8);
    pReader->mCache = 0;
    pReader->mCached = 0;
    pFetch = pReader->mStart + (int)(position >> 3);
    if ((uintptr_t)pFetch >= pReader->mRingEnd) {
        pFetch -= pReader->mRingSize;
    }
    pReader->mFetch = pFetch;
    pReader->mPosition = position;
    bitReaderAdvance(pReader, 0);
}

// 0x00610448
static unsigned char *pointerAt(const BitReader *pReader, int nBitPosition) {
    unsigned char *pByte = pReader->mStart + (nBitPosition >> 3);
    if ((uintptr_t)pByte >= pReader->mRingEnd) {
        pByte -= pReader->mRingSize;
    }
    return pByte;
}

// A 33-bit time stamp split 3, 15, and 15 around marker bits, after its four-bit prefix.
static inline long long readTimestamp(BitReader *pReader) {
    int high;
    int middle;
    int low;
    unsigned int lowWord;

    (void)getBits(pReader, 4);
    high = getBits(pReader, 3);
    (void)getBit(pReader);
    middle = getBits(pReader, 15);
    (void)getBit(pReader);
    low = getBits(pReader, 15);
    (void)getBit(pReader);
    lowWord = ((unsigned int)high << 30) | ((unsigned int)middle << 15) | (unsigned int)low;
    return (long long)(((unsigned long long)((high >> 2) & 1) << 32) | lowWord);
}

// 0x005cacc0
static int parseSystemHeader(BitReader *pReader, PackHeader *pPack) {
    (void)pPack; // The image passes the pack header and never reads it.
    (void)getBits(pReader, 56); // Start code, header length, and the first rate bits.
    (void)getBits(pReader, 40); // The rest of the fixed fields.
    while (peekBits(pReader, 1) == 1) {
        (void)getBits(pReader, 24); // One stream bound entry.
    }
    return 1;
}

// 0x005cab70
static int parsePackHeader(BitReader *pReader, PackHeader *pPack) {
    int high;
    int middle;
    int low;
    int stuffing;
    int i;

    (void)getBits(pReader, 34); // Start code and the '01' marker.
    high = getBits(pReader, 3);
    (void)getBit(pReader);
    middle = getBits(pReader, 15);
    (void)getBit(pReader);
    low = getBits(pReader, 15);
    (void)getBit(pReader);
    pPack->mScrExtension = getBits(pReader, 9);
    (void)getBits(pReader, 30); // Marker, mux rate, markers, and reserved bits.
    stuffing = getBits(pReader, 3);
    pPack->mScrLow = ((unsigned int)high << 30) | ((unsigned int)middle << 15) | (unsigned int)low;
    pPack->mScrHigh = (int)(((unsigned int)high >> 2) & 1);
    for (i = 0; i < stuffing; ++i) {
        (void)getBits(pReader, 8);
    }
    if (peekBits(pReader, 32) == kSystemHeaderStartCode) {
        pPack->mHasSystemHeader = 1;
        parseSystemHeader(pReader, pPack);
    } else {
        pPack->mHasSystemHeader = 0;
    }
    return 1;
}

// 0x005cad30
// Returns zero when the packet embeds a pack header. A program stream may not embed one.
static int parsePacket(BitReader *pReader, PesPacket *pPacket) {
    int streamId;
    int ptsDtsFlags;
    int hasEscr;
    int optionalFlags;
    int hasExtension;
    int headerDataLength;
    int headerStart;
    int payload;
    int skip;
    int i;

    pPacket->mHeaderPosition = (int)pReader->mPosition;
    (void)getBits(pReader, 24);
    streamId = getBits(pReader, 8);
    pPacket->mKey = streamKey(streamId);
    pPacket->mPacketLength = getBits(pReader, 16);
    pPacket->mPts = -1;
    pPacket->mDts = -1;
    switch (streamId) {
    case kStreamIdPrivate2:
        pPacket->mKey |= (unsigned int)getBits(pReader, 32);
        skip = pPacket->mPacketLength - kSubstreamHeaderBytes;
        if (skip != 0) {
            skipBytes(pReader, skip);
        }
        return 1;
    case kStreamIdProgramStreamMap:
    case kStreamIdPadding:
    case kStreamIdEcm:
    case kStreamIdEmm:
    case kStreamIdDirectory:
    case kStreamIdDsmcc:
    case kStreamIdH2221TypeE:
        if (pPacket->mPacketLength != 0) {
            skipBytes(pReader, pPacket->mPacketLength);
        }
        return 1;
    default:
        break;
    }

    (void)getBits(pReader, 2); // The '10' marker.
    pPacket->mScrambling = getBits(pReader, 2);
    (void)getBits(pReader, 4); // Priority, alignment, copyright, and original flags.
    ptsDtsFlags = getBits(pReader, 2);
    hasEscr = getBits(pReader, 1);
    optionalFlags = getBits(pReader, 4);
    hasExtension = getBits(pReader, 1);
    headerDataLength = getBits(pReader, 8);
    headerStart = (int)pReader->mPosition;
    if ((ptsDtsFlags & kPtsFlag) != 0) {
        pPacket->mPts = readTimestamp(pReader);
    }
    if (ptsDtsFlags == kPtsDtsFlags) {
        pPacket->mDts = readTimestamp(pReader);
    }
    if (hasEscr == 1) {
        (void)getBits(pReader, 48);
    }
    if (optionalFlags != 0) {
        (void)getBits(pReader, kOptionalFieldBits[optionalFlags]);
    }
    if (hasExtension == 1) {
        int hasPrivateData = getBits(pReader, 1);
        int hasPackHeader = getBits(pReader, 1);
        int hasSequenceCounter = getBits(pReader, 1);
        int hasPStdBuffer = getBits(pReader, 1);
        int hasExtension2;

        (void)getBits(pReader, 3);
        hasExtension2 = getBits(pReader, 1);
        if (hasPrivateData == 1) {
            // 128 bits, read as 48, 48, and 32.
            (void)getBits(pReader, 48);
            (void)getBits(pReader, 48);
            (void)getBits(pReader, 32);
        }
        if (hasPackHeader == 1) {
            sceMpegRaiseError("pack_header_field_flag needs to be '0' in PS\n");
            return 0;
        }
        if (hasSequenceCounter == 1) {
            (void)getBits(pReader, 16);
        }
        if (hasPStdBuffer == 1) {
            (void)getBits(pReader, 16);
        }
        if (hasExtension2 == 1) {
            int length;

            (void)getBit(pReader);
            length = getBits(pReader, 7);
            for (i = 0; i < length; ++i) {
                (void)getBits(pReader, 8);
            }
        }
    }
    // Skip the rest of the header data, including any stuffing.
    skip = headerDataLength -
           (int)((pReader->mPosition - (unsigned long long)(long long)headerStart) >> 3);
    if (skip != 0) {
        skipBytes(pReader, skip);
    }
    payload = pPacket->mPacketLength - headerDataLength;
    pPacket->mDataLength = payload - kPesFlagBytes;
    pPacket->mDataPosition = (int)pReader->mPosition;
    skip = payload - kPesFlagBytes;
    if (pPacket->mKey == streamKey(kStreamIdPrivate1)) {
        // The substream word stays in the reported data and length.
        pPacket->mKey |= (unsigned int)getBits(pReader, 32);
        skip = payload - kPesFlagBytes - kSubstreamHeaderBytes;
    }
    if (skip != 0) {
        skipBytes(pReader, skip);
    }
    return 1;
}

static int deliverPacket(sceMpeg *pMpeg,
                         const BitReader *pReader,
                         const PesPacket *pPacket,
                         sceMpegCallback pfnCallback,
                         void *pData) {
    sceMpegCbDataStr callbackData;

    callbackData.type = sceMpegCbStr;
    callbackData.header = pointerAt(pReader, pPacket->mHeaderPosition);
    callbackData.data = pointerAt(pReader, pPacket->mDataPosition);
    callbackData.len = (unsigned int)pPacket->mDataLength;
    callbackData.pts = pPacket->mPts;
    callbackData.dts = pPacket->mDts;
    return pfnCallback(pMpeg, &callbackData, pData);
}

static int isPacketStart(const BitReader *pReader) {
    return peekBits(pReader, 24) == kPacketStartPrefix &&
           peekBits(pReader, 32) != kPackStartCode && peekBits(pReader, 32) != kProgramEndCode;
}

// 0x005ca768
int sceMpegDemuxPssRing(sceMpeg *pMpeg,
                        unsigned char *pStart,
                        int nSize,
                        unsigned char *pBuffer,
                        int nBufferSize) {
    MpegWork *work;
    StreamEntry *table;
    BitReader reader;
    PackHeader pack;
    // Packets without header data do not set the payload fields, so a callback sees the previous
    // packet's values. The image starts them from uninitialised stack memory.
    PesPacket packet = { 0 };
    sceMpegCallback defaultCallback;
    void *defaultData;
    unsigned long long limit;
    int consumed;
    int proceed;
    int count;
    int i;

    work = (MpegWork *)pMpeg->pContext;
    table = work->mStreamTable;
    defaultCallback = NULL;
    defaultData = NULL;
    consumed = 0;
    proceed = 1;
    limit = (unsigned long long)(long long)(nSize * 8);
    initBitReader(&reader, pStart, pBuffer, nBufferSize);
    count = work->mStreamCount;
    for (i = 0; i < count; ++i) {
        if (table[i].key == kDefaultStreamKey) {
            defaultData = table[i].data;
            defaultCallback = table[i].callback;
        }
        if (defaultCallback != NULL) {
            break;
        }
    }

    if (peekBits(&reader, 32) == kPackStartCode) {
        parsePackHeader(&reader, &pack);
    }
    for (;;) {
        if (isPacketStart(&reader) && reader.mPosition < limit) {
            if (proceed == 0) {
                return consumed;
            }
            parsePacket(&reader, &packet);
            if (limit < reader.mPosition) {
                continue;
            }
            count = work->mStreamCount;
            for (i = 0; i < count; ++i) {
                if (table[i].key == (packet.mKey & table[i].templateBits)) {
                    proceed =
                        deliverPacket(pMpeg, &reader, &packet, table[i].callback, table[i].data);
                    break;
                }
            }
            // The count is read again after a callback. The callback may register a stream.
            if (i == work->mStreamCount && defaultCallback != NULL) {
                proceed = deliverPacket(pMpeg, &reader, &packet, defaultCallback, defaultData);
            }
            if (proceed != 0) {
                consumed = (int)(reader.mPosition >> 3);
            }
            continue;
        }
        if (limit < reader.mPosition || peekBits(&reader, 32) != kPackStartCode) {
            return consumed;
        }
        parsePackHeader(&reader, &pack);
    }
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
        sceMpegReportErrorFormatted("image buffer needs to be aligned to 64byte bound");
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
        sceMpegRaiseError("The size of work area is too small");
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
    sceMpegDisableIpuControlBit();
    sceMpegSub005e08e8(decoder);
    sceMpegSub005e0928(decoder);
    for (i = 0; i < kTableCount; ++i) {
        g_mpegTables[i] = &g_mpegSeqAreas[i];
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
sceMpegCallback sceMpegAddStrCallback(
    void *pDecoder, int nType, int nChannel, sceMpegCallback pfnCallback, void *pData) {
    MpegWork *work;
    StreamEntry *table;
    int count;
    unsigned long long key;
    unsigned long long templateBits;
    StreamEntry *entry;
    int index;
    sceMpegCallback found;

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
