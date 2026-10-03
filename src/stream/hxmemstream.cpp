#include "stream/hxmemstream.h"

#include <string.h>

#include "os/hxstr.h"
#include "os/log.h"
#include "stream/hxstream.h"

static const char kNoWriteMessage[] =
    "HxMemStream: does not support write operations at this time\n";

// The misspelling is the original's.
static const char kEndOfBufferMessage[] = "End Of buffset %s reached";

// NTSC-U/C: 0x00405cf8, PAL: 0x0043f5e8
HxMemStream::HxMemStream(const char *pszName, char *pData, int nSize) : mName(pszName) {
    mEnd = &pData[nSize];
    mCur = pData;
    mStatus = 0;
    mStart = pData;
}

// NTSC-U/C: 0x00405d98, PAL: 0x0043f688
HxMemStream::~HxMemStream() {
}

// NTSC-U/C: 0x00405e00, PAL: 0x0043f700
void HxMemStream::SetMarker(int nOffset, int nWhence) {
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

// NTSC-U/C: 0x00405e80, PAL: 0x0043f780
int HxMemStream::GetMarker() {
    return mCur - mStart;
}

// NTSC-U/C: 0x00405e90, PAL: 0x0043f790
int HxMemStream::Size() {
    return mEnd - mStart;
}

// NTSC-U/C: 0x00405ea0, PAL: 0x0043f7a0
HxStream &HxMemStream::Write([[maybe_unused]] const void *pSrc, [[maybe_unused]] int nSize) {
    Fatal(kNoWriteMessage);
    return *this;
}

// NTSC-U/C: 0x00405ed0, PAL: 0x0043f7d0
HxStream &HxMemStream::ReadData(void *pDest, int nSize) {
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
