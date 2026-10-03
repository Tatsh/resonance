#include "stream/obstream.h"

// NTSC-U/C: 0x004ed860, PAL: 0x0052c408
OBStream &OBStream::Write(const void *pSrc, int nSize) {
    return WriteBytes(pSrc, nSize);
}

// NTSC-U/C: 0x004edc80, PAL: 0x0052c828
OBStream &operator<<(OBStream &stream, int bValue) {
    char cValue = static_cast<char>(bValue);
    return stream.WriteBytes(&cValue, sizeof(cValue));
}

// NTSC-U/C: 0x004edcb8, PAL: 0x0052c860
OBStream &operator<<(OBStream &stream, long nValue) {
    const int nLow = static_cast<int>(nValue);
    return stream.Write(&nLow, sizeof(nLow));
}

// NTSC-U/C: 0x004edcf8, PAL: 0x0052c8a0
OBStream &operator<<(OBStream &stream, unsigned long nValue) {
    const int nLow = static_cast<int>(nValue);
    return stream.Write(&nLow, sizeof(nLow));
}
