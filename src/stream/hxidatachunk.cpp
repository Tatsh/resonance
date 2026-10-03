#include "stream/hxidatachunk.h"

#include "stream/hxchunkheader.h"
#include "stream/hxilistchunk.h"

// NTSC-U/C: 0x00145908, PAL: 0x00146420
HxIDataChunk::HxIDataChunk(HxIListChunk *pReader)
    : mReader(pReader), mSource(pReader->mStream), mId(nullptr) {
    mFatalOnEnd = 1;
    mSwapBytes = mSource->mSwapBytes;
    mId = new HxChunkHeader(*mReader->CurSubChunkHeader());
    mStart = mSource->GetMarker();
    mEnd = mStart + mId->mSize;
    mReader->Lock();
}

// NTSC-U/C: 0x00145a10, PAL: 0x00146528
HxIDataChunk::HxIDataChunk(HxStream *pSource) : mReader(nullptr), mSource(pSource), mId(nullptr) {
    mFatalOnEnd = 1;
    mSwapBytes = pSource->mSwapBytes;
    mId = new HxChunkHeader;
    mId->Read(*pSource);
    mStart = mSource->GetMarker();
    mEnd = mStart + mId->mSize;
}

// NTSC-U/C: 0x00146080, PAL: 0x00146b98
HxIDataChunk::~HxIDataChunk() {
    if (mReader != nullptr) {
        mReader->Unlock();
    }
    delete mId;
}

// NTSC-U/C: 0x00145b20, PAL: 0x00146638
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

// NTSC-U/C: 0x001460f0, PAL: 0x00146c08
int HxIDataChunk::GetMarker() {
    if ((mStatus & failbit) != 0 || (mStatus & badbit) != 0) {
        return -1;
    }
    return mSource->GetMarker() - mStart;
}

// NTSC-U/C: 0x00145fc0, PAL: 0x00146ad8
int HxIDataChunk::Size() {
    return mId->mSize;
}

// NTSC-U/C: 0x00146160, PAL: 0x00146c78
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

// NTSC-U/C: 0x00145fd8, PAL: 0x00146af0
HxStream *HxIDataChunk::UnderlyingStream() {
    return mSource;
}
