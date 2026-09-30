#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <eekernel.h>
#include <libgraph.h>

// Plain C reconstruction of the Sony libgraph entry points used by the game,
// translated from the disassembly. Register words are packed exactly as the
// machine code packs them, while kernel services go through ps2sdk.

// The VIF1 channel control register. Bit 8 reports a running transfer.
#define VIF1_CHCR (*(volatile unsigned int *)(uintptr_t)0x10009000U)
// The VIF1 channel address register.
#define VIF1_MADR (*(volatile unsigned int *)(uintptr_t)0x10009010U)
// The VIF1 channel quadword count register.
#define VIF1_QWC (*(volatile unsigned int *)(uintptr_t)0x10009020U)
// The VIF1 channel tag address register.
#define VIF1_TADR (*(volatile unsigned int *)(uintptr_t)0x10009030U)
// The VIF1 status register.
#define VIF1_STAT (*(volatile unsigned int *)(uintptr_t)0x10003C00U)
// The VIF1 force break register.
#define VIF1_FBRST (*(volatile unsigned int *)(uintptr_t)0x10003C10U)
// The VIF1 error mask register.
#define VIF1_ERR (*(volatile unsigned int *)(uintptr_t)0x10003C20U)
// The VIF1 data FIFO, read and written a whole quadword at a time.
#define VIF1_FIFO (*(vu128 *)(uintptr_t)0x10005000U)
// The GIF control register.
#define GIF_CTRL (*(volatile unsigned int *)(uintptr_t)0x10003000U)
// The GIF status register. Bits 10 and 11 report path activity.
#define GIF_STAT (*(volatile unsigned int *)(uintptr_t)0x10003020U)
// The GIF channel control register. Bit 8 reports a running transfer.
#define GIF_CHCR (*(volatile unsigned int *)(uintptr_t)0x1000A000U)
// The GIF channel address register.
#define GIF_MADR (*(volatile unsigned int *)(uintptr_t)0x1000A010U)
// The GIF channel quadword count register.
#define GIF_QWC (*(volatile unsigned int *)(uintptr_t)0x1000A020U)
// The GIF channel tag address register.
#define GIF_TADR (*(volatile unsigned int *)(uintptr_t)0x1000A030U)
// The interrupt controller status register. Writing a set bit acknowledges the matching cause.
#define INTC_STAT (*(volatile unsigned int *)(uintptr_t)0x1000F000U)
// The graphics synthesiser status register.
#define GS_CSR (*(volatile unsigned long long *)(uintptr_t)0x12001000U)
// The graphics synthesiser bus direction register.
#define GS_BUSDIR (*(volatile unsigned long long *)(uintptr_t)0x12001040U)
// The privileged display registers written by the display output helper.
#define GS_PMODE (*(volatile unsigned long long *)(uintptr_t)0x12000000U)
#define GS_SMODE2 (*(volatile unsigned long long *)(uintptr_t)0x12000020U)
#define GS_DISPFB1 (*(volatile unsigned long long *)(uintptr_t)0x12000070U)
#define GS_DISPLAY1 (*(volatile unsigned long long *)(uintptr_t)0x12000080U)
#define GS_DISPFB2 (*(volatile unsigned long long *)(uintptr_t)0x12000090U)
#define GS_DISPLAY2 (*(volatile unsigned long long *)(uintptr_t)0x120000A0U)
#define GS_EXTDATA (*(volatile unsigned long long *)(uintptr_t)0x120000C0U)
#define GS_BGCOLOR (*(volatile unsigned long long *)(uintptr_t)0x120000E0U)

// The shared graphics state block. The interlace, output, field, and GS revision words occupy the
// first eight bytes, followed by the vertical blank handler and its identifier.
typedef struct {
    short interlaceMode;
    short outputMode;
    short fieldMode;
    short signalBits;
    int (*vblankHandler)(int);
    int handlerId;
} GsState;

// The eight VIF1 codes the reset path primes the FIFO with, as two quadwords (STCYCL, STMASK and
// its operand, STMOD, MSKPATH3, OFFSET, BASE, and ITOP).
typedef union {
    unsigned int mWords[8];
    u128 mQuads[2];
} Vif1InitPacket;

// 0x007848d0
static const Vif1InitPacket g_dwVif1InitPacket = {{0x01000404U,
                                                   0x20000000U,
                                                   0U,
                                                   0x05000000U,
                                                   0x06000000U,
                                                   0x03000000U,
                                                   0x02000000U,
                                                   0x04000000U}};

// The MSKPATH3 code restored to the VIF1 FIFO after image store work, padded with three NOP codes.
// 0x007729c0
static const u128 g_vif1StorePacket = 0x06000000U;

// 0x00784900
static GsState g_GsStateBlock = {1, 2, 1, 3, NULL, 0};

// Mode words of the state block, and the vertical blank start bit of the interrupt controller.
enum {
    kGsFieldMode = 0,
    kGsInterlace = 1,
    kGsFrameMode = 1,
    kGsNtsc = 2,
    kGsPal = 3,
    kIntcVblankStartBit = 4
};

// The spin budget every libgraph busy wait shares.
enum {
    kChannelSpinLimit = 0x1000000
};

// 0x006004c8
// Returns the shared graphics state block.
static GsState *sceGsGetGParam(void) {
    return &g_GsStateBlock;
}

