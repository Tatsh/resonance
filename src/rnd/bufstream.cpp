#include "rnd/bufstream.h"

#include <string.h>

#include "rnd/stream.h"

namespace Rnd {

// NTSC-U/C: 0x005104b8, PAL: 0x0054faa0
BufStream::BufStream(char *pBuffer, int nSize)
    : mBuffer(pBuffer), mFail(pBuffer == nullptr), mPos(0), mSize(nSize) {
}

// NTSC-U/C: 0x005104e0, PAL: 0x0054fac8
Stream &BufStream::ReadBytes(void *pDest, int nSize) {
    if (mSize < mPos + nSize) {
        mFail = 1;
        nSize = mSize - mPos;
    }

    memcpy(pDest, mBuffer + mPos, nSize);
    mPos += nSize;
    return *this;
}

// NTSC-U/C: 0x00510558, PAL: 0x0054fb40
Stream &BufStream::WriteBytes(const void *pSrc, int nSize) {
    if (mSize < mPos + nSize) {
        mFail = 1;
        nSize = mSize - mPos;
    }

    memcpy(mBuffer + mPos, pSrc, nSize);
    mPos += nSize;
    return *this;
}

// NTSC-U/C: 0x0050fe68, PAL: 0x0054f450
Stream &BufStream::Flush() {
    return *this;
}

// NTSC-U/C: 0x005105c8, PAL: 0x0054fbb0
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

// NTSC-U/C: 0x0050fe70, PAL: 0x0054f458
int BufStream::Tell() {
    return mPos;
}

// NTSC-U/C: 0x0050fe78, PAL: 0x0054f460
int BufStream::Eof() {
    return mPos == mSize;
}

// NTSC-U/C: 0x0050fe90, PAL: 0x0054f478
int BufStream::Fail() {
    return mFail;
}

} // namespace Rnd
