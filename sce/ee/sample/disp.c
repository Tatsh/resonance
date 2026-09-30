#include "ezmpeg/disp.h"

#include <ee_regs.h>
#include <eekernel.h>
#include <ezmpeg.h>
#include <libdma.h>
#include <libgifpk.h>
#include <libgraph.h>
#include <malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "ezmpeg/videodec.h"
#include "ezmpeg/vobuf.h"

// The decoder instance, which the playback driver owns. The stream callbacks address it directly.
extern VideoDec videoDec;

// The audio decoder instance, which the playback driver owns. The audio callback addresses it
// directly.
extern AudioDec audioDec;

// The decoded frame queue, which the playback driver owns. The display handlers take entries
// from it and release them through it.
extern VoBuf voBuf;

// The display double buffer, which the playback driver owns. The endimage handler swaps its
// halves as fields are shown.
extern sceGsDBuff db;

enum {
    kPacketAlign = 0x10,
    kPacketBufferSize = 0x60,
    kPhysicalAddressMask = 0x0fffffff,
    kUncachedSegment = 0x20000000,
    kHalfOffset = 0x800,
    kImageBlockSize = 0x400,
    kImageQuadwords = 0x40,
    kReserveWords = 4,
    kFirstFieldOffset = 0x40,
    kSecondFieldOffset = 0x20240,
    kTagDecoded = 2,
    kTagFirstFieldShown = 1,
    kTagEmpty = 0,
};

// One 16-byte packet header, copied verbatim into display packets.
typedef struct {
    unsigned long long mLow;
    unsigned long long mHigh;
} PacketHeader;

// The two packet headers the image holds at 0x00835e78. The first heads register data and the
// second heads image transfer data. Their words are copied unchanged into every display packet,
// and the zeroes below stand in for the image words, which the disassembly does not reveal.
static const PacketHeader g_packetHeaders[2] = { { 0ULL, 0ULL }, { 0ULL, 0ULL } };

// 0x0077b128
// Set while the display runs. The endimage handler leaves quietly outside that window.
static int g_isDisplaying;

// 0x0077b12c
// Counts the endimage interrupts handled since the display started.
static int g_endimageCount;

// 0x0077b130
// Holds the shown tag state until the vertical blank handler releases the entry.
static int g_frameShown;

// 0x0077b134
// The field the last endimage interrupt observed.
static int g_currentField;

// 0x0077b138
// The outcome of the last path synchronisation in the endimage handler.
static int g_syncResult;

// 0x0070ce8c
// Counts the endimage interrupts that found no filled tag entry.
static int g_emptyCount;

static const unsigned long long kImageTagHeader = 0x0800000000000040ULL;
static const unsigned long long kTextureBase = 0xaa8031800ULL;
static const unsigned long long kClearCorner = 0x96002d00ULL;
static const unsigned long long kSourceBufferWord = 0xc180ULL << 36;
static const unsigned long long kTransferSizeWord = (0x10ULL << 32) | 0x10ULL;

// A cached address becomes an uncached address by keeping the low 28 bits and setting bit 29.
static unsigned char *ToUncachedAddress(const void *pAddress) {
    const uintptr_t nAddress = (uintptr_t)pAddress;
    return (unsigned char *)((nAddress & kPhysicalAddressMask) | kUncachedSegment);
}

