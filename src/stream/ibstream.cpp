#include "stream/ibstream.h"

IBStream &IBStream::ReadLE(void *pDest, int nSize) {
    return Read(pDest, nSize);
}

IBStream &operator>>(IBStream &stream, int &bValue) {
    char cValue;
    stream.Read(&cValue, sizeof(cValue));
    bValue = (cValue != 0) ? 1 : 0;
    return stream;
}

IBStream &operator>>(IBStream &stream, long &nValue) {
    int nStored;
    stream.ReadLE(&nStored, sizeof(nStored));
    nValue = nStored;
    return stream;
}

IBStream &operator>>(IBStream &stream, unsigned long &nValue) {
    unsigned nStored;
    stream.ReadLE(&nStored, sizeof(nStored));
    nValue = nStored;
    return stream;
}
