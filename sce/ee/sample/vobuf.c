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

// NTSC-U/C: 0x005d3fa0, PAL: 0x00616008
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

// NTSC-U/C: 0x005d3fe8, PAL: 0x00616050
void voBufReset(VoBuf *pVoBuf) {
    pVoBuf->count = 0;
    pVoBuf->write = 0;
}

// NTSC-U/C: 0x005d3ff8, PAL: 0x00616060
int voBufIsFull(VoBuf *pVoBuf) {
    return pVoBuf->count == pVoBuf->size;
}

// NTSC-U/C: 0x005d4010, PAL: 0x00616078
void voBufIncCount(VoBuf *pVoBuf) {
    DIntr();
    *(int *)((unsigned char *)pVoBuf->tag + pVoBuf->write * kTagEntrySize) = kTagDecoded;
    ++pVoBuf->count;
    pVoBuf->write = (pVoBuf->write + 1) % pVoBuf->size;
    EIntr();
}

// NTSC-U/C: 0x005d4088, PAL: 0x006160f0
void *voBufGetData(VoBuf *pVoBuf) {
    if (pVoBuf->count == pVoBuf->size) {
        return NULL;
    }
    return (unsigned char *)pVoBuf->data + pVoBuf->write * kFrameDataSize;
}

// NTSC-U/C: 0x005d40c0, PAL: 0x00616128
void voBufDelete(VoBuf *pVoBuf) {
    (void)pVoBuf;
}

// NTSC-U/C: 0x005d40d8, PAL: 0x00616140
void *voBufGetTag(VoBuf *pVoBuf) {
    int readIndex;

    if (pVoBuf->count == 0) {
        return NULL;
    }
    readIndex = (pVoBuf->write - pVoBuf->count + pVoBuf->size) % pVoBuf->size;
    return (unsigned char *)pVoBuf->tag + readIndex * kTagEntrySize;
}

// NTSC-U/C: 0x005d4130, PAL: 0x00616198
void voBufDecCount(VoBuf *pVoBuf) {
    if (pVoBuf->count > 0) {
        --pVoBuf->count;
    }
}
