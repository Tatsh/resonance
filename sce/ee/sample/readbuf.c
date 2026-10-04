#include "ezmpeg/readbuf.h"

#include <ezmpeg.h>

void readBufCreate(ReadBuf *pReadBuf) {
    // Reset the offsets and restore the full capacity.
    pReadBuf->put = 0;
    pReadBuf->count = 0;
    pReadBuf->size = (int)sizeof(pReadBuf->data);
}

void readBufDelete(ReadBuf *pReadBuf) {
    // The buffer owns no resources, so deletion performs no work.
    (void)pReadBuf;
}

int readBufBeginPut(ReadBuf *pReadBuf, unsigned char **ppPut) {
    // Offer the write pointer with the free byte count.
    int freeSize = pReadBuf->size - pReadBuf->count;
    if (freeSize != 0) {
        *ppPut = &pReadBuf->data[pReadBuf->put];
    }
    return freeSize;
}

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

int readBufBeginGet(ReadBuf *pReadBuf, unsigned char **ppGet) {
    // Offer the read pointer with the held byte count.
    int heldSize = pReadBuf->count;
    if (heldSize != 0) {
        int get = (pReadBuf->put - heldSize + pReadBuf->size) % pReadBuf->size;
        *ppGet = &pReadBuf->data[get];
    }
    return heldSize;
}

int readBufEndGet(ReadBuf *pReadBuf, int nSize) {
    // Discard the consumed bytes from the held count.
    int getSize = pReadBuf->count;
    if (nSize < getSize) {
        getSize = nSize;
    }
    pReadBuf->count -= getSize;
    return getSize;
}
