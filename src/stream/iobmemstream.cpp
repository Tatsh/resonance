#include "stream/iobmemstream.h"

#include <string.h>
#include <vector>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// The buffer reserves this much up front and grows by the same step.
constexpr int kMemStreamGrowStep = 1024;

// 0x004ecca8
IOBMemStream::IOBMemStream() {
    mBuffer.reserve(kMemStreamGrowStep);
    mFail = 0;
    mEof = 0;
    mPos = 0;
}

// 0x004ece70
IOBMemStream::IOBMemStream(const void *pData, int nSize) {
    mBuffer.reserve(kMemStreamGrowStep);
    mFail = 0;
    mEof = 0;
    mPos = 0;
    WriteBytes(pData, nSize);
}

// 0x004ed978
IOBMemStream::~IOBMemStream() {
}

// 0x004ee0e8
IBStream &IOBMemStream::ReadBytes(void *pDest, int nSize) {
    const int nAvailable = static_cast<int>(mBuffer.size());
    if (nAvailable < (mPos + nSize)) {
        nSize = nAvailable - mPos;
        mEof = 1;
        mFail = 1;
    }
    memcpy(pDest, &mBuffer[mPos], nSize);
    mPos += nSize;
    return *this;
}

// 0x004ee168
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

// 0x004eda38
int IOBMemStream::Tell() {
    return mPos;
}

// 0x004eda40
int IOBMemStream::Eof() {
    return mEof;
}

// 0x004eda48
int IOBMemStream::Fail() {
    return mFail;
}

// 0x004eda50
IBStream &IOBMemStream::Flush() {
    return *this;
}

// 0x004ee018
void IOBMemStream::Load(const void *pSrc, int nSize) {
    mBuffer.resize(mBuffer.size() + nSize);
    memcpy(&mBuffer[mPos], pSrc, nSize);
}

// 0x004ee260
void IOBMemStream::Resize(int nSize) {
    mBuffer.resize(nSize);
    mPos = 0;
}

// 0x004eda58
char *IOBMemStream::Buffer() {
    return mBuffer.data();
}

// 0x004ee1f0
void IOBMemStream::DiscardReadBytes() {
    mBuffer.erase(mBuffer.begin(), mBuffer.begin() + mPos);
    mPos = 0;
}

// 0x004ed068
OBStream &IOBMemStream::WriteBytes(const void *pSrc, int nSize) {
    if (static_cast<int>(mBuffer.capacity()) < (mPos + nSize)) {
        mBuffer.reserve(mBuffer.capacity() + kMemStreamGrowStep);
    }
    if (static_cast<int>(mBuffer.size()) < (mPos + nSize)) {
        mBuffer.resize(mPos + nSize);
    }
    memcpy(&mBuffer[mPos], pSrc, nSize);
    mPos += nSize;
    return *this;
}

// 0x004eda30
OBStream &IOBMemStream::Reset() {
    return *this;
}
