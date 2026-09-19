#include "stream/ibstream.h"

// 0x004ed838
IBStream &IBStream::Read(void *pDest, int nSize) {
    return ReadBytes(pDest, nSize);
}

// 0x004edb78
IBStream &operator>>(IBStream &stream, int &bValue) {
    char cValue;
    stream.ReadBytes(&cValue, sizeof(cValue));
    bValue = (cValue != 0) ? 1 : 0;
    return stream;
}
