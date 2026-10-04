#include "rnd/memstream.h"

#include <string.h>
#include <vector>

#include "rnd/stream.h"

namespace Rnd {

MemStream::MemStream() : mEof(0), mFail(0), mPos(0) {
    mBuffer.reserve(kMemStreamReserve);
}

MemStream::~MemStream() {
}

Stream &MemStream::Read(void *pDest, int nSize) {
    const int nAvailable = mBuffer.size();
    if (static_cast<unsigned>(nAvailable) < static_cast<unsigned>(mPos + nSize)) {
        mEof = 1;
        mFail = 1;
        nSize = nAvailable - mPos;
    }

    memcpy(pDest, &mBuffer[mPos], nSize);
    mPos += nSize;
    return *this;
}

Stream &MemStream::Write(const void *pSrc, int nSize) {
    const unsigned nEnd = static_cast<unsigned>(mPos + nSize);
    if (mBuffer.capacity() < nEnd) {
        mBuffer.reserve(mBuffer.capacity() + kMemStreamReserve);
    }
    if (mBuffer.size() < nEnd) {
        mBuffer.resize(nEnd);
    }

    memcpy(mBuffer.data() + mPos, pSrc, nSize);
    mPos += nSize;
    return *this;
}

Stream &MemStream::Flush() {
    return *this;
}

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
    } else {
        mFail = 1;
    }
    return *this;
}

int MemStream::Tell() {
    return mPos;
}

int MemStream::Eof() {
    return mEof;
}

int MemStream::Fail() {
    return mFail;
}

void MemStream::Compact() {
    mBuffer.erase(mBuffer.begin(), mBuffer.begin() + mPos);
    mPos = 0;
}

} // namespace Rnd
