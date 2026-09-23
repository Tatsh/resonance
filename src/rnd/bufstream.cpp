#include "rnd/bufstream.h"

#include <string.h>

#include "rnd/stream.h"

namespace Rnd {

// 0x005104b8
BufStream::BufStream(char *pBuffer, int nSize)
    : mBuffer(pBuffer), mFail(pBuffer == nullptr), mPos(0), mSize(nSize) {
}

// 0x005104e0
Stream &BufStream::ReadBytes(void *pDest, int nSize) {
    if (mSize < mPos + nSize) {
        mFail = 1;
        nSize = mSize - mPos;
    }

    memcpy(pDest, mBuffer + mPos, nSize);
    mPos += nSize;
    return *this;
}

// 0x00510558
Stream &BufStream::WriteBytes(const void *pSrc, int nSize) {
    if (mSize < mPos + nSize) {
        mFail = 1;
        nSize = mSize - mPos;
    }

    memcpy(mBuffer + mPos, pSrc, nSize);
    mPos += nSize;
    return *this;
}

// 0x0050fe68
Stream &BufStream::Flush() {
    return *this;
}

// 0x005105c8
Stream &BufStream::Seek(int nOffset, int nWhence) {
    switch (nWhence) {
    case kSeekSet:
        break;
    case kSeekCur:
        nOffset += mPos;
        break;
    case kSeekEnd:
        nOffset += mSize;
        break;
    default:
        return *this;
    }

    if (nOffset >= 0 && nOffset <= mSize) {
        mPos = nOffset;
    }
    return *this;
}

// 0x0050fe70
int BufStream::Tell() {
    return mPos;
}

// 0x0050fe78
int BufStream::Eof() {
    return mPos == mSize;
}

// 0x0050fe90
int BufStream::Fail() {
    return mFail;
}

} // namespace Rnd
