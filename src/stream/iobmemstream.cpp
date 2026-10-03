#include "stream/iobmemstream.h"

#include <string.h>
#include <vector>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// The buffer reserves this much up front and grows by the same step.
constexpr int kMemStreamGrowStep = 1024;

// NTSC-U/C: 0x004ecca8, PAL: 0x0052b850
IOBMemStream::IOBMemStream() {
    mBuffer.reserve(kMemStreamGrowStep);
    mFail = 0;
    mEof = 0;
    mPos = 0;
}

// NTSC-U/C: 0x004ece70, PAL: 0x0052ba18
IOBMemStream::IOBMemStream(const void *pData, int nSize) {
    mBuffer.reserve(kMemStreamGrowStep);
    mFail = 0;
    mEof = 0;
    mPos = 0;
    WriteBytes(pData, nSize);
}

// NTSC-U/C: 0x004ed978, PAL: 0x0052c520
IOBMemStream::~IOBMemStream() {
}

// NTSC-U/C: 0x004ee0e8, PAL: 0x0052cc90
IBStream &IOBMemStream::ReadBytes(void *pDest, int nSize) {
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

// NTSC-U/C: 0x004ee168, PAL: 0x0052cd10
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

// NTSC-U/C: 0x004eda38, PAL: 0x0052c5e0
int IOBMemStream::Tell() {
    return mPos;
}

// NTSC-U/C: 0x004eda40, PAL: 0x0052c5e8
int IOBMemStream::Eof() {
    return mEof;
}

// NTSC-U/C: 0x004eda48, PAL: 0x0052c5f0
int IOBMemStream::Fail() {
    return mFail;
}

// NTSC-U/C: 0x004eda50, PAL: 0x0052c5f8
IBStream &IOBMemStream::Flush() {
    return *this;
}

// NTSC-U/C: 0x004ee018, PAL: 0x0052cbc0
void IOBMemStream::Load(const void *pSrc, int nSize) {
    mBuffer.resize(mBuffer.size() + nSize);
    memcpy(&mBuffer[mPos], pSrc, nSize);
}

// NTSC-U/C: 0x004ee260, PAL: 0x0052ce08
void IOBMemStream::Resize(int nSize) {
    mBuffer.resize(nSize);
    mPos = 0;
}

// NTSC-U/C: 0x004eda58, PAL: 0x0052c600
char *IOBMemStream::Buffer() {
    return mBuffer.data();
}

// NTSC-U/C: 0x004ee1f0, PAL: 0x0052cd98
void IOBMemStream::DiscardReadBytes() {
    mBuffer.erase(mBuffer.begin(), mBuffer.begin() + mPos);
    mPos = 0;
}

// NTSC-U/C: 0x004ed068, PAL: 0x0052bc10
OBStream &IOBMemStream::WriteBytes(const void *pSrc, int nSize) {
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

// NTSC-U/C: 0x004eda30, PAL: 0x0052c5d8
OBStream &IOBMemStream::Reset() {
    return *this;
}
