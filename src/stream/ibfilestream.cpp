#include "stream/ibfilestream.h"

#include <stdio.h>

#include "os/hxstr.h"
#include "stream/ibstream.h"

static const char kReadMode[] = "rb";

// 0x004edd38
IBFileStream::IBFileStream(const HxStr &path) {
    mFile = fopen(path.mStr != nullptr ? path.mStr : "", kReadMode);
}

// 0x004edd88
IBFileStream::~IBFileStream() {
    fclose(mFile); // Yes, the binary closes without testing for null.
}

// 0x004edde0
IBStream &IBFileStream::ReadBytes(void *pDest, int nSize) {
    fread(pDest, 1, nSize, mFile);
    return *this;
}

// 0x004ede20
IBStream &IBFileStream::Seek(int nOffset, int nWhence) {
    // The table is materialised on the stack at every call, so it was a local rather than a
    // file-scope constant.
    int anWhence[] = {SEEK_SET, SEEK_CUR, SEEK_END};
    fseek(mFile, nOffset, anWhence[nWhence]);
    return *this;
}

// 0x004ede78
int IBFileStream::Tell() {
    return ftell(mFile);
}

// 0x004edea0
int IBFileStream::Eof() {
    return feof(mFile);
}

// 0x004edeb8
int IBFileStream::Fail() {
    return mFile == nullptr || ferror(mFile);
}

// 0x004ede18
IBStream &IBFileStream::Flush() {
    return *this;
}
