#include "stream/hxilistchunk.h"

#include "stream/hxchunkname.h"
#include "stream/hxstream.h"

namespace {

// Bytes of a plain chunk header, and of a list header with its form type.
constexpr int kChunkHeaderSize = 8;
constexpr int kListHeaderSize = 12;

} // namespace

// NTSC-U/C: 0x00145c40, PAL: 0x00146758
HxIListChunk::HxIListChunk(HxStream *pStream, bool bReadHeader)
    : mParent(nullptr), mStream(pStream), mHeader(nullptr), mLocked(0), mAtStart(1) {
    if (bReadHeader) {
        mHeader = new HxChunkHeader;
        mHeader->Read(*mStream);
    } else {
        int nSize = pStream->Size() - pStream->GetMarker();
        mHeader = new HxChunkHeader(kListChunkID, nSize, 1);
    }
    mStart = mStream->GetMarker();
    Init();
}

// NTSC-U/C: 0x00146220, PAL: 0x00146d38
HxIListChunk::HxIListChunk(HxIListChunk *pParent)
    : mParent(pParent), mStream(pParent->mStream), mHeader(nullptr), mLocked(0), mAtStart(1) {
    mHeader = new HxChunkHeader(*mParent->CurSubChunkHeader());
    mStart = mStream->GetMarker();
    Init();
}

// NTSC-U/C: 0x00146308, PAL: 0x00146e20
HxIListChunk::~HxIListChunk() {
    if (mParent != nullptr) {
        mParent->Unlock();
    }
    delete mHeader;
}

// NTSC-U/C: 0x00146360, PAL: 0x00146e78
void HxIListChunk::Init() {
    mEnd = mStart + mHeader->mSize;
    if (mParent != nullptr) {
        mParent->Lock();
    }
    Reset();
}

// NTSC-U/C: 0x001463b0, PAL: 0x00146ec8
void HxIListChunk::Reset() {
    mStream->SetMarker(mStart, kHxSeekSet);
    mAtStart = 1;
    mNext = mStart;
    mHasCurrent = 0;
}

// NTSC-U/C: 0x00145db8, PAL: 0x001468d0
HxChunkHeader *HxIListChunk::Next() {
    mAtStart = 0;
    if (mNext >= mEnd) {
        mHasCurrent = 0;
        return nullptr;
    }

    mHasCurrent = 1;
    mStream->SetMarker(mNext, kHxSeekSet);
    *mStream >> mCurrent;
    int nSize = mCurrent.mSize + (mCurrent.mIsList != 0 ? kListHeaderSize : kChunkHeaderSize);
    if (mCurrent.Name() != kMidiTrackChunkID) {
        nSize = (nSize - nSize / 2) * 2;
    }
    mNext += nSize;
    return &mCurrent;
}

// NTSC-U/C: 0x00146408, PAL: 0x00146f20
HxChunkHeader *HxIListChunk::CurSubChunkHeader() {
    return mHasCurrent != 0 ? &mCurrent : nullptr;
}

// NTSC-U/C: 0x00146428, PAL: 0x00146f40
HxChunkHeader *HxIListChunk::Next(const HxChunkName &name) {
    while (Next() != nullptr) {
        if (name == mCurrent.Name()) {
            return &mCurrent;
        }
    }
    return nullptr;
}

// NTSC-U/C: 0x001464a0, PAL: 0x00146fb8
void HxIListChunk::Lock() {
    mLocked = 1;
}

// NTSC-U/C: 0x001464b0, PAL: 0x00146fc8
void HxIListChunk::Unlock() {
    mLocked = 0;
}
