#include "ezmpeg/audiodec.h"

#include <eekernel.h>
#include <ezmpeg.h>
#include <libsdr.h>
#include <sifdev.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "os/log.h"

enum {
    kPresetSize = 0x800,
    kFullLevel = 0x3fff,
    kFullInputVolume = 0x7fff,
    kBlockStep = 0x400,
    kPositionMask = 0xffffff,
};

// The cleared block that creation uploads to the second processor side region.
static unsigned char g_presetData[kPresetSize];

// 0x00567e68
static int dmaToIop(int nIopDest, void *pSource, int nSize) {
    // Send one contiguous block to the processor side, then wait for completion.
    sceSifDmaData transfer;
    unsigned int transferId;

    if (nSize <= 0) {
        return 0;
    }
    transfer.data = (unsigned int)(uintptr_t)pSource;
    transfer.addr = (unsigned int)nIopDest;
    transfer.size = (unsigned int)nSize;
    transfer.mode = 0;
    FlushCache(WRITEBACK_DCACHE);
    transferId = sceSifSetDma(&transfer, 1);
    while (sceSifDmaStat(transferId) >= 0) {
    }
    return nSize;
}

// 0x00567c78
static void splitIopSpan(int *pWrite,
                         int *pWriteLen,
                         int *pWrap,
                         int *pWrapLen,
                         AudioDec *pWork,
                         int nEnd) {
    // Describe the span ending at nEnd as up to two processor side blocks, wrapping at the
    // buffer size. The first block starts at the write offset, and the second restarts at the
    // buffer base when the span crosses the end.
    int aligned;
    int span;

    nEnd += pWork->iopBufferSize;
    nEnd -= pWork->iopOffset;
    span = pWork->iopBufferSize - pWork->iopOffset;
    nEnd -= kBlockStep;
    nEnd %= pWork->iopBufferSize;
    aligned = nEnd / kBlockStep * kBlockStep;
    *pWrite = pWork->iopBuffer + pWork->iopOffset;
    if (span < aligned) {
        *pWriteLen = pWork->iopBufferSize - pWork->iopOffset;
        *pWrap = pWork->iopBuffer;
        *pWrapLen = aligned - (pWork->iopBufferSize - pWork->iopOffset);
    } else {
        *pWriteLen = aligned;
        *pWrap = 0;
        *pWrapLen = 0;
    }
}

// 0x00567d20
static int sendWrappedToIop(int nIopDest,
                            int nFirstLen,
                            int nIopWrap,
                            int nLimitExtra,
                            unsigned char *pSource,
                            int nCount,
                            unsigned char *pWrapSource,
                            int nTrailing) {
    // Copy across both ring wraps in up to three direct memory access blocks. The read span
    // runs from pSource for nCount bytes and continues at pWrapSource for nTrailing bytes,
    // while the write span runs from nIopDest and continues at nIopWrap.
    int limit = nFirstLen + nLimitExtra;
    int total = nCount + nTrailing;

    if (limit < total) {
        int over = total - limit;
        if (over < nTrailing) {
            nTrailing -= over;
        } else {
            nCount -= over - nTrailing;
            nTrailing = 0;
        }
    }
    if (nCount < nFirstLen) {
        int tail = nFirstLen - nCount;

        dmaToIop(nIopDest, pSource, nCount);
        if (nTrailing < tail) {
            dmaToIop(nIopDest + nCount, pWrapSource, nTrailing);
        } else {
            dmaToIop(nIopDest + nCount, pWrapSource, tail);
            dmaToIop(nIopWrap, pWrapSource + tail, nTrailing - tail);
        }
    } else {
        dmaToIop(nIopDest, pSource, nFirstLen);
        dmaToIop(nIopWrap, pSource + nFirstLen, nCount - nFirstLen);
        dmaToIop(nIopWrap + nCount - nFirstLen, pWrapSource, nTrailing);
    }
    return nCount + nTrailing;
}

// 0x00567ee0
static void setupIopVoices(int nLevel) {
    // Apply the level to both voice pairs through the sound driver.
    int core;

    for (core = 0; core < 2; ++core) {
        sceSdRemote(1, 0x8010, 0x980 | core, nLevel);
        sceSdRemote(1, 0x8010, 0xa80 | core, nLevel);
    }
}

// 0x00567f48
static void setIopInputVolume(int nVolume) {
    // Set the left and right sound data input volume of the second core through the sound driver.
    sceSdRemote(1, 0x8010, 0xf81, nVolume);
    sceSdRemote(1, 0x8010, 0x1081, nVolume);
}

