#include "stream/iobmemstream.h"

#include <string.h>
#include <vector>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// The buffer reserves this much up front and grows by the same step.
constexpr int kMemStreamGrowStep = 1024;

IOBMemStream::IOBMemStream() {
    mBuffer.reserve(kMemStreamGrowStep);
    mFail = 0;
    mEof = 0;
    mPos = 0;
}

IOBMemStream::IOBMemStream(const void *pData, int nSize) {
    mBuffer.reserve(kMemStreamGrowStep);
    mFail = 0;
    mEof = 0;
    mPos = 0;
    Write(pData, nSize);
}

IOBMemStream::~IOBMemStream() {
}

IBStream &IOBMemStream::Read(void *pDest, int nSize) {
    const int nAvailable = static_cast<int>(mBuffer.size());
    if (static_cast<unsigned>(nAvailable) < static_cast<unsigned>(mPos + nSize)) {
        nSize = nAvailable - mPos;
        mEof = 1;
        mFail = 1;
    }
    memcpy(pDest, &mBuffer[mPos], nSize);
    mPos += nSize;
    return *this;
}

IBStream &IOBMemStream::Seek(int nOffset, int nWhence) {
    switch (nWhence) {
    case kStreamSeekSet:
        break;
    case kStreamSeekCur:
        nOffset += mPos;
        break;
    case kStreamSeekEnd:
        nOffset += static_cast<int>(mBuffer.size());
        break;
    default:
        return *this;
    }
    if ((nOffset < 0) || (static_cast<int>(mBuffer.size()) < nOffset)) {
        mFail = 1; // A target outside the data fails rather than clamping.
    } else {
        mPos = nOffset;
    }
    return *this;
}

int IOBMemStream::Tell() {
    return mPos;
}

int IOBMemStream::Eof() {
    return mEof;
}

int IOBMemStream::Fail() {
    return mFail;
}

IBStream &IOBMemStream::Flush() {
    return *this;
}

void IOBMemStream::Fill(const void *pSrc, int nSize) {
    mBuffer.resize(mBuffer.size() + nSize);
    memcpy(&mBuffer[mPos], pSrc, nSize);
}

void IOBMemStream::Resize(int nSize) {
    mBuffer.resize(nSize);
    mPos = 0;
}

char *IOBMemStream::Buffer() {
    return mBuffer.data();
}

void IOBMemStream::Compact() {
    mBuffer.erase(mBuffer.begin(), mBuffer.begin() + mPos);
    mPos = 0;
}

OBStream &IOBMemStream::Write(const void *pSrc, int nSize) {
    if (mBuffer.capacity() < static_cast<unsigned int>(mPos + nSize)) {
        mBuffer.reserve(mBuffer.capacity() + kMemStreamGrowStep);
    }
    if (mBuffer.size() < static_cast<unsigned int>(mPos + nSize)) {
        mBuffer.resize(mPos + nSize);
    }
    memcpy(&mBuffer[mPos], pSrc, nSize);
    mPos += nSize;
    return *this;
}

OBStream &IOBMemStream::Reset() {
    return *this;
}
