#include "rnd/memstream.h"

#include <string.h>
#include <vector>

#include "rnd/stream.h"

namespace Rnd {

// NTSC-U/C: 0x0050f288, PAL: 0x0054e870
MemStream::MemStream() : mEof(0), mFail(0), mPos(0) {
    mBuffer.reserve(kMemStreamReserve);
}

// NTSC-U/C: 0x0050fca0, PAL: 0x0054f288
MemStream::~MemStream() {
}

// NTSC-U/C: 0x005100c0, PAL: 0x0054f6a8
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

// NTSC-U/C: 0x0050f448, PAL: 0x0054ea30
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

// NTSC-U/C: 0x0050fd48, PAL: 0x0054f330
Stream &MemStream::Flush() {
    return *this;
}

// NTSC-U/C: 0x00510140, PAL: 0x0054f728
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

// NTSC-U/C: 0x0050fd50, PAL: 0x0054f338
int MemStream::Tell() {
    return mPos;
}

// NTSC-U/C: 0x0050fd58, PAL: 0x0054f340
int MemStream::Eof() {
    return mEof;
}

// NTSC-U/C: 0x0050fd60, PAL: 0x0054f348
int MemStream::Fail() {
    return mFail;
}

// NTSC-U/C: 0x005101c8, PAL: 0x0054f7b0
void MemStream::Compact() {
    mBuffer.erase(mBuffer.begin(), mBuffer.begin() + mPos);
    mPos = 0;
}

} // namespace Rnd