// 0x00567670
int audioDecSendToIOP(AudioDec *pAudioDec) {
    // Move the staged bytes to the processor side, following the transfer stage. The idle and
    // stopping stages transfer nothing, the priming stage offers the whole free span, and the
    // streaming stage queries the driver for the write position first. The staged counts are read
    // after the driver call.
    int pending;
    int total;
    int aligned;
    // Both writers below fill every slot, but the stage chain leaves the array untouched on its
    // early exits, so the staging starts cleared.
    int span[4] = {0, 0, 0, 0};
    int transferred = 0;
    int ready;
    int extra;
    int rest;
    int chunk;
    unsigned char *pRead;
    int remaining;
    int writePos;

    if (pAudioDec->state == 1) {
        span[1] = pAudioDec->iopBufferSize - pAudioDec->totalBytesSent;
        span[0] = pAudioDec->iopBuffer + pAudioDec->totalBytesSent % pAudioDec->iopBufferSize;
        span[2] = 0;
        span[3] = 0;
    } else if (pAudioDec->state < 2) {
        if (pAudioDec->state == 0) {
            return 0;
        }
    } else if (pAudioDec->state == 2) {
        const int position = sceSdRemote(1, 0x8100, 1);

        splitIopSpan(&span[0],
                     &span[1],
                     &span[2],
                     &span[3],
                     pAudioDec,
                     (position & kPositionMask) - pAudioDec->iopBuffer);
    } else if (pAudioDec->state == 3) {
        return 0;
    }
    pending = pAudioDec->count;
    total = pAudioDec->put - pending + pAudioDec->bufferSize;
    aligned = pending / kBlockStep * kBlockStep;
    ready = span[1];
    extra = span[3];
    rest = total % pAudioDec->bufferSize;
    chunk = pAudioDec->bufferSize - rest;
    pRead = pAudioDec->buffer + rest;
    if (aligned < chunk) {
        chunk = aligned;
    }
    remaining = aligned - chunk;
    writePos = pAudioDec->iopOffset;
    if (ready + extra >= kBlockStep && chunk + remaining >= kBlockStep) {
        transferred = sendWrappedToIop(span[0],
                                       ready,
                                       span[2],
                                       extra,
                                       pRead,
                                       chunk,
                                       pAudioDec->buffer,
                                       remaining);
    }
    writePos += transferred;
    pAudioDec->count -= transferred;
    pAudioDec->totalBytesSent += transferred;
    pAudioDec->iopOffset = writePos % pAudioDec->iopBufferSize;
    return transferred;
}

// 0x00567820
int audioDecCreate(AudioDec *pAudioDec,
                   unsigned char *pBuffer,
                   int nBufferSize,
                   int nIopBufferSize) {
    // Clear the decoder, reserve both processor side regions, and upload the preset block. A
    // failed reservation reports through the console and the routine returns zero.
    pAudioDec->state = 0;
    pAudioDec->headerCount = 0;
    pAudioDec->put = 0;
    pAudioDec->count = 0;
    pAudioDec->totalBytes = 0;
    pAudioDec->totalBytesSent = 0;
    pAudioDec->iopOffset = 0;
    pAudioDec->iopPauseOffset = 0;
    pAudioDec->buffer = pBuffer;
    pAudioDec->bufferSize = nBufferSize;
    pAudioDec->iopBufferSize = nIopBufferSize;
    pAudioDec->iopBuffer = (int)(uintptr_t)sceSifAllocIopHeap(nIopBufferSize);
    if (pAudioDec->iopBuffer < 0) {
        LogPrintf("Cannot allocate IOP memory\n");
        return 0;
    }
    pAudioDec->iopExtra = (int)(uintptr_t)sceSifAllocIopHeap(kPresetSize);
    if (pAudioDec->iopExtra < 0) {
        LogPrintf("Cannot allocate IOP memory\n");
        return 0;
    }
    memset(g_presetData, 0, kPresetSize);
    dmaToIop(pAudioDec->iopExtra, g_presetData, kPresetSize);
    setupIopVoices(kFullLevel);
    return 1;
}

// 0x005678e0
int audioDecDelete(AudioDec *pAudioDec) {
    // Release both processor side regions and silence the voices.
    sceSifFreeIopHeap((void *)(uintptr_t)pAudioDec->iopBuffer);
    sceSifFreeIopHeap((void *)(uintptr_t)pAudioDec->iopExtra);
    setupIopVoices(0);
    return 1;
}

// 0x00567a50
int audioDecIsPreset(AudioDec *pAudioDec) {
    // Report whether the handed count has reached the processor side buffer size.
    return pAudioDec->totalBytesSent >= pAudioDec->iopBufferSize;
}

// 0x00567a68
void audioDecStart(AudioDec *pAudioDec) {
    // Raise the input volume, hand the buffer range to the driver, and enter streaming.
    const int aligned = pAudioDec->iopBufferSize / kBlockStep * kBlockStep;

    setIopInputVolume(kFullInputVolume);
    sceSdRemote(1,
                0x80e0,
                1,
                0x13,
                pAudioDec->iopBuffer,
                aligned,
                pAudioDec->iopBuffer + pAudioDec->iopPauseOffset);
    pAudioDec->state = 2;
}

// 0x00567ad8
void audioDecReset(AudioDec *pAudioDec) {
    // Stop the driver, park its reply, and clear the decoder back to idle.
    int position;

    pAudioDec->state = 3;
    setIopInputVolume(0);
    sceSdRemote(1, 0x80e0, 1, 2, 0, 0);
    position = sceSdRemote(1, 0x80d0, 1, 0, pAudioDec->iopExtra, 0x4000, 0x800);
    pAudioDec->iopPauseOffset = (position & kPositionMask) - pAudioDec->iopBuffer;
    pAudioDec->iopPauseOffset = 0;
    pAudioDec->state = 0;
    pAudioDec->headerCount = 0;
    pAudioDec->put = 0;
    pAudioDec->count = 0;
    pAudioDec->totalBytes = 0;
    pAudioDec->totalBytesSent = 0;
    pAudioDec->iopOffset = 0;
}