// 0x005963e0
// Acknowledges the vertical blank start interrupt, spins until it is raised again, and
// acknowledges it once more.
static void WaitVsyncStart(void) {
    INTC_STAT = kIntcVblankStartBit;
    while ((INTC_STAT & kIntcVblankStartBit) == 0U) {
    }
    INTC_STAT = kIntcVblankStartBit;
}

// 0x00596420
// Waits for the next vertical blank while a handler is installed and returns the GS status word
// the kernel captured at the blank. Bit 13 of the word records the field the blank began.
static unsigned long long WaitVsyncFlag(void) {
    unsigned int flag = 0U;
    unsigned long long status;

    SetVSyncFlag(&flag, &status);
    INTC_STAT = kIntcVblankStartBit;
    while ((INTC_STAT & kIntcVblankStartBit) == 0U) {
        __asm__ volatile("" ::: "memory");
        if (flag != 0U) {
            break;
        }
    }
    INTC_STAT = kIntcVblankStartBit;
    return status; // The binary returns the word unread when the interrupt bit ends the wait.
}

// 0x0062f318
// Returns the frame buffer page count of a width and height pair. The count is also the base page
// of a depth buffer placed after the frame buffer. Sixteen-bit formats count rows in 64-pixel
// units, and the other formats in 32-pixel units. Only interlaced field mode retains a single
// count; every other mode doubles it. The result is truncated to 16 bits.
static int FramePageCount(short nPsm, short nWidth, short nHeight) {
    GsState *state = sceGsGetGParam();
    int columns = (nWidth + 63) / 64;
    int rows;
    int blocks;

    if ((nPsm & 2) != 0) {
        rows = (nHeight + 63) / 64;
    } else {
        rows = (nHeight + 31) / 32;
    }
    blocks = columns * rows;
    if (state->interlaceMode == kGsInterlace && state->fieldMode == kGsFieldMode) {
        return (short)blocks;
    }
    return (short)(blocks << 1);
}

// 0x00636360
// Writes one display environment to the privileged registers. Field mode
// one selects the first video circuit, and any other mode selects the
// second circuit together with the output mode register.
static void WriteDisplayEnv(const sceGsDispEnv *pDisp) {
    GsState *state = sceGsGetGParam();

    if (state->fieldMode == 1) {
        GS_PMODE = pDisp->pmode;
        GS_DISPFB1 = pDisp->dispfb;
        GS_DISPLAY1 = pDisp->display;
        GS_EXTDATA = pDisp->bgcolor;
    } else {
        GS_PMODE = pDisp->pmode;
        GS_SMODE2 = pDisp->smode2;
        GS_DISPFB2 = pDisp->dispfb;
        GS_DISPLAY2 = pDisp->display;
        GS_BGCOLOR = pDisp->bgcolor;
    }
}

// 0x006002d0
// Resets the VIF1, VU1, and GIF path, then primes VIF1 through its FIFO.
void sceGsResetPath(void) {
    unsigned int clip = 0U;

    VIF1_FBRST = 1U;
    VIF1_ERR = 2U;
    __sync_synchronize();
    __asm__ volatile("cfc2 %0, $12" : "=r"(clip));
    clip |= 0x200U;
    __asm__ volatile("ctc2 %0, $12" ::"r"(clip));
    __sync_synchronize();
    VIF1_FIFO = g_dwVif1InitPacket.mQuads[0];
    VIF1_FIFO = g_dwVif1InitPacket.mQuads[1];
    GIF_CTRL = 1U;
}

// 0x00600338
// Resets the graphics state. Mode one clears the vertical blank flag, mode
// five replays the video setup while retaining the blank handler, mode zero
// additionally removes that handler, and other modes return quietly.
void sceGsResetGraph(short nMode, short nInterlace, short nOutputMode, short nFieldMode) {
    GsState *state;

    if (nMode == 1) {
        GS_CSR = 0x100ULL;
        return;
    }
    if (nMode < 2) {
        unsigned long long status;

        if (nMode != 0) {
            return;
        }
        state = sceGsGetGParam();
        GS_CSR = 0x200ULL;
        state->interlaceMode = nInterlace;
        state->outputMode = nOutputMode;
        status = GS_CSR;
        state->signalBits = (short)((status >> 16) & 0xFFULL);
        GsPutIMR(0xFF00ULL);
        state->fieldMode = (short)(nFieldMode > 0);
        if (state->vblankHandler != NULL) {
            DisableIntc(INTC_VBLANK_S);
            RemoveIntcHandler(INTC_VBLANK_S, state->handlerId);
            state->handlerId = 0;
            state->vblankHandler = NULL;
        }
    } else {
        if (nMode != 5) {
            return;
        }
        state = sceGsGetGParam();
        state->fieldMode = (short)(nFieldMode > 0);
        state->interlaceMode = nInterlace;
        state->outputMode = nOutputMode;
        state->signalBits = (short)((GS_CSR >> 16) & 0xFFULL);
    }
    SetGsCrt((short)(nInterlace & 1), (short)(nOutputMode & 0xFF), (short)(nFieldMode & 1));
}