// 0x005d2860
void clearGsMem(int nRed, int nGreen, int nBlue, int nWidth, int nHeight) {
    void *pBuffer = memalign(kPacketAlign, kPacketBufferSize);
    sceDmaChan *pChannel = sceDmaGetChan(SCE_DMA_GIF);

    // Zero the tag first, then read its words back. The loads observe zero, so the stored tag
    // below is constant.
    db.giftag0.mWords[0] = 0ULL;
    db.giftag0.mWords[1] = 0ULL;
    unsigned long long tagWord0 = db.giftag0.mWords[0];
    unsigned long long tagWord1 = db.giftag0.mWords[1];
    tagWord0 =
        (((tagWord0 & 0xffffffffffff8000ULL) | 0x8008ULL) & 0x0fffffffffffffffULL) |
        0x1000000000000000ULL;
    tagWord1 = (tagWord1 & 0xfffffffffffffff0ULL) | 0xeULL;
    db.giftag0.mWords[0] = tagWord0;
    db.giftag0.mWords[1] = tagWord1;

    // Halve the height, then round both heights up to 32-pixel units and combine them into the
    // draw height.
    const int halfHeight = nHeight / 2;
    const int heightRound = nHeight + 31;
    const int heightStep = (heightRound >= 0 ? heightRound : nHeight + 62) >> 5;
    const int halfRound = halfHeight + 31;
    const int halfStep = (halfRound >= 0 ? halfRound : halfHeight + 62) >> 5;
    const int drawHeight = (halfStep + heightStep) << 6;
    const short drawWidth = (short)nWidth;

    sceGsSetDefDrawEnv(&db.draw0, 0, drawWidth, drawHeight, 0, 0);
    db.draw0.xyoffset1 = 0ULL;
    FlushCache(0);
    sceGsSyncPath(0, 0);
    sceGsPutDrawEnv(&db.giftag0);

    // The clear packet draws one sprite in the clear colour across the buffer.
    sceGifPkData packet;
    sceGifPkInit(&packet, pBuffer);
    sceGifPkReset(&packet);
    sceGifPkEnd(&packet, 0, 0, 0);
    sceGifPkOpenGsAD(&packet, &g_packetHeaders[0]);
    const unsigned long long clearColour =
        (unsigned long long)(nRed | (nGreen << 8) | (nBlue << 16));
    sceGifPkAddGsAD(&packet, 0, 6);
    sceGifPkAddGsAD(&packet, 1, clearColour);
    sceGifPkAddGsAD(&packet, 5, 0);
    sceGifPkAddGsAD(&packet, 5, kClearCorner);
    sceGifPkCloseGifTag(&packet);
    sceGifPkTerminate(&packet);
    FlushCache(0);
    sceGsSyncPath(0, 0);
    sceDmaSend(pChannel, packet.mBase);
    sceGsSyncPath(0, 0);
    free(pBuffer);
}

// 0x005d2ab8
void setImageTag(void *pTag, void *pImage, int nField, int nWidth, int nHeight) {
    sceGifPkData packet;
    unsigned char *image = (unsigned char *)pImage;
    void *packetBase = ToUncachedAddress(pTag);

    sceGifPkInit(&packet, packetBase);
    sceGifPkReset(&packet);
    if (nField == 0) {
        // The image uploads in 16-pixel blocks, each with its own transfer registers and
        // image data.
        sceGifPkCnt(&packet, 0, 0, 0);
        sceGifPkOpenGsAD(&packet, &g_packetHeaders[1]);
        sceGifPkAddGsAD(&packet, 0x50, kSourceBufferWord);
        sceGifPkAddGsAD(&packet, 0x52, kTransferSizeWord);
        sceGifPkCloseGifTag(&packet);

        const int columns = nWidth >> 4;
        const int rows = nHeight >> 4;
        for (int column = 0; column < columns; ++column) {
            if (rows <= 0) {
                continue;
            }
            for (int row = 0; row < rows; ++row) {
                sceGifPkCnt(&packet, 0, 0, 0);
                sceGifPkOpenGsAD(&packet, &g_packetHeaders[1]);
                const unsigned long long position =
                    ((unsigned long long)(column * 16) << 32) |
                    ((unsigned long long)(row * 16) << 48);
                sceGifPkAddGsAD(&packet, 0x51, position);
                // The transfer always runs from the host to local memory.
                sceGifPkAddGsAD(&packet, 0x53, 0);
                sceGifPkCloseGifTag(&packet);
                unsigned long long *imageTag = sceGifPkReserve(&packet, kReserveWords);
                imageTag[0] = kImageTagHeader;
                imageTag[1] = 0ULL;
                const uintptr_t imageAddress = (uintptr_t)image;
                sceGifPkRef(&packet,
                            (void *)(imageAddress & kPhysicalAddressMask),
                            kImageQuadwords,
                            0,
                            0,
                            0);
                image += kImageBlockSize;
            }
        }
    }

    // The display setup draws the uploaded image as a textured sprite.
    sceGifPkEnd(&packet, 0, 0, 0);
    sceGifPkOpenGsAD(&packet, &g_packetHeaders[0]);
    sceGifPkAddGsAD(&packet, 0x3f, 0);
    sceGifPkAddGsAD(&packet, 0x14, 0x60);
    sceGifPkAddGsAD(&packet, 6, kTextureBase);
    sceGifPkAddGsAD(&packet, 0, 0x116);
    const unsigned long long firstColour = 8ULL | (8ULL << 16);
    const unsigned long long firstCorner = 0x6c00ULL | (0x7880ULL << 16);
    const unsigned long long secondColour =
        (8ULL + (unsigned long long)(nWidth * 16)) |
        ((8ULL + (unsigned long long)(nHeight * 16)) << 16);
    const unsigned long long secondCorner =
        (0x6c00ULL + 0x2800ULL) | ((0x7880ULL + 0xf00ULL) << 16);
    sceGifPkAddGsAD(&packet, 3, firstColour);
    sceGifPkAddGsAD(&packet, 5, firstCorner);
    sceGifPkAddGsAD(&packet, 3, secondColour);
    sceGifPkAddGsAD(&packet, 5, secondCorner);
    sceGifPkCloseGifTag(&packet);
    sceGifPkTerminate(&packet);
}

