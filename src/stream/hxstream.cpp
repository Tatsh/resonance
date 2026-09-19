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
void HxStream::Seek(int nOffset, int nWhence) {
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
HxStream &HxStream::Write(const void *pSrc, int nSize) {
    return *this;
}

// 0x00145f38
HxStream &HxStream::Read(void *pDest, int nSize) {
    return *this;
}

// 0x00145f40
int HxStream::Unknown7() {
    return 0;
}
