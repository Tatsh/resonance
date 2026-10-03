#include "ezmpeg/readbuf.h"

#include <ezmpeg.h>

// NTSC-U/C: 0x005cb2b8, PAL: 0x0060d218
void readBufCreate(ReadBuf *pReadBuf) {
    // Reset the offsets and restore the full capacity.
    pReadBuf->put = 0;
    pReadBuf->count = 0;
    pReadBuf->size = (int)sizeof(pReadBuf->data);
}

// NTSC-U/C: 0x005cb2d0, PAL: 0x0060d230
void readBufDelete(ReadBuf *pReadBuf) {
    // The buffer owns no resources, so deletion performs no work.
    (void)pReadBuf;
}

// NTSC-U/C: 0x005cb2d8, PAL: 0x0060d238
int readBufBeginPut(ReadBuf *pReadBuf, unsigned char **ppPut) {
    // Offer the write pointer with the free byte count.
    int freeSize = pReadBuf->size - pReadBuf->count;
    if (freeSize != 0) {
        *ppPut = &pReadBuf->data[pReadBuf->put];
    }
    return freeSize;
}

// NTSC-U/C: 0x005cb308, PAL: 0x0060d268
int readBufEndPut(ReadBuf *pReadBuf, int nSize) {
    // Advance the write pointer past the stored bytes, wrapping at the capacity.
    int putSize = pReadBuf->size - pReadBuf->count;
    if (nSize < putSize) {
        putSize = nSize;
    }
    pReadBuf->put = (pReadBuf->put + putSize) % pReadBuf->size;
    pReadBuf->count += putSize;
    return putSize;
}

// NTSC-U/C: 0x005cb350, PAL: 0x0060d2b0
int readBufBeginGet(ReadBuf *pReadBuf, unsigned char **ppGet) {
    // Offer the read pointer with the held byte count.
    int heldSize = pReadBuf->count;
    if (heldSize != 0) {
        int get = (pReadBuf->put - heldSize + pReadBuf->size) % pReadBuf->size;
        *ppGet = &pReadBuf->data[get];
    }
    return heldSize;
}

// NTSC-U/C: 0x005cb398, PAL: 0x0060d2f8
int readBufEndGet(ReadBuf *pReadBuf, int nSize) {
    // Discard the consumed bytes from the held count.
    int getSize = pReadBuf->count;
    if (nSize < getSize) {
        getSize = nSize;
    }
    pReadBuf->count -= getSize;
    return getSize;
}
