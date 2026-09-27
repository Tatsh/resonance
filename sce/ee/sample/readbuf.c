#include "ezmpeg/readbuf.h"

#include <ezmpeg.h>

// 0x005cb2b8
void readBufCreate(ReadBuf *pReadBuf) {
    // Reset the offsets and restore the full capacity.
    pReadBuf->put = 0;
    pReadBuf->count = 0;
    pReadBuf->size = (int)sizeof(pReadBuf->data);
}

// 0x005cb2d0
void readBufDelete(ReadBuf *pReadBuf) {
    // The buffer owns no resources, so deletion performs no work.
    (void)pReadBuf;
}

// 0x005cb2d8
int readBufBeginPut(ReadBuf *pReadBuf, unsigned char **ppPut) {
    // Offer the write pointer with the free byte count.
    int freeSize = pReadBuf->size - pReadBuf->count;
    if (freeSize != 0) {
        *ppPut = &pReadBuf->data[pReadBuf->put];
    }
    return freeSize;
}

// 0x005cb308
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

// 0x005cb350
int readBufBeginGet(ReadBuf *pReadBuf, unsigned char **ppGet) {
    // Offer the read pointer with the held byte count.
    int heldSize = pReadBuf->count;
    if (heldSize != 0) {
        int get = (pReadBuf->put - heldSize + pReadBuf->size) % pReadBuf->size;
        *ppGet = &pReadBuf->data[get];
    }
    return heldSize;
}

// 0x005cb398
int readBufEndGet(ReadBuf *pReadBuf, int nSize) {
    // Discard the consumed bytes from the held count.
    int getSize = pReadBuf->count;
    if (nSize < getSize) {
        getSize = nSize;
    }
    pReadBuf->count -= getSize;
    return getSize;
}