// 0x00596708
// Waits for the next vertical blank and returns the field it began in.
// Progressive modes report field one.
int sceGsSyncV(int nMode) {
    GsState *state = sceGsGetGParam();
    unsigned long long status;
    int field;

    (void)nMode;
    if (state->vblankHandler == NULL) {
        WaitVsyncStart();
        if (state->interlaceMode != 1) {
            return 1;
        }
        return (int)((GS_CSR >> 13) & 1ULL);
    }
    status = WaitVsyncFlag() >> 13;
    field = (int)(status & 1ULL);
    if (state->interlaceMode != 1) {
        return 1;
    }
    return field;
}

// 0x00621c20
// Fills the four alpha environment pairs and returns the pair count.
int sceGsSetDefAlphaEnv(sceGsAlphaEnv *pAlpha, short nPabe) {
    pAlpha->mWords[1] = 0x42ULL;
    pAlpha->mWords[0] = 0x44ULL;
    pAlpha->mWords[3] = 0x49ULL;
    pAlpha->mWords[2] = (unsigned long long)(long long)nPabe;
    pAlpha->mWords[5] = 0x3BULL;
    pAlpha->mWords[4] = (0x81ULL << 32) | 0x807FULL;
    pAlpha->mWords[7] = 0x4AULL;
    pAlpha->mWords[6] = 0ULL;
    __sync_synchronize();
    return 4;
}

// 0x006217c8
// Fills the five display registers. The output mode selects the timing
// branch, and unknown modes only report an error.
void sceGsSetDefDispEnv(
    sceGsDispEnv *pDisp, short nPsm, short nWidth, short nHeight, short nDx, short nDy) {
    GsState *state = sceGsGetGParam();
    long long width = nWidth;
    long long height = nHeight;
    int interlace = state->interlaceMode;
    int output = state->outputMode;
    int field = state->fieldMode;
    unsigned long long mode;
    unsigned long long buffer;
    unsigned long long shown;

    pDisp->pmode = 0x66ULL;
    if (interlace == 0) {
        pDisp->smode2 = 2ULL;
    } else if (field != 0) {
        pDisp->smode2 = 3ULL;
    } else {
        pDisp->smode2 = 1ULL;
    }
    buffer = ((unsigned long long)(nPsm & 0xF) << 15) |
        ((((unsigned long long)(width + 0x3F) >> 6) & 0x3FULL) << 9);
    pDisp->dispfb = buffer;
    if (output == 2) {
        long long divisor = width + 0x9FF;
        long long factor = divisor / width;
        long long product = factor * width;
        long long scan = factor * nDx;
        unsigned long long vertical;
        unsigned long long across;
        unsigned long long down;
        unsigned long long lines;

        if (nDx != 1) {
            vertical = ((unsigned long long)(nDy + 0x32) & 0xFFFULL) << 12;
            across = ((unsigned long long)(scan + 0x27C) & 0xFFFULL);
        } else {
            vertical = ((unsigned long long)(nDy + 0x19) & 0xFFFULL) << 12;
            across = ((unsigned long long)(scan + 0x27C) & 0xFFFULL);
        }
        mode = ((unsigned long long)(factor - 1) << 23);
        down = (unsigned long long)(product - 1);
        if (field == 0) {
            lines = (unsigned long long)(height - 1);
        } else {
            lines = (unsigned long long)(height * 2 - 1);
        }
        shown = vertical | mode | across | (down & 0xFFFFFFFFULL) | (lines << 44);
        pDisp->display = shown;
    } else if (output == 3) {
        long long divisor = width + 0x9FF;
        long long factor = divisor / width;
        long long product = factor * width;
        long long scan = factor * nDx;
        unsigned long long vertical;
        unsigned long long across;
        unsigned long long down;
        unsigned long long lines;

        if (nDx != 1) {
            vertical = ((unsigned long long)(nDy + 0x48) & 0xFFFULL) << 12;
            across = ((unsigned long long)(scan + 0x290) & 0xFFFULL);
        } else {
            vertical = ((unsigned long long)(nDy + 0x24) & 0xFFFULL) << 12;
            across = ((unsigned long long)(scan + 0x290) & 0xFFFULL);
        }
        mode = ((unsigned long long)(factor - 1) << 23);
        down = (unsigned long long)(product - 1);
        if (field == 0) {
            lines = (unsigned long long)(height - 1);
        } else {
            lines = (unsigned long long)(height * 2 - 1);
        }
        shown = vertical | mode | across | (down & 0xFFFFFFFFULL) | (lines << 44);
        pDisp->display = shown;
    } else {
        printf("sceGsDefDispEnv:Not support displaymode for %d!!\n", output);
    }
    pDisp->bgcolor = 0ULL;
}