// 0x005d2e38
int handler_endimage(int nCause) {
    (void)nCause;
    sceDmaChan *pChannel = sceDmaGetChan(SCE_DMA_GIF);

    // The field bit sits at bit 13 of the graphics status register.
    const int field = (int)((*R_EE_GS_CSR >> 13) & 1);
    g_currentField = field;
    if (g_isDisplaying == 0) {
        ExitHandler();
        return 0;
    }
    ++g_endimageCount;
    g_syncResult = sceGsSyncPath(1, 0);
    if (g_syncResult != 0) {
        ExitHandler();
        return 0;
    }
    void *pTag = voBufGetTag(&voBuf);
    if (pTag == NULL) {
        ++g_emptyCount;
        ExitHandler();
        return 0;
    }
    unsigned char *pDraw;
    if ((g_currentField & 1) != 0) {
        pDraw = ToUncachedAddress(&db.draw1);
    } else {
        pDraw = ToUncachedAddress(&db.draw0);
    }
    // Centre the half pixel offset on the shown field.
    sceGsSetHalfOffset(pDraw, kHalfOffset, kHalfOffset, g_currentField ^ 1);
    int *pTagState = (int *)pTag;
    const int tagState = *pTagState;
    if (field == 0) {
        if (tagState == kTagDecoded) {
            sceGsSwapDBuff(&db, 0);
            sceGsSyncPath(0, 0);
            sceDmaSend(pChannel, (unsigned char *)pTag + kFirstFieldOffset);
            *pTagState = kTagFirstFieldShown;
        }
    } else if (field == 1 && tagState == kTagFirstFieldShown) {
        sceGsSwapDBuff(&db, 1);
        sceGsSyncPath(0, 0);
        sceDmaSend(pChannel, (unsigned char *)pTag + kSecondFieldOffset);
        *pTagState = kTagEmpty;
        g_frameShown = tagState;
    }
    ExitHandler();
    return 0;
}

// 0x005d3008
void startDisplay(int nWaitField) {
    // Wait for the field to move off the requested one, so playback starts on its complement.
    while (sceGsSyncV(0) == nWaitField) {
    }
    g_emptyCount = 0;
    g_isDisplaying = 1;
    g_endimageCount = 0;
}

// 0x005d3050
void endDisplay(void) {
    g_isDisplaying = 0;
    g_emptyCount = 0;
}

// 0x005d3068
int vblankHandler(int nCause) {
    (void)nCause;
    if (g_frameShown != 0) {
        voBufDecCount(&voBuf);
        g_frameShown = 0;
    }
    ExitHandler();
    return 0;
}

