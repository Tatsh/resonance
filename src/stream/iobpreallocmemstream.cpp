#include "stream/iobpreallocmemstream.h"

#include <string.h>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x004ee2f8, PAL: 0x0052cea0
IOBPreallocMemStream::IOBPreallocMemStream(char *pBuffer, int nCapacity) {
    mBuffer = pBuffer;
    mCapacity = nCapacity;
    mWritePos = 0;
    mReadPos = 0;
    mEof = 0;
    mFail = 0;
}

// NTSC-U/C: 0x004edb00, PAL: 0x0052c6a8
IOBPreallocMemStream::~IOBPreallocMemStream() {
}

// NTSC-U/C: 0x004ee330, PAL: 0x0052ced8
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

// NTSC-U/C: 0x004ee3a8, PAL: 0x0052cf50
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

// NTSC-U/C: 0x004edb48, PAL: 0x0052c6f0
int IOBPreallocMemStream::Tell() {
    return mReadPos;
}

// NTSC-U/C: 0x004edb50, PAL: 0x0052c6f8
int IOBPreallocMemStream::Eof() {
    return mEof;
}

// NTSC-U/C: 0x004edb58, PAL: 0x0052c700
int IOBPreallocMemStream::Fail() {
    return mFail;
}

// NTSC-U/C: 0x004ee420, PAL: 0x0052cfc8
IBStream &IOBPreallocMemStream::Flush() {
    return *this;
}

// NTSC-U/C: 0x004edb40, PAL: 0x0052c6e8
void IOBPreallocMemStream::SetSize(int nSize) {
    mWritePos = nSize;
}

// NTSC-U/C: 0x004edb60, PAL: 0x0052c708
char *IOBPreallocMemStream::Buffer() {
    return mBuffer;
}

// NTSC-U/C: 0x004edb70, PAL: 0x0052c718
int IOBPreallocMemStream::Capacity() {
    return mCapacity;
}

// NTSC-U/C: 0x004ee428, PAL: 0x0052cfd0
OBStream &IOBPreallocMemStream::WriteBytes(const void *pSrc, int nSize) {
    if (mCapacity < (mWritePos + nSize)) {
        mFail = 1; // An overrun transfers nothing and does not set the end flag.
    } else {
        memcpy(&mBuffer[mWritePos], pSrc, nSize);
        mWritePos += nSize;
    }
    return *this;
}

// NTSC-U/C: 0x004ee498, PAL: 0x0052d040
OBStream &IOBPreallocMemStream::Reset() {
    mWritePos = 0;
    mReadPos = 0;
    mEof = 0;
    mFail = 0;
    return *this;
}
