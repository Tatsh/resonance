#include "stream/hxidatachunk.h"

#include "stream/hxchunkheader.h"
#include "stream/hxilistchunk.h"

HxIDataChunk::HxIDataChunk(HxIListChunk *pReader)
    : mReader(pReader), mSource(pReader->mStream), mId(nullptr) {
    mFatalOnEnd = 1;
    mSwapBytes = mSource->mSwapBytes;
    mId = new HxChunkHeader(*mReader->CurSubChunkHeader());
    mStart = mSource->GetMarker();
    mEnd = mStart + mId->mSize;
    mReader->Lock();
}

HxIDataChunk::HxIDataChunk(HxStream *pSource) : mReader(nullptr), mSource(pSource), mId(nullptr) {
    mFatalOnEnd = 1;
    mSwapBytes = pSource->mSwapBytes;
    mId = new HxChunkHeader;
    mId->Read(*pSource);
    mStart = mSource->GetMarker();
    mEnd = mStart + mId->mSize;
}

HxIDataChunk::~HxIDataChunk() {
    if (mReader != nullptr) {
        mReader->Unlock();
    }
    delete mId;
}

void HxIDataChunk::SetMarker(int nOffset, int nWhence) {
    if ((mStatus & failbit) != 0 || (mStatus & badbit) != 0) {
        return;
    }

    switch (nWhence) {
    case kHxSeekSet:
        if (mId->mSize < nOffset) {
            mStatus = failbit;
        }
        mSource->SetMarker(nOffset + mStart, kHxSeekSet);
        break;
    case kHxSeekCur:
        mSource->SetMarker(nOffset, kHxSeekCur);
        break;
    case kHxSeekEnd:
        if (nOffset < -mId->mSize) {
            mStatus = failbit;
        }
        mSource->SetMarker(nOffset + mEnd, kHxSeekSet);
        break;
    }
    mStatus = goodbit; // Yes, the binary overwrites the range status set above.
}

int HxIDataChunk::GetMarker() {
    if ((mStatus & failbit) != 0 || (mStatus & badbit) != 0) {
        return -1;
    }
    return mSource->GetMarker() - mStart;
}

int HxIDataChunk::Size() {
    return mId->mSize;
}

HxStream &HxIDataChunk::ReadData(void *pDest, int nSize) {
    if (mStatus != goodbit) {
        return *this;
    }

    int nLeft = mEnd - mSource->GetMarker();
    if (nLeft < nSize) {
        mSource->ReadData(pDest, nLeft);
        mStatus = kStatusEnd;
    } else {
        mSource->ReadData(pDest, nSize);
    }
    return *this;
}

HxStream *HxIDataChunk::UnderlyingStream() {
    return mSource;
}
