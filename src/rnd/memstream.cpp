#include "rnd/memstream.h"

#include <string.h>
#include <vector>

#include "rnd/stream.h"

namespace Rnd {

// 0x0050f288
MemStream::MemStream() : mEof(0), mFail(0), mPos(0) {
    mBuffer.reserve(kMemStreamReserve);
}

// 0x0050fca0
MemStream::~MemStream() {
}

// 0x005100c0
Stream &MemStream::ReadBytes(void *pDest, int nSize) {
    const int nAvailable = mBuffer.size();
    if (nAvailable < mPos + nSize) {
        mEof = 1;
        mFail = 1;
        nSize = nAvailable - mPos;
    }

    memcpy(pDest, &mBuffer[mPos], nSize);
    mPos += nSize;
    return *this;
}

// 0x0050f448
Stream &MemStream::WriteBytes(const void *pSrc, int nSize) {
    if (static_cast<int>(mBuffer.capacity()) < mPos + nSize) {
        mBuffer.reserve(mBuffer.capacity() + kMemStreamReserve);
    }

    memcpy(&mBuffer[mPos], pSrc, nSize);
    mPos += nSize;
    return *this;
}

// 0x0050fd48
Stream &MemStream::Flush() {
    return *this;
}

// 0x00510140
Stream &MemStream::Seek(int nOffset, int nWhence) {
    switch (nWhence) {
    case kSeekSet:
        break;
    case kSeekCur:
        nOffset += mPos;
        break;
    case kSeekEnd:
        nOffset += mBuffer.size();
        break;
    default:
        return *this;
    }

    if (nOffset >= 0 && nOffset <= static_cast<int>(mBuffer.size())) {
        mPos = nOffset;
    }
    return *this;
}

// 0x0050fd50
int MemStream::Tell() {
    return mPos;
}

// 0x0050fd58
int MemStream::Eof() {
    return mEof;
}

// 0x0050fd60
int MemStream::Fail() {
    return mFail;
}

// 0x005101c8
void MemStream::DiscardReadBytes() {
    mBuffer.erase(mBuffer.begin(), mBuffer.begin() + mPos);
    mPos = 0;
}

} // namespace Rnd
