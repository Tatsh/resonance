#include "stream/obstream.h"

// 0x004ed860
OBStream &OBStream::Write(const void *pSrc, int nSize) {
    return WriteBytes(pSrc, nSize);
}
