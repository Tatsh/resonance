#include "stream/ibstream.h"

// 0x004ed838
IBStream &IBStream::Read(void *pDest, int nSize) {
    return ReadBytes(pDest, nSize);
}
