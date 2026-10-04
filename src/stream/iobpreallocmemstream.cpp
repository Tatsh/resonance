#include "stream/iobpreallocmemstream.h"

#include <string.h>

#include "stream/ibstream.h"
#include "stream/obstream.h"

IOBPreallocMemStream::IOBPreallocMemStream(char *pBuffer, int nCapacity)
    : mBuffer(pBuffer), mCapacity(nCapacity), mWritePos(0), mReadPos(0), mEof(0), mFail(0) {
}

IOBPreallocMemStream::~IOBPreallocMemStream() {
}

IBStream &IOBPreallocMemStream::Read(void *pDest, int nSize) {
    if (mWritePos < (mReadPos + nSize)) {
        nSize = mWritePos - mReadPos;
        mEof = 1;
        mFail = 1;
    }
    memcpy(pDest, &mBuffer[mReadPos], nSize);
    mReadPos += nSize;
    return *this;
}

IBStream &IOBPreallocMemStream::Seek(int nOffset, int nWhence) {
    switch (nWhence) {
    case kStreamSeekSet:
        break;
    case kStreamSeekCur:
        nOffset += mReadPos;
        break;
    case kStreamSeekEnd:
        nOffset += mWritePos;
        break;
    default:
        return *this;
    }
    if ((nOffset < 0) || (mWritePos < nOffset)) {
        mFail = 1; // A target outside the data fails rather than clamping.
    } else {
        mReadPos = nOffset;
    }
    return *this;
}

int IOBPreallocMemStream::Tell() {
    return mReadPos;
}

int IOBPreallocMemStream::Eof() {
    return mEof;
}

int IOBPreallocMemStream::Fail() {
    return mFail;
}

IBStream &IOBPreallocMemStream::Flush() {
    return *this;
}

void IOBPreallocMemStream::SetSize(int nSize) {
    mWritePos = nSize;
}

char *IOBPreallocMemStream::Buffer() {
    return mBuffer;
}

int IOBPreallocMemStream::Capacity() {
    return mCapacity;
}

OBStream &IOBPreallocMemStream::Write(const void *pSrc, int nSize) {
    if (mCapacity < (mWritePos + nSize)) {
        mFail = 1; // An overrun transfers nothing and does not set the end flag.
    } else {
        memcpy(&mBuffer[mWritePos], pSrc, nSize);
        mWritePos += nSize;
    }
    return *this;
}

OBStream &IOBPreallocMemStream::Reset() {
    mWritePos = 0;
    mReadPos = 0;
    mEof = 0;
    mFail = 0;
    return *this;
}
