#include "stream/ibstream.h"

// NTSC-U/C: 0x004ed838, PAL: 0x0052c3e0
IBStream &IBStream::Read(void *pDest, int nSize) {
    return ReadBytes(pDest, nSize);
}

// NTSC-U/C: 0x004edb78, PAL: 0x0052c720
IBStream &operator>>(IBStream &stream, int &bValue) {
    char cValue;
    stream.ReadBytes(&cValue, sizeof(cValue));
    bValue = (cValue != 0) ? 1 : 0;
    return stream;
}

// NTSC-U/C: 0x004edbd0, PAL: 0x0052c778
IBStream &operator>>(IBStream &stream, long &nValue) {
    int nStored;
    stream.Read(&nStored, sizeof(nStored));
    nValue = nStored;
    return stream;
}

// NTSC-U/C: 0x004edc28, PAL: 0x0052c7d0
IBStream &operator>>(IBStream &stream, unsigned long &nValue) {
    unsigned nStored;
    stream.Read(&nStored, sizeof(nStored));
    nValue = nStored;
    return stream;
}
