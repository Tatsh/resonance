#include "stream/obstream.h"

OBStream &OBStream::WriteLE(const void *pSrc, int nSize) {
    return Write(pSrc, nSize);
}

OBStream &operator<<(OBStream &stream, int bValue) {
    char cValue = static_cast<char>(bValue);
    return stream.Write(&cValue, sizeof(cValue));
}

OBStream &operator<<(OBStream &stream, long nValue) {
    const int nLow = static_cast<int>(nValue);
    return stream.WriteLE(&nLow, sizeof(nLow));
}

OBStream &operator<<(OBStream &stream, unsigned long nValue) {
    const int nLow = static_cast<int>(nValue);
    return stream.WriteLE(&nLow, sizeof(nLow));
}
