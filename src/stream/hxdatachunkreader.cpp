#include "stream/hxdatachunkreader.h"

#include "stream/hxchunkname.h"
#include "stream/hxstream.h"

namespace {

// Bytes of a plain chunk header, and of a list header with its form type.
constexpr int kChunkHeaderSize = 8;
constexpr int kListHeaderSize = 12;

} // namespace

// 0x00145c40
HxDataChunkReader::HxDataChunkReader(HxStream *pStream, bool bReadHeader)
    : mParent(nullptr), mStream(pStream), mHeader(nullptr), mLocked(0), mAtStart(1) {
    if (bReadHeader) {
        mHeader = new HxDataChunkId;
        mHeader->Read(*mStream);
    } else {
        int nSize = pStream->Size() - pStream->Tell();
        mHeader = new HxDataChunkId(g_listChunkName, nSize, 1);
    }
    mStart = mStream->Tell();
    Init();
}

// 0x00146220
HxDataChunkReader::HxDataChunkReader(HxDataChunkReader *pParent)
    : mParent(pParent), mStream(pParent->mStream), mHeader(nullptr), mLocked(0), mAtStart(1) {
    mHeader = new HxDataChunkId(*mParent->Current());
    mStart = mStream->Tell();
    Init();
}

// 0x00146308
HxDataChunkReader::~HxDataChunkReader() {
    if (mParent != nullptr) {
        mParent->Unlock();
    }
    delete mHeader;
}

// 0x00146360
void HxDataChunkReader::Init() {
    mEnd = mStart + mHeader->mSize;
    if (mParent != nullptr) {
        mParent->Lock();
    }
    Rewind();
}

// 0x001463b0
void HxDataChunkReader::Rewind() {
    mStream->Seek(mStart, kHxSeekSet);
    mAtStart = 1;
    mNext = mStart;
    mHasCurrent = 0;
}

// 0x00145db8
HxDataChunkId *HxDataChunkReader::Next() {
    mAtStart = 0;
    if (mNext >= mEnd) {
        mHasCurrent = 0;
        return nullptr;
    }

    mHasCurrent = 1;
    mStream->Seek(mNext, kHxSeekSet);
    *mStream >> mCurrent;
    int nSize = mCurrent.mSize + (mCurrent.mIsList != 0 ? kListHeaderSize : kChunkHeaderSize);
    if (mCurrent.Name() != g_mtrkChunkName) {
        nSize = (nSize - nSize / 2) * 2;
    }
    mNext += nSize;
    return &mCurrent;
}

// 0x00146408
HxDataChunkId *HxDataChunkReader::Current() {
    return mHasCurrent != 0 ? &mCurrent : nullptr;
}

// 0x00146428
HxDataChunkId *HxDataChunkReader::Find(const HxChunkName &name) {
    while (Next() != nullptr) {
        if (name == mCurrent.Name()) {
            return &mCurrent;
        }
    }
    return nullptr;
}

// 0x001464a0
void HxDataChunkReader::Lock() {
    mLocked = 1;
}

// 0x001464b0
void HxDataChunkReader::Unlock() {
    mLocked = 0;
}
