#include "stream/obstream.h"

// 0x004ed860
OBStream &OBStream::Write(const void *pSrc, int nSize) {
    return WriteBytes(pSrc, nSize);
}

// 0x004edc80
OBStream &operator<<(OBStream &stream, int bValue) {
    char cValue = static_cast<char>(bValue);
    return stream.WriteBytes(&cValue, sizeof(cValue));
}