// 0x0059afa0
int videoCallback(sceMpeg *pMpeg, void *pCallbackData, void *pData) {
    (void)pMpeg;
    sceMpegCbDataStr *packet = (sceMpegCbDataStr *)pCallbackData;
    ReadBuf *readBuffer = (ReadBuf *)pData;

    // The bytes up to the ring end move first, and the rest wraps to the ring base.
    unsigned char *ringEnd = readBuffer->data + readBuffer->size;
    int contiguous = (int)(ringEnd - packet->data);
    if ((int)packet->len < contiguous) {
        contiguous = (int)packet->len;
    }
    int remaining = (int)packet->len - contiguous;
    unsigned char *putA;
    int sizeA;
    unsigned char *putB;
    int sizeB;
    videoDecBeginPut(&videoDec, &putA, &sizeA, &putB, &sizeB);
    int copied = cpy2area(ToUncachedAddress(putA),
                          sizeA,
                          ToUncachedAddress(putB),
                          sizeB,
                          packet->data,
                          contiguous,
                          readBuffer->data,
                          remaining);
    if (copied > 0) {
        if (videoDecPutTs(&videoDec, packet->pts, packet->dts, putA, copied) ==
            0) {
            ErrMessage("pts buffer overflow\n");
        }
    }
    videoDecEndPut(&videoDec, copied);
    return copied > 0 ? 1 : 0;
}

// 0x00567920
// Compute the two staging spans for one audio packet. Inferred.
static void AudioPutSpans(AudioDec *pFields, int *pOut0, int *pOut1, int *pOut2, int *pOut3) {
    if (pFields->state == 0) {
        *pOut0 = (int)(uintptr_t)((unsigned char *)pFields + pFields->field2c + 4);
        *pOut1 = 0x28 - pFields->field2c;
        *pOut2 = (int)(uintptr_t)pFields->buffer;
        *pOut3 = pFields->bufferSize;
        return;
    }
    {
        int nAvail = pFields->bufferSize - pFields->field34;
        int nRemaining = pFields->bufferSize - pFields->field38;

        *pOut0 = (int)(uintptr_t)(pFields->buffer + pFields->field34);
        if (nAvail >= nRemaining) {
            *pOut1 = nRemaining;
            *pOut2 = 0;
            *pOut3 = 0;
            return;
        }
        *pOut1 = nAvail;
        *pOut2 = (int)(uintptr_t)pFields->buffer;
        *pOut3 = pFields->field34 - pFields->field38;
    }
}

// 0x005679d0
// Commit copied bytes into the staging counts. Inferred.
static void AudioCommitCopied(AudioDec *pFields, int nCopied) {
    if (pFields->state == 0) {
        int nTake = 0x28 - pFields->field2c;
        int nSum;

        if (nTake > nCopied) {
            nTake = nCopied;
        }
        nSum = pFields->field2c + nTake;
        pFields->field2c = nSum;
        if (nSum >= 0x28) {
            pFields->state = 1;
        }
        nCopied -= nTake;
    }
    pFields->field38 += nCopied;
    pFields->field40 += nCopied;
    pFields->field34 = (pFields->field34 + nCopied) % pFields->bufferSize;
}

// 0x0059b0c8
int pcmCallback(sceMpeg *pMpeg, void *pCallbackData, void *pData) {
    sceMpegCbDataStr *packet = (sceMpegCbDataStr *)pCallbackData;
    ReadBuf *readBuffer = (ReadBuf *)pData;

    // Pulse code modulation packets carry a four-byte header, which the copy skips.
    unsigned char *source = packet->data + 4;
    int total = (int)packet->len - 4;
    unsigned char *ringEnd = readBuffer->data + readBuffer->size;
    int contiguous;
    int remaining;
    int staged[4];
    int copied;

    (void)pMpeg;
    if (source >= ringEnd) {
        source -= readBuffer->size;
    }
    contiguous = (int)(ringEnd - source);
    if (total < contiguous) {
        contiguous = total;
    }
    remaining = total - contiguous;
    AudioPutSpans(&audioDec, &staged[0], &staged[1], &staged[2], &staged[3]);
    copied = cpy2area((unsigned char *)(uintptr_t)staged[0],
                      staged[1],
                      (unsigned char *)(uintptr_t)staged[2],
                      staged[3],
                      source,
                      contiguous,
                      readBuffer->data,
                      remaining);
    AudioCommitCopied(&audioDec, copied);
    return copied > 0 ? 1 : 0;
}
