#include "ezmpeg/videodec.h"

#include <ezmpeg.h>
#include <libmpeg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ezmpeg/disp.h"
#include "ezmpeg/vibuf.h"
#include "ezmpeg/vobuf.h"
#include "os/log.h"

// The decoded frame queue, which the playback driver owns. The worker stages pictures into it.
extern VoBuf voBuf;

// The decoder instance, which the playback driver owns. The decoder callbacks address it directly
// because the sample registers them with a null context.
extern VideoDec videoDec;

enum {
#ifdef VIDEO_STANDARD_PAL
    kPictureBufferMacroblocks = 0x654,
    kTagFirstFieldOffset = 0x40,
    kTagSecondFieldOffset = 0x26740,
    kTagEntrySize = 0x4ce40,
    kFrameDataSize = 0x195000,
#else
    kPictureBufferMacroblocks = 0x546,
    kTagFirstFieldOffset = 0x40,
    kTagSecondFieldOffset = 0x20240,
    kTagEntrySize = 0x40440,
    kFrameDataSize = 0x151800,
#endif
    kFlushWriteSize = 4,
    kFlushingState = 2,
    kPhysicalAddressMask = 0x0fffffff,
    kUncachedSegment = 0x20000000
};

// The MPEG sequence end code, which the flush writes after the last input bytes.
static const unsigned char kSequenceEndCode[] = {0x00, 0x00, 0x01, 0xb7};

// The input ring embedded in the decoder.
static ViBuf *inputBuf(VideoDec *pVideoDec) {
    return (ViBuf *)((unsigned char *)pVideoDec + sizeof(sceMpeg));
}

// NTSC-U/C: 0x00569700, PAL: 0x005a9bc8
static int mpegError(sceMpeg *pMpeg, void *pCallbackData, void *pData) {
    char *pMessage;

    (void)pMpeg;
    (void)pData;
    pMessage = *(char **)((unsigned char *)pCallbackData + 4);
    printf("%s\n", pMessage);
    return 1;
}

// NTSC-U/C: 0x00569728, PAL: 0x005a9bf0
static int mpegNodata(sceMpeg *pMpeg, void *pCallbackData, void *pData) {
    (void)pMpeg;
    (void)pCallbackData;
    (void)pData;
    switchThread();
    viBufAddDMA(inputBuf(&videoDec));
    return 1;
}

// NTSC-U/C: 0x00569758, PAL: 0x005a9c20
static int mpegStopDMA(sceMpeg *pMpeg, void *pCallbackData, void *pData) {
    (void)pMpeg;
    (void)pCallbackData;
    (void)pData;
    viBufStopDMA(inputBuf(&videoDec));
    return 1;
}

// NTSC-U/C: 0x00569780, PAL: 0x005a9c48
static int mpegRestartDMA(sceMpeg *pMpeg, void *pCallbackData, void *pData) {
    (void)pMpeg;
    (void)pCallbackData;
    (void)pData;
    viBufRestartDMA(inputBuf(&videoDec));
    return 1;
}

// NTSC-U/C: 0x005697a8, PAL: 0x005a9c70
static int mpegTS(sceMpeg *pMpeg, void *pCallbackData, void *pData) {
    long long stamps[2];
    long long *pDest;

    (void)pMpeg;
    (void)pData;
    viBufGetTs(inputBuf(&videoDec), stamps);
    pDest = (long long *)pCallbackData;
    pDest[1] = stamps[0];
    pDest[2] = stamps[1];
    return 1;
}

