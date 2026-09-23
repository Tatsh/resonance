#include "stream/hxstream.h"

#include "os/hxstr.h"

namespace {

constexpr int kVarLenBits = 7;
constexpr unsigned char kVarLenValueMask = 0x7f;
constexpr unsigned char kVarLenContinue = 0x80;
constexpr int kBitsPerByte = 8;

} // namespace

const int HxStream::kStatusOk = 0;
const int HxStream::kStatusEnd = 1;
const int HxStream::kStatusRange = 2;
const int HxStream::kStatusFailed = 4;

// 0x004057a8
HxStream::HxStream() {
    mSwapBytes = 0;
    mStatus = 0;
    mFatalOnEnd = 0;
}

// 0x00145ee8
HxStream::~HxStream() {
}

// 0x00145f18
void HxStream::Seek([[maybe_unused]] int nOffset, [[maybe_unused]] int nWhence) {
}

// 0x00145f20
int HxStream::Tell() {
    return 0;
}

// 0x00145f28
int HxStream::Size() {
    return 0;
}

// 0x00145f30
HxStream &HxStream::Write([[maybe_unused]] const void *pSrc, [[maybe_unused]] int nSize) {
    return *this;
}

// 0x00145f38
HxStream &HxStream::Read([[maybe_unused]] void *pDest, [[maybe_unused]] int nSize) {
    return *this;
}

// 0x00145f40
HxStream *HxStream::Unknown7() {
    return nullptr;
}

// 0x004059f8
HxStream &HxStream::ReadSwapped(void *pDest, int nSize) {
    if (mSwapBytes == 0 || nSize == 1) {
        return Read(pDest, nSize);
    }

    unsigned char *pBytes = static_cast<unsigned char *>(pDest);
    while (nSize != 0) {
        --nSize;
        Read(&pBytes[nSize], 1);
    }
    return *this;
}

// 0x00405958
HxStream &HxStream::WriteSwapped(const void *pSrc, int nSize) {
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

// 0x004057c8
HxStream &HxStream::ReadString(char *pszDest, int nDestSize) {
    int nLength;
    ReadVarLen(nLength, *this);
    if (nLength < nDestSize) {
        Read(pszDest, nLength);
        pszDest[nLength] = '\0';
        return *this;
    }
    Read(pszDest, nDestSize - 1);
    pszDest[nDestSize - 1] = '\0';
    Seek(nLength - nDestSize + 1, kHxSeekCur);
    return *this;
}

// 0x004058a8
HxStream &HxStream::ReadString(HxStr &str) {
    int nLength;
    ReadVarLen(nLength, *this);
    char *pszBuffer = new char[nLength + 1];
    Read(pszBuffer, nLength);
    pszBuffer[nLength] = '\0';
    str = pszBuffer;
    delete[] pszBuffer;
    return *this;
}

// 0x00405a98
HxStream *HxStream::BaseStream() {
    HxStream *pStream = this;
    while (HxStream *pInner = pStream->Unknown7()) {
        pStream = pInner;
    }
    return pStream;
}

// 0x00405ad8
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
        stream.WriteSwapped(&byte, sizeof(byte));
        nPacked >>= kBitsPerByte;
    }
    return stream;
}

// 0x00405b70
HxStream &ReadVarLen(int &nValue, HxStream &stream) {
    nValue = 0;
    unsigned char byte;
    do {
        stream.ReadSwapped(&byte, sizeof(byte));
        nValue = (nValue << kVarLenBits) + (byte & kVarLenValueMask);
    } while ((byte & kVarLenContinue) != 0);
    return stream;
}