// 0x00621a38
// Fills the eight draw environment pairs and returns the pair count. The
// depth buffer follows the frame buffer, dithering follows the colour
// depth, and the test word follows the depth test.
int sceGsSetDefDrawEnv(
    sceGsDrawEnv1 *pDraw, short nPsm, short nWidth, short nHeight, short nZTest, short nZPsm) {
    unsigned long long frame;
    unsigned long long depth;
    unsigned long long offset;
    unsigned long long clip;
    unsigned long long test;

    frame = (((unsigned long long)(nPsm & 0xF)) << 24) |
        ((((unsigned long long)(nWidth + 0x3F) >> 6) & 0x3FULL) << 16);
    pDraw->frame1 = frame;
    pDraw->frame1addr = 0x4CULL;
    pDraw->zbuf1addr = 0x4EULL;
    depth = (unsigned long long)FramePageCount(nPsm, nWidth, nHeight);
    depth |= ((unsigned long long)(nZPsm & 0xF)) << 24;
    if (nZTest == 0) {
        depth |= 0x8000ULL << 17;
    }
    pDraw->zbuf1 = depth;
    offset = (((unsigned long long)(0x800 - (nWidth >> 1))) << 4) |
        (((unsigned long long)(0x800 - (nHeight >> 1))) << 36);
    pDraw->xyoffset1 = offset;
    pDraw->xyoffset1addr = 0x18ULL;
    clip = (((unsigned long long)(nWidth - 1)) << 16) |
        (((unsigned long long)(nHeight - 1)) << 48);
    pDraw->scissor1 = clip;
    pDraw->scissor1addr = 0x40ULL;
    pDraw->prmodecontaddr = 0x1AULL;
    pDraw->prmodecont |= 1ULL;
    pDraw->colclampaddr = 0x46ULL;
    pDraw->colclamp |= 1ULL;
    if ((nPsm & 2) != 0) {
        pDraw->dtheaddr = 0x45ULL;
        pDraw->dthe |= 1ULL;
    } else {
        pDraw->dthe &= ~1ULL;
    }
    if (nZTest == 0) {
        test = 0x30000ULL;
    } else {
        test = (((unsigned long long)(nZTest & 3)) << 17) | 0x10000ULL;
    }
    pDraw->test1 = test;
    pDraw->test1addr = 0x47ULL;
    __sync_synchronize();
    return 8;
}

// 0x00622508
// Fills the six clear packet pairs and returns the pair count. The packet
// disables testing, draws one sprite in the clear colour, then restores the
// requested test.
int sceGsSetDefClear(sceGsClear *pClear,
                     short nZTest,
                     short nX,
                     short nY,
                     short nWidth,
                     short nHeight,
                     unsigned long long nRed,
                     unsigned long long nGreen,
                     unsigned long long nBlue,
                     unsigned long long nAlpha,
                     unsigned int nZ) {
    unsigned long long first;
    unsigned long long second;
    unsigned long long colour;
    unsigned long long test;

    first = ((unsigned long long)(nX << 4)) | (((unsigned long long)(nY << 4)) << 16);
    second = ((unsigned long long)((nX + nWidth) << 4)) |
        ((unsigned long long)((nY + nHeight) << 4) << 16);
    first |= ((unsigned long long)nZ) << 32;
    second |= ((unsigned long long)nZ) << 32;
    colour = (nRed & 0xFFULL) | ((nGreen & 0xFFULL) << 8) | ((nBlue & 0xFFULL) << 16);
    colour |= (nAlpha & 0xFFULL) << 24;
    colour |= 0xFE00ULL << 46;
    pClear->prim = 6ULL;
    pClear->rgbaqaddr = 1ULL;
    pClear->rgbaqWord = colour;
    pClear->xyz2a = first;
    pClear->xyz2aaddr = 5ULL;
    pClear->xyz2b = second;
    pClear->xyz2baddr = 5ULL;
    pClear->testaaddr = 0x47ULL;
    pClear->testa = 0x30000ULL;
    pClear->primaddr = 0ULL;
    pClear->testbaddr = 0x47ULL;
    if (nZTest == 0) {
        test = 0x30000ULL;
    } else {
        test = (((unsigned long long)(nZTest & 3)) << 17) | 0x10000ULL;
    }
    pClear->testb = test;
    __sync_synchronize();
    return 6;
}

// 0x005e4b30
// Fills the display, draw, and clear halves plus the two display tags. The
// clear halves stay empty unless requested, and interlaced modes patch the
// first half frame addresses for the odd field.
int sceGsSetDefDBuff(sceGsDBuff *pDBuff,
                     short nPsm,
                     short nWidth,
                     short nHeight,
                     short nZTest,
                     short nZPsm,
                     short nClear) {
    GsState *state = sceGsGetGParam();
    unsigned long long loops = (nClear != 0) ? 0xEULL : 8ULL;
    int clearX = 0x800 - (nWidth >> 1);
    int clearY = 0x800 - (nHeight >> 1);
    int pages;

    sceGsSetDefDispEnv(&pDBuff->disp[0], nPsm, nWidth, nHeight, 0, 0);
    sceGsSetDefDispEnv(&pDBuff->disp[1], nPsm, nWidth, nHeight, 0, 0);
    sceGsSetDefDrawEnv(&pDBuff->draw0, nPsm, nWidth, nHeight, nZTest, nZPsm);
    sceGsSetDefDrawEnv(&pDBuff->draw1, nPsm, nWidth, nHeight, nZTest, nZPsm);
    if (nClear != 0) {
        sceGsSetDefClear(&pDBuff->clear0, nZTest, (short)clearX, (short)clearY, nWidth,
            nHeight, 0ULL, 0ULL, 0ULL, 0ULL, 0U);
        sceGsSetDefClear(&pDBuff->clear1, nZTest, (short)clearX, (short)clearY, nWidth,
            nHeight, 0ULL, 0ULL, 0ULL, 0ULL, 0U);
    }
    pDBuff->giftag0.mWords[0] = loops | 0x8000ULL | 0x1000000000000000ULL;
    pDBuff->giftag0.mWords[1] = 0xEULL;
    pDBuff->giftag1.mWords[0] = loops | 0x8000ULL | 0x1000000000000000ULL;
    pDBuff->giftag1.mWords[1] = 0xEULL;
    pages = FramePageCount(nPsm, nWidth, nHeight);
    // Interlaced frame mode and progressive output place the second buffer after the first.
    if (!(state->interlaceMode == kGsInterlace && state->fieldMode == kGsFrameMode) &&
        state->interlaceMode != 0) {
        return 0;
    }
    pages >>= 1;
    pDBuff->disp[1].dispfb =
        (pDBuff->disp[1].dispfb & ~0x1FFULL) | (unsigned long long)(pages & 0x1FF);
    pDBuff->draw0.frame1 =
        (pDBuff->draw0.frame1 & ~0x1FFULL) | (unsigned long long)(pages & 0x1FF);
    return 0;
}