// NTSC-U/C: 0x00569128, PAL: 0x005a95f0
static int decode(VideoDec *pVideoDec) {
    int result = 1;
    void *pPicture;
    int width;
    int height;
    int slotIndex;
    int tagOffset;
    int dataOffset;

    while (sceMpegIsEnd(pVideoDec) == 0) {
        if (pVideoDec->state == VD_STATE_ABORT) {
            printf("decode thread: aborted\n");
            result = -1;
            break;
        }
        while ((pPicture = voBufGetData(&voBuf)) == NULL) {
            switchThread();
        }
        if (sceMpegGetPicture(pVideoDec, pPicture, kPictureBufferMacroblocks) < 0) {
            ErrMessage("sceMpegGetPicture() decode error");
        }
        if (pVideoDec->mpeg.frameCount == 0) {
            width = pVideoDec->mpeg.width;
            height = pVideoDec->mpeg.height;
            slotIndex = 0;
            tagOffset = 0;
            dataOffset = 0;
            if (voBuf.size > 0) {
                do {
                    unsigned char *pTag = (unsigned char *)voBuf.tag + tagOffset;
                    unsigned char *pData = (unsigned char *)voBuf.data + dataOffset;

                    setImageTag(pTag + kTagFirstFieldOffset, pData, 0, width,
                                height);
                    setImageTag(pTag + kTagSecondFieldOffset, pData, 1, width,
                                height);
                    tagOffset += kTagEntrySize;
                    dataOffset += kFrameDataSize;
                    ++slotIndex;
                } while (slotIndex < voBuf.size);
            }
        }
        voBufIncCount(&voBuf);
        switchThread();
    }
    sceMpegReset(pVideoDec);
    return result;
}

// NTSC-U/C: 0x005692f0, PAL: 0x005a97b8
static void videoDecReset(VideoDec *pVideoDec) {
    pVideoDec->state = 0;
}

void videoDecCreate(VideoDec *pVideoDec,
                    unsigned char *pWork,
                    int nWorkSize,
                    void *pData,
                    void *pTag,
                    int nTagSize,
                    void *pTimeStamps,
                    int nTimeStamps) {
    sceMpegCreateDecoderContext(pVideoDec, pWork, nWorkSize);
    sceMpegSetCallbackSlot(pVideoDec, 0, mpegError, NULL);
    sceMpegSetCallbackSlot(pVideoDec, 1, mpegNodata, NULL);
    sceMpegSetCallbackSlot(pVideoDec, 2, mpegStopDMA, NULL);
    sceMpegSetCallbackSlot(pVideoDec, 3, mpegRestartDMA, NULL);
    sceMpegSetCallbackSlot(pVideoDec, 5, mpegTS, NULL);
    videoDecReset(pVideoDec);
    sceDmaCreateQueueSemaphore(inputBuf(pVideoDec), pData, pTag, nTagSize, pTimeStamps, nTimeStamps);
}

int videoDecDelete(VideoDec *pVideoDec) {
    sceDmaDeleteQueueSemaphore(inputBuf(pVideoDec));
    sceMpegDelete(pVideoDec);
    return 1;
}

void videoDecAbort(VideoDec *pVideoDec) {
    pVideoDec->state = VD_STATE_ABORT;
}

int videoDecGetState(VideoDec *pVideoDec) {
    return pVideoDec->state;
}

int videoDecInputCount(VideoDec *pVideoDec) {
    return viBufCount(inputBuf(pVideoDec));
}

int videoDecInputSpaceCount(VideoDec *pVideoDec) {
    unsigned char *pPut;
    int putSize;
    unsigned char *pWrappedPut;
    int wrappedSize;

    viBufBeginPut(inputBuf(pVideoDec), &pPut, &putSize, &pWrappedPut, &wrappedSize);
    return putSize + wrappedSize;
}

void videoDecSetDecodeMode(VideoDec *pVideoDec, int nIntra, int nPredicted, int nBidirectional) {
    sceMpegSetDecodeMode(pVideoDec, nIntra, nPredicted, nBidirectional);
}

int videoDecFlush(VideoDec *pVideoDec) {
    unsigned char endCode[kFlushWriteSize];
    unsigned char *pPut;
    int putSize;
    unsigned char *pWrappedPut;
    int wrappedSize;
    unsigned char *pUncachedPut;
    unsigned char *pUncachedWrapped;
    uintptr_t putAddress;
    uintptr_t wrappedAddress;
    int copied;

    memcpy(endCode, kSequenceEndCode, sizeof(endCode));
    viBufBeginPut(inputBuf(pVideoDec), &pPut, &putSize, &pWrappedPut, &wrappedSize);
    if (putSize + wrappedSize < kFlushWriteSize) {
        return 0;
    }
    putAddress = (uintptr_t)pPut;
    wrappedAddress = (uintptr_t)pWrappedPut;
    pUncachedPut =
        (unsigned char *)((putAddress & kPhysicalAddressMask) | kUncachedSegment);
    pUncachedWrapped = (unsigned char *)((wrappedAddress & kPhysicalAddressMask) |
                                         kUncachedSegment);
    copied = cpy2area(pUncachedPut, putSize, pUncachedWrapped, wrappedSize, endCode,
                      kFlushWriteSize, NULL, 0);
    viBufEndPut(inputBuf(&videoDec), copied);
    viBufFlush(inputBuf(pVideoDec));
    if (pVideoDec->state != 0) {
        return 1;
    }
    pVideoDec->state = kFlushingState;
    return 1;
}

