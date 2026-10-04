#include "stream/hxstream.h"

#include "os/hxstr.h"
#include "stream/hxvarlennumber.h"

const int HxStream::goodbit = 0;
const int HxStream::kStatusEnd = 1;
const int HxStream::failbit = 2;
const int HxStream::badbit = 4;

HxStream::HxStream() : mSwapBytes(0), mStatus(0), mFatalOnEnd(0) {
}

HxStream::~HxStream() {
}

void HxStream::SetMarker([[maybe_unused]] int nOffset, [[maybe_unused]] int nWhence) {
}

int HxStream::GetMarker() {
    return 0;
}

int HxStream::Size() {
    return 0;
}

HxStream &HxStream::Write([[maybe_unused]] const void *pSrc, [[maybe_unused]] int nSize) {
    return *this;
}

HxStream &HxStream::ReadData([[maybe_unused]] void *pDest, [[maybe_unused]] int nSize) {
    return *this;
}

HxStream *HxStream::UnderlyingStream() {
    return nullptr;
}

HxStream &HxStream::ReadNum(void *pDest, int nSize) {
    if (mSwapBytes == 0 || nSize == 1) {
        return ReadData(pDest, nSize);
    }

    unsigned char *pBytes = static_cast<unsigned char *>(pDest);
    while (nSize != 0) {
        --nSize;
        ReadData(&pBytes[nSize], 1);
    }
    return *this;
}

HxStream &HxStream::WriteNum(const void *pSrc, int nSize) {
    if (mSwapBytes == 0 || nSize == 1) {
        return Write(pSrc, nSize);
    }

    const unsigned char *pBytes = static_cast<const unsigned char *>(pSrc);
    while (nSize != 0) {
        --nSize;
        Write(&pBytes[nSize], 1);
    }
    return *this;
}

HxStream &HxStream::ReadStr(char *pszDest, int nDestSize) {
    HxVarLenNumber length;
    length.Read(*this);
    if (length < nDestSize) {
        ReadData(pszDest, length);
        pszDest[length] = '\0';
        return *this;
    }
    ReadData(pszDest, nDestSize - 1);
    pszDest[nDestSize - 1] = '\0';
    SetMarker(length - nDestSize + 1, kHxSeekCur);
    return *this;
}

HxStream &HxStream::ReadString(HxStr &str) {
    HxVarLenNumber length;
    length.Read(*this);
    char *pszBuffer = new char[length + 1];
    ReadData(pszBuffer, length);
    pszBuffer[length] = '\0';
    str = pszBuffer;
    delete[] pszBuffer;
    return *this;
}

HxStream *HxStream::GetRootStream() {
    HxStream *pStream = this;
    while (HxStream *pInner = pStream->UnderlyingStream()) {
        pStream = pInner;
    }
    return pStream;
}
