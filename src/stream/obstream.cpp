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

// 0x004edcb8
OBStream &operator<<(OBStream &stream, long nValue) {
    const int nLow = static_cast<int>(nValue);
    return stream.Write(&nLow, sizeof(nLow));
}

// 0x004edcf8
OBStream &operator<<(OBStream &stream, unsigned long nValue) {
    const int nLow = static_cast<int>(nValue);
    return stream.Write(&nLow, sizeof(nLow));
}