// 0x00612600
// Sends one draw environment packet through the GIF channel. A busy channel
// waits briefly, and an expiry reports an error.
int sceGsPutDrawEnv(sceGifTag *pGifTag) {
    unsigned int spins = 0U;
    unsigned long long first;
    unsigned int address;
    unsigned int count;

    if ((GIF_CHCR & 0x100U) != 0U) {
        spins = 0U;
        while ((GIF_CHCR & 0x100U) != 0U) {
            if (kChannelSpinLimit < spins) {
                printf("sceGsPutDrawEnv: DMA Ch.2 does not terminate\r\n");
                return -1;
            }
            spins++;
        }
    }
    first = pGifTag->mWords[0];
    address = (unsigned int)(uintptr_t)pGifTag;
    count = (unsigned int)(first & 0x7FFFULL) + 1U;
    GIF_QWC = count;
    if ((address & 0x70000000U) == 0x70000000U) {
        GIF_MADR = (address & 0x0FFFFFFFU) | 0x80000000U;
    } else {
        GIF_MADR = address & 0x0FFFFFFFU;
    }
    GIF_CHCR = 0x101U;
    return 0;
}

// 0x005e4948
// Fills the twelve word load image descriptor and returns the register
// count. Oversized transfers only report an error.
int sceGsSetDefLoadImage(sceGsLoadImage *pLoadImage,
                         short nTbp,
                         short nTbw,
                         short nPsm,
                         short nSsx,
                         short nSsy,
                         short nRrw,
                         short nRrh) {
    int count = 0;
    unsigned long long buffer;
    unsigned long long position;
    unsigned long long size;

    // The format table routes each storage format to its quadword count.
    if (nPsm < 0x3B) {
        switch (nPsm) {
        case 0x00:
        case 0x2D:
            count = (nRrw * nRrh) >> 2;
            break;
        case 0x01:
        case 0x30:
            count = ((nRrw * nRrh) << 1) + (nRrw * nRrh);
            count >>= 4;
            break;
        case 0x02:
        case 0x0A:
        case 0x31:
        case 0x39:
            count = (nRrw * nRrh) >> 3;
            break;
        case 0x13:
        case 0x1B:
            count = (nRrw * nRrh) >> 4;
            break;
        case 0x14:
        case 0x24:
        case 0x2C:
            count = (nRrw * nRrh) >> 5;
            break;
        default:
            break;
        }
    }
    if (0x7FFF < count) {
        printf("sceGsSetDefLoadImage: too big size\r\n");
        return 0;
    }
    // The hardware clears both tag slots before the masked words go in.
    pLoadImage->mWords[10] = 0ULL;
    pLoadImage->mWords[11] = 0ULL;
    pLoadImage->mWords[0] = 0ULL;
    pLoadImage->mWords[1] = 0ULL;
    pLoadImage->mWords[10] = (((unsigned long long)(count & 0x7FFF)) | 0x8000ULL) &
        0xF3FFFFFFFFFFFFFFULL;
    pLoadImage->mWords[10] |= 0x0800000000000000ULL;
    buffer = (((unsigned long long)nTbp) << 32) | (((unsigned long long)nTbw) << 48);
    buffer |= ((unsigned long long)nPsm) << 56;
    pLoadImage->mWords[2] = buffer;
    position = (((unsigned long long)nSsx) << 32) | (((unsigned long long)nSsy) << 48);
    pLoadImage->mWords[4] = position;
    size = ((unsigned long long)nRrw) | (((unsigned long long)nRrh) << 32);
    pLoadImage->mWords[6] = size;
    pLoadImage->mWords[0] = 0x1000000000008004ULL;
    pLoadImage->mWords[1] = 0xEULL;
    pLoadImage->mWords[3] = 0x50ULL;
    pLoadImage->mWords[5] = 0x51ULL;
    pLoadImage->mWords[7] = 0x52ULL;
    pLoadImage->mWords[9] = 0x53ULL;
    pLoadImage->mWords[8] = 0ULL;
    __sync_synchronize();
    return 6;
}

