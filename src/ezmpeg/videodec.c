#include "ezmpeg/videodec.h"

#include <ezmpeg.h>
#include <libmpeg.h>
#include <stdint.h>
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
    kDecodePictureMode = 0x546,
    kTagFirstFieldOffset = 0x40,
    kTagSecondFieldOffset = 0x20240,
    kTagEntrySize = 0x40440,
    kFrameDataSize = 0x151800,
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

static int mpegError(sceMpeg *pMpeg, void *pCallbackData, void *pData);
static int mpegNodata(sceMpeg *pMpeg, void *pCallbackData, void *pData);
static int mpegStopDMA(sceMpeg *pMpeg, void *pCallbackData, void *pData);
static int mpegRestartDMA(sceMpeg *pMpeg, void *pCallbackData, void *pData);
static int mpegTS(sceMpeg *pMpeg, void *pCallbackData, void *pData);

// 0x00569128
static int decode(VideoDec *pVideoDec) {
    int result = 1;
    void *pPicture;
    int contextWordZero;
    int contextWordOne;
    int slotIndex;
    int tagOffset;
    int dataOffset;

    while (sceMpegGetContextWordZero(pVideoDec) == 0) {
        if (pVideoDec->state == VD_STATE_ABORT) {
            LogPrintf("decode thread: aborted");
            result = -1;
            break;
        }
        do {
            switchThread();
            pPicture = voBufGetData(&voBuf);
        } while (pPicture == NULL);
        if (sceMpegSub005e07b0(pVideoDec, pPicture, kDecodePictureMode) < 0) {
            ErrMessage("sceMpegGetPicture() decode error");
        }
        if (pVideoDec->mpeg.frameCount == 0) {
            contextWordZero = pVideoDec->mpeg.mUnknown00[0];
            contextWordOne = pVideoDec->mpeg.mUnknown00[1];
            slotIndex = 0;
            tagOffset = 0;
            dataOffset = 0;
            if (voBuf.size > 0) {
                do {
                    unsigned char *pTag = (unsigned char *)voBuf.tag + tagOffset;
                    unsigned char *pData = (unsigned char *)voBuf.data + dataOffset;

                    setImageTag(pTag + kTagFirstFieldOffset, pData, 0, contextWordZero,
                                contextWordOne);
                    setImageTag(pTag + kTagSecondFieldOffset, pData, 1, contextWordZero,
                                contextWordOne);
                    tagOffset += kTagEntrySize;
                    dataOffset += kFrameDataSize;
                    ++slotIndex;
                } while (slotIndex < voBuf.size);
            }
        }
        voBufIncCount(&voBuf);
        switchThread();
    }
    sceMpegSub005e08e8(pVideoDec);
    return result;
}

// 0x005692f0
static void MpegDecoderRoutine005692f0(VideoDec *pVideoDec) {
    pVideoDec->state = 0;
}

// 0x005692f8
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
    MpegDecoderRoutine005692f0(pVideoDec);
    sceDmaCreateQueueSemaphore(inputBuf(pVideoDec), pData, pTag, nTagSize, pTimeStamps, nTimeStamps);
}

// 0x005693f8
int videoDecDelete(VideoDec *pVideoDec) {
    sceDmaDeleteQueueSemaphore(inputBuf(pVideoDec));
    sceMpegReturnOne(pVideoDec);
    return 1;
}

// 0x00569430
void videoDecAbort(VideoDec *pVideoDec) {
    pVideoDec->state = VD_STATE_ABORT;
}

// 0x00569440
int videoDecGetState(VideoDec *pVideoDec) {
    return pVideoDec->state;
}

// 0x00569458
int videoDecInputCount(VideoDec *pVideoDec) {
    return viBufCount(inputBuf(pVideoDec));
}

// 0x00569478
int videoDecInputSpaceCount(VideoDec *pVideoDec) {
    unsigned char *pPut;
    int putSize;
    unsigned char *pWrappedPut;
    int wrappedSize;

    viBufBeginPut(inputBuf(pVideoDec), &pPut, &putSize, &pWrappedPut, &wrappedSize);
    return putSize + wrappedSize;
}

