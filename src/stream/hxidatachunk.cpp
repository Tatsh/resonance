#include "stream/hxidatachunk.h"

#include "stream/hxdatachunkid.h"
#include "stream/hxdatachunkreader.h"

// 0x00145908
HxIDataChunk::HxIDataChunk(HxDataChunkReader *pReader)
    : mReader(pReader), mSource(pReader->mStream), mId(nullptr) {
    mFatalOnEnd = 1;
    mSwapBytes = mSource->mSwapBytes;
    mId = new HxDataChunkId(*mReader->Current());
    mStart = mSource->Tell();
    mEnd = mStart + mId->mSize;
    mReader->Lock();
}

// 0x00145a10
HxIDataChunk::HxIDataChunk(HxStream *pSource) : mReader(nullptr), mSource(pSource), mId(nullptr) {
    mFatalOnEnd = 1;
    mSwapBytes = pSource->mSwapBytes;
    mId = new HxDataChunkId;
    mId->Read(*pSource);
    mStart = mSource->Tell();
    mEnd = mStart + mId->mSize;
}

// 0x00146080
HxIDataChunk::~HxIDataChunk() {
    if (mReader != nullptr) {
        mReader->Unlock();
    }
    delete mId;
}

// 0x00145b20
void HxIDataChunk::Seek(int nOffset, int nWhence) {
    if ((mStatus & kStatusRange) != 0 || (mStatus & kStatusFailed) != 0) {
        return;
    }

    switch (nWhence) {
    case kHxSeekSet:
        if (mId->mSize < nOffset) {
            mStatus = kStatusRange;
        }
        mSource->Seek(nOffset + mStart, kHxSeekSet);
        break;
    case kHxSeekCur:
        mSource->Seek(nOffset, kHxSeekCur);
        break;
    case kHxSeekEnd:
        if (nOffset < -mId->mSize) {
            mStatus = kStatusRange;
        }
        mSource->Seek(nOffset + mEnd, kHxSeekSet);
        break;
    }
    mStatus = kStatusOk; // Yes, the binary overwrites the range status set above.
}

// 0x001460f0
int HxIDataChunk::Tell() {
    if ((mStatus & kStatusRange) != 0 || (mStatus & kStatusFailed) != 0) {
        return -1;
    }
    return mSource->Tell() - mStart;
}

// 0x00145fc0
int HxIDataChunk::Size() {
    return mId->mSize;
}

// 0x00146160
HxStream &HxIDataChunk::Read(void *pDest, int nSize) {
    if (mStatus != kStatusOk) {
        return *this;
    }

    int nLeft = mEnd - mSource->Tell();
    if (nLeft < nSize) {
        mSource->Read(pDest, nLeft);
        mStatus = kStatusEnd;
    } else {
        mSource->Read(pDest, nSize);
    }
    return *this;
}

// 0x00145fd8
HxStream *HxIDataChunk::Unknown7() {
    return mSource;
}
