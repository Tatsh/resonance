#include "stream/hxstream.h"

// 0x004057a8
HxStream::HxStream() {
    mUnknown00 = 0;
    mStatus = 0;
    mFatalOnEnd = 0;
}

// 0x00145ee8
HxStream::~HxStream() {
}

// 0x00145f18
void HxStream::Seek([[maybe_unused]] int nOffset, [[maybe_unused]] int nWhence) {
}

// 0x00145f20
int HxStream::Tell() {
    return 0;
}

// 0x00145f28
int HxStream::Size() {
    return 0;
}

// 0x00145f30
HxStream &HxStream::Write([[maybe_unused]] const void *pSrc, [[maybe_unused]] int nSize) {
    return *this;
}

// 0x00145f38
HxStream &HxStream::Read([[maybe_unused]] void *pDest, [[maybe_unused]] int nSize) {
    return *this;
}

// 0x00145f40
int HxStream::Unknown7() {
    return 0;
}