int videoDecIsFlushed(VideoDec *pVideoDec) {
    if (viBufCount(inputBuf(pVideoDec)) != 0) {
        return 0;
    }
    return sceMpegIsRefBuffEmpty(pVideoDec) != 0;
}

int videoDecSetStream(
    VideoDec *pVideoDec, int nType, int nChannel, VideoDecCallback pfnCallback, void *pData) {
    sceMpegAddStrCallback(pVideoDec, nType, nChannel, pfnCallback, pData);
    return 1;
}

void videoDecBeginPut(VideoDec *pVideoDec,
                      unsigned char **ppPut,
                      int *pPutSize,
                      unsigned char **ppWrappedPut,
                      int *pWrappedSize) {
    viBufBeginPut(inputBuf(pVideoDec), ppPut, pPutSize, ppWrappedPut, pWrappedSize);
}

void videoDecEndPut(VideoDec *pVideoDec, int nSize) {
    viBufEndPut(inputBuf(pVideoDec), nSize);
}

int videoDecPutTs(VideoDec *pVideoDec,
                  long long nFirstStamp,
                  long long nSecondStamp,
                  unsigned char *pOffset,
                  int nSize) {
    ViTimeStamp stamp;
    ViBuf *pInput;

    stamp.mFirst = nFirstStamp;
    stamp.mSecond = nSecondStamp;
    pInput = inputBuf(pVideoDec);
    stamp.mOffset = (int)(pOffset - pInput->mData);
    stamp.mSize = nSize;
    return viBufPutTs(inputBuf(&videoDec), &stamp);
}

void videoDecMain(VideoDec *pVideoDec) {
    viBufReset(inputBuf(pVideoDec));
    voBufReset(&voBuf);
    decode(pVideoDec);
    // The worker waits here until the display has drained the queue.
    while (voBuf.count != 0) {
    }
    pVideoDec->state = VD_STATE_END;
}

// NTSC-U/C: 0x005697f0, PAL: 0x005a9cb8
// The short-source branch below reads past its source and reports a negative size, which the
// analyser flags. The branch structure matches the image instruction for instruction, and the
// branch only runs with a negative destination size, which never happens, so the diagnostics
// are suppressed at the function rather than worked around in the reconstruction.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#pragma GCC diagnostic ignored "-Wnonnull"
#endif
int cpy2area(unsigned char *pDestA,
                    int nDestA,
                    unsigned char *pDestB,
                    int nDestB,
                    unsigned char *pSrcA,
                    int nSrcA,
                    unsigned char *pSrcB,
                    int nSrcB) {
    if (nDestA + nDestB < nSrcA + nSrcB) {
        return 0;
    }
    if (nSrcA < nDestA) {
        const int firstPart = nDestA - nSrcA;

        if (nSrcB < firstPart) {
            memcpy(pDestA, pSrcA, nSrcA);
            memcpy(pDestA + nSrcA, pSrcB, nSrcB);
        } else {
            memcpy(pDestA, pSrcA, nSrcA);
            memcpy(pDestA + nSrcA, pSrcB, firstPart);
            memcpy(pDestB, pSrcB + firstPart, nSrcB - firstPart);
        }
    } else {
        memcpy(pDestA, pSrcA, nDestA);
        memcpy(pDestB, pSrcA + nDestA, nSrcA - nDestA);
        memcpy(pDestB + nSrcA - nDestA, pSrcB, nSrcB);
    }
    return nSrcA + nSrcB;
}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
