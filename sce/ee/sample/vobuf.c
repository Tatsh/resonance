#include "ezmpeg/vobuf.h"

#include <stddef.h>

#include <eekernel.h>
#include <ezmpeg.h>

enum {
    kTagEntrySize = 0x40440,
    kFrameDataSize = 0x151800,
    kTagDecoded = 2,
};

// 0x005d3fa0
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

// 0x005d3fe8
void voBufReset(VoBuf *pVoBuf) {
    pVoBuf->count = 0;
    pVoBuf->write = 0;
}

// 0x005d3ff8
int voBufIsFull(VoBuf *pVoBuf) {
    return pVoBuf->count == pVoBuf->size;
}

// 0x005d4010
void voBufIncCount(VoBuf *pVoBuf) {
    DIntr();
    *(int *)((unsigned char *)pVoBuf->tag + pVoBuf->write * kTagEntrySize) = kTagDecoded;
    ++pVoBuf->count;
    pVoBuf->write = (pVoBuf->write + 1) % pVoBuf->size;
    EIntr();
}

// 0x005d4088
void *voBufGetData(VoBuf *pVoBuf) {
    if (pVoBuf->count == pVoBuf->size) {
        return NULL;
    }
    return (unsigned char *)pVoBuf->data + pVoBuf->write * kFrameDataSize;
}

// 0x005d40c0
void voBufDelete(VoBuf *pVoBuf) {
    (void)pVoBuf;
}

// 0x005d40d8
void *voBufGetTag(VoBuf *pVoBuf) {
    int readIndex;

    if (pVoBuf->count == 0) {
        return NULL;
    }
    readIndex = (pVoBuf->write - pVoBuf->count + pVoBuf->size) % pVoBuf->size;
    return (unsigned char *)pVoBuf->tag + readIndex * kTagEntrySize;
}

// 0x005d4130
void voBufDecCount(VoBuf *pVoBuf) {
    if (pVoBuf->count > 0) {
        --pVoBuf->count;
    }
}