// 0x005e2a08
// Sends one load image descriptor, then streams the source pixels behind
// it. Either busy wait shares a single spin budget.
int sceGsExecLoadImage(sceGsLoadImage *pLoadImage, const void *pSource) {
    unsigned int spins = 0U;
    unsigned int address;
    unsigned int count;

    if ((GIF_CHCR & 0x100U) != 0U) {
        while ((GIF_CHCR & 0x100U) != 0U) {
            if (kChannelSpinLimit < spins) {
                printf("sceGsExecLoadImage: DMA Ch.2 does not terminate\r\n");
                return -1;
            }
            spins++;
        }
    }
    GIF_QWC = 6U;
    address = (unsigned int)(uintptr_t)pLoadImage;
    if ((address & 0x70000000U) == 0x70000000U) {
        GIF_MADR = (address & 0x0FFFFFFFU) | 0x80000000U;
    } else {
        GIF_MADR = address & 0x0FFFFFFFU;
    }
    GIF_CHCR = 0x101U;
    while ((GIF_CHCR & 0x100U) != 0U) {
        if (kChannelSpinLimit < spins) {
            printf("sceGsExecLoadImage: DMA Ch.2 does not terminate\r\n");
            return -1;
        }
        spins++;
    }
    count = (unsigned int)(pLoadImage->mWords[10] & 0x7FFFULL) + 1U;
    GIF_QWC = count;
    address = (unsigned int)(uintptr_t)pSource;
    if ((address & 0x70000000U) == 0x70000000U) {
        GIF_MADR = (address & 0x0FFFFFFFU) | 0x80000000U;
    } else {
        GIF_MADR = address & 0x0FFFFFFFU;
    }
    GIF_CHCR = 0x101U;
    return 0;
}

// 0x005e4808
// Fills the store image packet and returns the register count. The packet
// spans 0x70 bytes of tag, register data, and register identifiers.
int sceGsSetDefStoreImage(sceGsStoreImage *pStoreImage,
                          short nSbp,
                          short nSbw,
                          short nPsm,
                          short nSsx,
                          short nSsy,
                          short nRrw,
                          short nRrh) {
    volatile unsigned int *halfWords = (volatile unsigned int *)pStoreImage;
    volatile unsigned long long *words = (volatile unsigned long long *)pStoreImage;
    unsigned long long buffer;
    unsigned long long position;
    unsigned long long size;

    // The hardware clears the first register pair before merging the masked
    // words in.
    words[2] = 0ULL;
    words[3] = 0ULL;
    words[2] = 0x8005ULL;
    words[3] = 0xEULL;
    buffer = ((unsigned long long)(long long)nSbp) | (((unsigned long long)nSbw) << 16);
    buffer |= ((unsigned long long)(long long)nPsm) << 24;
    words[4] = buffer;
    position = ((unsigned long long)(long long)nSsx) | (((unsigned long long)nSsy) << 16);
    words[6] = position;
    size = ((unsigned long long)(long long)nRrw) | (((unsigned long long)(long long)nRrh) << 32);
    words[8] = size;
    halfWords[1] = 0x06008000U;
    halfWords[2] = 0x13000000U;
    halfWords[3] = 0x50000006U;
    words[5] = 0x50ULL;
    words[7] = 0x51ULL;
    words[9] = 0x52ULL;
    words[11] = 0x61ULL;
    words[12] = 1ULL;
    words[13] = 0x53ULL;
    halfWords[0] = 0U;
    words[10] = 0ULL;
    __sync_synchronize();
    return 7;
}

