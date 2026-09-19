#include "stream/iobpreallocmemstream.h"

#include <string.h>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x004ee2f8
IOBPreallocMemStream::IOBPreallocMemStream(char *pBuffer, int nCapacity) {
    mBuffer = pBuffer;
    mCapacity = nCapacity;
    mWritePos = 0;
    mReadPos = 0;
    mEof = 0;
    mFail = 0;
}

// 0x004edb00
IOBPreallocMemStream::~IOBPreallocMemStream() {
}

// 0x004ee330
IBStream &IOBPreallocMemStream::ReadBytes(void *pDest, int nSize) {
    if (mWritePos < (mReadPos + nSize)) {
        nSize = mWritePos - mReadPos;
        mEof = 1;
        mFail = 1;
    }
    memcpy(pDest, &mBuffer[mReadPos], nSize);
    mReadPos += nSize;
    return *this;
}

// 0x004ee3a8
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

// 0x004edb48
int IOBPreallocMemStream::Tell() {
    return mReadPos;
}

// 0x004edb50
int IOBPreallocMemStream::Eof() {
    return mEof;
}

// 0x004edb58
int IOBPreallocMemStream::Fail() {
    return mFail;
}

// 0x004ee420
IBStream &IOBPreallocMemStream::Flush() {
    return *this;
}

// 0x004edb40
void IOBPreallocMemStream::SetSize(int nSize) {
    mWritePos = nSize;
}

// 0x004edb60
char *IOBPreallocMemStream::Buffer() {
    return mBuffer;
}

// 0x004edb68
int IOBPreallocMemStream::Size() {
    return mWritePos;
}

// 0x004edb70
int IOBPreallocMemStream::Capacity() {
    return mCapacity;
}

// 0x004ee428
OBStream &IOBPreallocMemStream::WriteBytes(const void *pSrc, int nSize) {
    if (mCapacity < (mWritePos + nSize)) {
        mFail = 1; // An overrun transfers nothing and does not set the end flag.
    } else {
        memcpy(&mBuffer[mWritePos], pSrc, nSize);
        mWritePos += nSize;
    }
    return *this;
}

// 0x004ee498
OBStream &IOBPreallocMemStream::Reset() {
    mWritePos = 0;
    mReadPos = 0;
    mEof = 0;
    mFail = 0;
    return *this;
}
