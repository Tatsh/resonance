#include "rnd/bufstream.h"

#include <string.h>

#include "rnd/stream.h"

namespace Rnd {

BufStream::BufStream(char *pBuffer, int nSize)
    : mBuffer(pBuffer), mFail(pBuffer == nullptr), mPos(0), mSize(nSize) {
}

Stream &BufStream::Read(void *pDest, int nSize) {
    if (mSize < mPos + nSize) {
        mFail = 1;
        nSize = mSize - mPos;
    }

    memcpy(pDest, mBuffer + mPos, nSize);
    mPos += nSize;
    return *this;
}

Stream &BufStream::Write(const void *pSrc, int nSize) {
    if (mSize < mPos + nSize) {
        mFail = 1;
        nSize = mSize - mPos;
    }

    memcpy(mBuffer + mPos, pSrc, nSize);
    mPos += nSize;
    return *this;
}

Stream &BufStream::Flush() {
    return *this;
}

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
    } else {
        mFail = 1;
    }
    return *this;
}

int BufStream::Tell() {
    return mPos;
}

int BufStream::Eof() {
    return mPos == mSize;
}

int BufStream::Fail() {
    return mFail;
}

} // namespace Rnd
