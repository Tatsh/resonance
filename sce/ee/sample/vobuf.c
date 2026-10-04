#include "ezmpeg/vobuf.h"

#include <stddef.h>

#include <eekernel.h>
#include <ezmpeg.h>

enum {
#ifdef VIDEO_STANDARD_PAL
    kTagEntrySize = 0x4ce40,
    kFrameDataSize = 0x195000,
#else
    kTagEntrySize = 0x40440,
    kFrameDataSize = 0x151800,
#endif
    kTagDecoded = 2,
};

void voBufCreate(VoBuf *pVoBuf, void *pData, void *pTag, int nFrames) {
    unsigned char *tagBytes;
    int offset;
    int remaining;

    pVoBuf->count = 0;
    pVoBuf->data = pData;
    pVoBuf->tag = pTag;
    pVoBuf->size = nFrames;
    pVoBuf->write = 0;
    if (nFrames <= 0) {
        return;
    }
    tagBytes = (unsigned char *)pTag;
    offset = 0;
    remaining = nFrames;
    do {
        *(int *)(tagBytes + offset) = 0;
        offset += kTagEntrySize;
        --remaining;
    } while (remaining != 0);
}

void voBufReset(VoBuf *pVoBuf) {
    pVoBuf->count = 0;
    pVoBuf->write = 0;
}

int voBufIsFull(VoBuf *pVoBuf) {
    return pVoBuf->count == pVoBuf->size;
}

void voBufIncCount(VoBuf *pVoBuf) {
    DIntr();
    *(int *)((unsigned char *)pVoBuf->tag + pVoBuf->write * kTagEntrySize) = kTagDecoded;
    ++pVoBuf->count;
    pVoBuf->write = (pVoBuf->write + 1) % pVoBuf->size;
    EIntr();
}

void *voBufGetData(VoBuf *pVoBuf) {
    if (pVoBuf->count == pVoBuf->size) {
        return NULL;
    }
    return (unsigned char *)pVoBuf->data + pVoBuf->write * kFrameDataSize;
}

void voBufDelete(VoBuf *pVoBuf) {
    (void)pVoBuf;
}

void *voBufGetTag(VoBuf *pVoBuf) {
    int readIndex;

    if (pVoBuf->count == 0) {
        return NULL;
    }
    readIndex = (pVoBuf->write - pVoBuf->count + pVoBuf->size) % pVoBuf->size;
    return (unsigned char *)pVoBuf->tag + readIndex * kTagEntrySize;
}

void voBufDecCount(VoBuf *pVoBuf) {
    if (pVoBuf->count > 0) {
        --pVoBuf->count;
    }
}