// 0x005694b0
void videoDecReset(VideoDec *pVideoDec, int nArgA, int nArgB, int nArgC) {
    sceMpegSub005e0890(pVideoDec, nArgA, nArgB, nArgC);
}

// 0x005694d0
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
    sceDmaSub00613798(inputBuf(pVideoDec));
    if (pVideoDec->state != 0) {
        return 1;
    }
    pVideoDec->state = kFlushingState;
    return 1;
}

// 0x005695b0
int videoDecIsFlushed(VideoDec *pVideoDec) {
    if (viBufCount(inputBuf(pVideoDec)) != 0) {
        return 0;
    }
    return sceMpegIsContextWordFourClear(pVideoDec) != 0;
}

// 0x00569600
int videoDecSetStream(
    VideoDec *pVideoDec, int nType, int nChannel, VideoDecCallback pfnCallback, void *pData) {
    sceMpegAddStrCallback(pVideoDec, nType, nChannel, pfnCallback, pData);
    return 1;
}

// 0x00569620
void videoDecBeginPut(VideoDec *pVideoDec,
                      unsigned char **ppPut,
                      int *pPutSize,
                      unsigned char **ppWrappedPut,
                      int *pWrappedSize) {
    viBufBeginPut(inputBuf(pVideoDec), ppPut, pPutSize, ppWrappedPut, pWrappedSize);
}

// 0x00569640
void videoDecEndPut(VideoDec *pVideoDec, int nSize) {
    viBufEndPut(inputBuf(pVideoDec), nSize);
}

// 0x00569660
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

// 0x005696a0
void videoDecMain(VideoDec *pVideoDec) {
    sceDmaSub006126e8(inputBuf(pVideoDec));
    voBufReset(&voBuf);
    decode(pVideoDec);
    // The worker waits here until the display has drained the queue.
    while (voBuf.count != 0) {
    }
    pVideoDec->state = VD_STATE_END;
}

// 0x00569700
static int mpegError(sceMpeg *pMpeg, void *pCallbackData, void *pData) {
    char *pMessage;

    (void)pMpeg;
    (void)pData;
    pMessage = *(char **)((unsigned char *)pCallbackData + 4);
    LogPrintf("%s", pMessage);
    return 1;
}

// 0x00569728
static int mpegNodata(sceMpeg *pMpeg, void *pCallbackData, void *pData) {
    (void)pMpeg;
    (void)pCallbackData;
    (void)pData;
    switchThread();
    sceDmaSub00612890(inputBuf(&videoDec));
    return 1;
}

// 0x00569758
static int mpegStopDMA(sceMpeg *pMpeg, void *pCallbackData, void *pData) {
    (void)pMpeg;
    (void)pCallbackData;
    (void)pData;
    sceDmaSub00612b40(inputBuf(&videoDec));
    return 1;
}

// 0x00569780
static int mpegRestartDMA(sceMpeg *pMpeg, void *pCallbackData, void *pData) {
    (void)pMpeg;
    (void)pCallbackData;
    (void)pData;
    sceDmaSub00612cc0(inputBuf(&videoDec));
    return 1;
}

// 0x005697a8
static int mpegTS(sceMpeg *pMpeg, void *pCallbackData, void *pData) {
    long long stamps[2];
    long long *pDest;

    (void)pMpeg;
    (void)pData;
    sceDmaSub006131e0(inputBuf(&videoDec), stamps);
    pDest = (long long *)pCallbackData;
    pDest[1] = stamps[0];
    pDest[2] = stamps[1];
    return 1;
}

// 0x005697f0
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
            memcpy(pDestA + nSrcA, pSrcB, firstPart);
            memcpy(pDestB, pSrcB + nDestA - nSrcA, nSrcB - firstPart);
        } else {
            memcpy(pDestA, pSrcA, nSrcA);
            memcpy(pDestA + nSrcA, pSrcB, nSrcB);
        }
    } else {
        memcpy(pDestA, pSrcA, nDestA);
        memcpy(pDestB, pSrcA + nDestA, nSrcA - nDestA);
        memcpy(pDestB + nSrcA - nDestA, pSrcB, nSrcB);
    }
    return nSrcA + nSrcB;
}
