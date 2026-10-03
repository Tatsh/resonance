#include "stream/hxstream.h"

#include "os/hxstr.h"

namespace {

constexpr int kVarLenBits = 7;
constexpr unsigned char kVarLenValueMask = 0x7f;
constexpr unsigned char kVarLenContinue = 0x80;
constexpr int kBitsPerByte = 8;

} // namespace

const int HxStream::goodbit = 0;
const int HxStream::kStatusEnd = 1;
const int HxStream::failbit = 2;
const int HxStream::badbit = 4;

// NTSC-U/C: 0x004057a8, PAL: 0x0043f098
HxStream::HxStream() {
    mSwapBytes = 0;
    mStatus = 0;
    mFatalOnEnd = 0;
}

// NTSC-U/C: 0x00145ee8, PAL: 0x00146a00
HxStream::~HxStream() {
}

// NTSC-U/C: 0x00145f18, PAL: 0x00146a30
void HxStream::SetMarker([[maybe_unused]] int nOffset, [[maybe_unused]] int nWhence) {
}

// NTSC-U/C: 0x00145f20, PAL: 0x00146a38
int HxStream::GetMarker() {
    return 0;
}

// NTSC-U/C: 0x00145f28, PAL: 0x00146a40
int HxStream::Size() {
    return 0;
}

// NTSC-U/C: 0x00145f30, PAL: 0x00146a48
HxStream &HxStream::Write([[maybe_unused]] const void *pSrc, [[maybe_unused]] int nSize) {
    return *this;
}

// NTSC-U/C: 0x00145f38, PAL: 0x00146a50
HxStream &HxStream::ReadData([[maybe_unused]] void *pDest, [[maybe_unused]] int nSize) {
    return *this;
}

// NTSC-U/C: 0x00145f40, PAL: 0x00146a58
HxStream *HxStream::UnderlyingStream() {
    return nullptr;
}

// NTSC-U/C: 0x004059f8, PAL: 0x0043f2e8
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

// NTSC-U/C: 0x00405958, PAL: 0x0043f248
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

// NTSC-U/C: 0x004057c8, PAL: 0x0043f0b8
HxStream &HxStream::ReadStr(char *pszDest, int nDestSize) {
    int nLength;
    ReadVarLen(nLength, *this);
    if (nLength < nDestSize) {
        ReadData(pszDest, nLength);
        pszDest[nLength] = '\0';
        return *this;
    }
    ReadData(pszDest, nDestSize - 1);
    pszDest[nDestSize - 1] = '\0';
    SetMarker(nLength - nDestSize + 1, kHxSeekCur);
    return *this;
}

// NTSC-U/C: 0x004058a8, PAL: 0x0043f198
HxStream &HxStream::ReadString(HxStr &str) {
    int nLength;
    ReadVarLen(nLength, *this);
    char *pszBuffer = new char[nLength + 1];
    ReadData(pszBuffer, nLength);
    pszBuffer[nLength] = '\0';
    str = pszBuffer;
    delete[] pszBuffer;
    return *this;
}

// NTSC-U/C: 0x00405a98, PAL: 0x0043f388
HxStream *HxStream::GetRootStream() {
    HxStream *pStream = this;
    while (HxStream *pInner = pStream->UnderlyingStream()) {
        pStream = pInner;
    }
    return pStream;
}

// NTSC-U/C: 0x00405ad8, PAL: 0x0043f3c8
HxStream &WriteVarLen(const int &nValue, HxStream &stream) {
    int nRemaining = nValue;
    int nPacked = nRemaining & kVarLenValueMask;
    int nBytes = 1;
    while ((nRemaining >>= kVarLenBits) != 0) {
        nPacked = (nPacked << kBitsPerByte) | kVarLenContinue | (nRemaining & kVarLenValueMask);
        ++nBytes;
    }
    for (; nBytes != 0; --nBytes) {
        const unsigned char byte = static_cast<unsigned char>(nPacked);
        stream.WriteNum(&byte, sizeof(byte));
        nPacked >>= kBitsPerByte;
    }
    return stream;
}

// NTSC-U/C: 0x00405b70, PAL: 0x0043f460
HxStream &ReadVarLen(int &nValue, HxStream &stream) {
    nValue = 0;
    unsigned char byte;
    do {
        stream.ReadNum(&byte, sizeof(byte));
        nValue = (nValue << kVarLenBits) + (byte & kVarLenValueMask);
    } while ((byte & kVarLenContinue) != 0);
    return stream;
}