// 0x005a3550
// Streams one store image descriptor, then receives the pixels at the
// destination. Ragged widths round the height up and drain the remainder
// through the stack slot.
int sceGsExecStoreImage(sceGsStoreImage *pStoreImage, void *pDest) {
    volatile unsigned long long *words = (volatile unsigned long long *)pStoreImage;
    unsigned long long packed = words[4];
    unsigned long long regs = words[8];
    int width = (int)(regs & 0xFFFULL);
    int height = (int)(regs >> 32);
    int format = (int)((packed >> 24) & 0x3FULL);
    int extra = 0;
    int ragged = 0;
    int aligned = 0;
    int small = 0;
    int spins = 0;
    int rounded = height;
    unsigned long long saved;

    if (format < 0x3B) {
        int full = 0;

        switch (format) {
        case 0x00:
        case 0x2D:
            full = (width * height) << 2;
            aligned = (full >> 4) & ~7;
            ragged = full & 0xF;
            small = (full >> 4) & 7;
            if (ragged != 0) {
                rounded = (height + 3) & 0x1FFC;
                extra = ((width * rounded) >> 2) - aligned - small - 1;
            }
            break;
        case 0x01:
        case 0x30:
            full = width * height;
            full += (full << 1);
            aligned = (full >> 4) & ~7;
            ragged = full & 0xF;
            small = (full >> 4) & 7;
            if (ragged != 0) {
                int product;

                rounded = (height + 0xF) & 0x1FF0;
                product = width * rounded;
                extra = ((product + (product << 1)) >> 4) - aligned - small - 1;
            }
            break;
        case 0x02:
        case 0x0A:
        case 0x31:
        case 0x39:
            full = (width * height) << 1;
            aligned = (full >> 4) & ~7;
            ragged = full & 0xF;
            small = (full >> 4) & 7;
            if (ragged != 0) {
                rounded = (height + 7) & ~7;
                extra = ((width * rounded) >> 3) - aligned - small - 1;
            }
            break;
        case 0x13:
        case 0x1B:
            full = width * height;
            aligned = (full >> 4) & ~7;
            ragged = full & 0xF;
            small = (full >> 4) & 7;
            if (ragged != 0) {
                rounded = (height + 7) & ~7;
                extra = ((width * rounded) >> 4) - aligned - small - 1;
            }
            break;
        default:
            full = width * height;
            aligned = (full >> 5) & ~7;
            ragged = (full >> 1) & 0xF;
            small = (full >> 5) & 7;
            if (ragged != 0) {
                rounded = (height + 7) & ~7;
                extra = ((width * rounded) >> 5) - aligned - small - 1;
            }
            break;
        }
    } else {
        rounded = 0;
    }
    if (ragged != 0) {
        unsigned long long size = ((unsigned long long)width) |
            (((unsigned long long)rounded) << 32);
        volatile unsigned long long *uncached;

        uncached = (volatile unsigned long long *)(uintptr_t)(
            ((uintptr_t)pStoreImage + 0x40U) | 0x20000000U);
        *uncached = size;
    }
    if ((VIF1_CHCR & 0x100U) != 0U) {
        while ((VIF1_CHCR & 0x100U) != 0U) {
            if ((unsigned int)kChannelSpinLimit < (unsigned int)spins) {
                printf("sceGsExecStoreImage: DMA Ch.1 does not terminate\r\n");
                return -1;
            }
            spins++;
        }
    }
    saved = GsGetIMR();
    GsPutIMR(saved | 0x200ULL);
    GS_CSR = 2ULL;
    VIF1_QWC = 7U;
    if ((((unsigned int)(uintptr_t)pStoreImage) & 0x70000000U) == 0x70000000U) {
        VIF1_MADR = (((unsigned int)(uintptr_t)pStoreImage) & 0x0FFFFFFFU) | 0x80000000U;
    } else {
        VIF1_MADR = ((unsigned int)(uintptr_t)pStoreImage) & 0x0FFFFFFFU;
    }
    VIF1_CHCR = 0x101U;
    while ((VIF1_CHCR & 0x100U) != 0U) {
        if ((unsigned int)kChannelSpinLimit < (unsigned int)spins) {
            printf("sceGsExecStoreImage: DMA Ch.1 does not terminate\r\n");
            return -1;
        }
        spins++;
    }
    if ((GS_CSR & 2ULL) == 0ULL) {
        while ((GS_CSR & 2ULL) == 0ULL) {
            if ((unsigned int)kChannelSpinLimit < (unsigned int)spins) {
                printf("sceGsExecStoreImage: GS does not terminate\r\n");
                VIF1_FIFO = g_vif1StorePacket;
                return -1;
            }
            spins++;
        }
    }
    VIF1_STAT = 0x80000000U;
    GS_BUSDIR = 1ULL;
    if (aligned != 0) {
        VIF1_QWC = (unsigned int)aligned;
        if ((((unsigned int)(uintptr_t)pDest) & 0x70000000U) == 0x70000000U) {
            VIF1_MADR = (((unsigned int)(uintptr_t)pDest) & 0x0FFFFFFFU) | 0x80000000U;
        } else {
            VIF1_MADR = ((unsigned int)(uintptr_t)pDest) & 0x0FFFFFFFU;
        }
        VIF1_CHCR = 0x100U;
        while ((VIF1_CHCR & 0x100U) != 0U) {
            if ((unsigned int)kChannelSpinLimit < (unsigned int)spins) {
                printf("sceGsExecStoreImage: DMA Ch.1 (GS->MEM) does not terminate\r\n");
                GS_CSR = 0x100ULL;
                GS_BUSDIR = 0ULL;
                GIF_CTRL = 1U;
                VIF1_FBRST = 1U;
                return -1;
            }
            spins++;
        }
    }
    if (0 < extra) {
        int drained = 0;

        while (drained < extra) {
            u128 quad;

            while ((VIF1_STAT & 0x1F000000U) == 0U) {
                if ((unsigned int)kChannelSpinLimit < (unsigned int)spins) {
                    printf("sceGsExecStoreImage: Enough data does not reach VIF1\n");
                    GS_CSR = 0x100ULL;
                    GS_BUSDIR = 0ULL;
                    GIF_CTRL = 1U;
                    VIF1_FBRST = 1U;
                    return -1;
                }
                spins++;
            }
            quad = VIF1_FIFO;
            (void)quad; // The binary drains the FIFO and discards each quadword.
            drained++;
        }
    }
    VIF1_STAT = 0U;
    GsPutIMR(saved);
    GS_BUSDIR = 0ULL;
    GS_CSR = 2ULL;
    VIF1_FIFO = g_vif1StorePacket;
    return 0;
}

