#include "stream/hxmemstream.h"

#include <string.h>

#include "os/hxstr.h"
#include "os/log.h"
#include "stream/hxstream.h"

static const char kNoWriteMessage[] =
    "HxMemStream: does not support write operations at this time\n";

// The misspelling is the original's.
static const char kEndOfBufferMessage[] = "End Of buffset %s reached";

// 0x00405cf8
HxMemStream::HxMemStream(const HxStr &name, char *pData, int nSize) : mName(name) {
    mEnd = &pData[nSize];
    mCur = pData;
    mStatus = 0;
    mStart = pData;
}

// 0x00405d98
HxMemStream::~HxMemStream() {
}

// 0x00405e00
void HxMemStream::Seek(int nOffset, int nWhence) {
    char *pTarget = nullptr;
    switch (nWhence) {
    case kHxSeekCur:
        pTarget = &mCur[nOffset];
        break;
    case kHxSeekEnd:
        pTarget = &mEnd[nOffset];
        break;
    default:
        pTarget = &mStart[nOffset];
        break;
    }
    mCur = pTarget;
    mStatus = 0; // Yes, the binary clears this before either clamp is tested.
    if (mCur < mStart) {
        mCur = mStart;
        return;
    }
    if (mEnd < mCur) {
        mCur = mEnd;
        mStatus = 1;
    }
}

// 0x00405e80
int HxMemStream::Tell() {
    return mCur - mStart;
}

// 0x00405e90
int HxMemStream::Size() {
    return mEnd - mStart;
}

// 0x00405ea0
HxStream &HxMemStream::Write([[maybe_unused]] const void *pSrc, [[maybe_unused]] int nSize) {
    Fatal(kNoWriteMessage);
    return *this;
}

// 0x00405ed0
HxStream &HxMemStream::Read(void *pDest, int nSize) {
    if (nSize > 0) {
        int nCount = mEnd - mCur;
        if (nCount >= nSize) {
            nCount = nSize;
        }
        memcpy(pDest, mCur, nCount);
        mCur += nCount;
    }
    if (mCur < mEnd) {
        return *this;
    }
    mStatus = 1; // Yes, the binary records this before testing the flag.
    if (mFatalOnEnd != 0) {
        Fatal(kEndOfBufferMessage, mName.mStr != nullptr ? mName.mStr : "");
    }
    return *this;
}