// 0x00552270
// Synchronises the graphics path. Mode zero waits for VIF1, GIF, and VU1 with one spin budget
// shared across every wait, and dumps the channel registers when the budget runs out. Other modes
// report the busy units as a mask. The timeout argument is accepted but unused.
int sceGsSyncPath(int nMode, unsigned short nTimeout) {
    unsigned int spins = 0U;
    unsigned int vuStatus;
    const char *stage;
    int mask;

    (void)nTimeout;
    if (nMode != 0) {
        mask = (VIF1_CHCR & 0x100U) != 0U ? 1 : 0;
        if ((GIF_CHCR & 0x100U) != 0U) {
            mask |= 2;
        }
        if ((VIF1_STAT & 0x1F000003U) != 0U) {
            mask |= 4;
        }
        __asm__ volatile("cfc2 %0, $13" : "=r"(vuStatus));
        if ((vuStatus & 0x100U) != 0U) {
            mask |= 8;
        }
        if ((GIF_STAT & 0xC00U) != 0U) {
            mask |= 0x10;
        }
        return mask;
    }

    while ((VIF1_CHCR & 0x100U) != 0U) {
        if (spins++ > kChannelSpinLimit) {
            stage = "sceGsSyncPath: DMA Ch.1 does not terminate\r\n";
            goto timeout;
        }
    }
    while ((GIF_CHCR & 0x100U) != 0U) {
        if (spins++ > kChannelSpinLimit) {
            stage = "sceGsSyncPath: DMA Ch.2 does not terminate\r\n";
            goto timeout;
        }
    }
    while ((VIF1_STAT & 0x1F000003U) != 0U) {
        if (spins++ > kChannelSpinLimit) {
            stage = "sceGsSyncPath: VIF1 does not terminate\r\n";
            goto timeout;
        }
    }
    for (;;) {
        __asm__ volatile("cfc2 %0, $13" : "=r"(vuStatus));
        if ((vuStatus & 0x100U) == 0U) {
            break;
        }
        if (spins++ > kChannelSpinLimit) {
            stage = "sceGsSyncPath: VU1 does not terminate\r\n";
            goto timeout;
        }
    }
    while ((GIF_STAT & 0xC00U) != 0U) {
        if (spins++ > kChannelSpinLimit) {
            stage = "sceGsSyncPath: GIF does not terminate\r\n";
            goto timeout;
        }
    }
    return 0;

timeout:
    printf(stage);
    printf("\t<D1_CHCR=%08x:", VIF1_CHCR);
    printf("D1_TADR=%08x:", VIF1_TADR);
    printf("D1_MADR=%08x:", VIF1_MADR);
    printf("D1_QWC=%08x>\r\n", VIF1_QWC);
    printf("\t<D2_CHCR=%08x:", GIF_CHCR);
    printf("D2_TADR=%08x:", GIF_TADR);
    printf("D2_MADR=%08x:", GIF_MADR);
    printf("D2_QWC=%08x>\r\n", GIF_QWC);
    printf("\t<VIF1_STAT=%08x:", VIF1_STAT);
    printf("GIF_STAT=%08x>\r\n", GIF_STAT);
    return -1;
}

// 0x0062dca0
// Recentres the half pixel offset on the scissor extent, adding a half dot
// of vertical offset for the odd field.
void sceGsSetHalfOffset(void *pDrawEnv, int nOffsetX, int nOffsetY, int nField) {
    volatile unsigned long long *words = (volatile unsigned long long *)pDrawEnv;
    unsigned long long scissor = words[6];
    long long shownX = (long long)((scissor >> 16) & 0x7FFULL);
    long long shownY = (long long)((scissor >> 48) & 0x7FFULL);
    long long deltaX = (long long)(short)nOffsetX - ((shownX + 1) >> 1);
    long long deltaY = (long long)(short)nOffsetY - ((shownY + 1) >> 1);
    unsigned long long across = (unsigned long long)deltaX << 4;
    unsigned long long down = (unsigned long long)deltaY << 4;

    if ((short)nField == 0) {
        words[4] = across | (down << 32);
    } else {
        words[4] = across | ((down + 8ULL) << 32);
    }
}

// 0x0062d998
// Presents one double buffer half. The display environment of the selected
// field goes to the privileged registers, and the matching display tag is
// sent through the GIF channel.
void sceGsSwapDBuff(sceGsDBuff *pDBuff, int nField) {
    int field = nField & 1;
    unsigned char *base = (unsigned char *)pDBuff;
    const sceGsDispEnv *shown;

    shown = (const sceGsDispEnv *)(base + (unsigned int)(field * 0x28));
    WriteDisplayEnv(shown);
    if (field == 0) {
        sceGsPutDrawEnv(&pDBuff->giftag0);
    } else {
        sceGsPutDrawEnv(&pDBuff->giftag1);
    }
}

// 0x005e8558
// Installs the vertical blank handler, replacing the previous one, or
// removes it when null. Returns the previous handler.
int (*sceGsSyncVCallback(int (*pfnHandler)(int)))(int) {
    GsState *state = sceGsGetGParam();
    int (*oldHandler)(int) = state->vblankHandler;

    if (pfnHandler == NULL) {
        DisableIntc(INTC_VBLANK_S);
        RemoveIntcHandler(INTC_VBLANK_S, state->handlerId);
        state->vblankHandler = NULL;
        state->handlerId = 0;
    } else {
        if (oldHandler != NULL) {
            DisableIntc(INTC_VBLANK_S);
            RemoveIntcHandler(INTC_VBLANK_S, state->handlerId);
        }
        state->vblankHandler = pfnHandler;
        state->handlerId = AddIntcHandler(INTC_VBLANK_S, pfnHandler, -1);
        EnableIntc(INTC_VBLANK_S);
    }
    return oldHandler;
}
