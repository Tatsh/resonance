#include "stream/ibfilestream.h"

#include <stdio.h>

#include "os/hxstr.h"
#include "stream/ibstream.h"

static const char kReadMode[] = "rb";

IBFileStream::IBFileStream(const HxStr &path) {
    mFile = fopen(path.mStr != nullptr ? path.mStr : "", kReadMode);
}

IBFileStream::~IBFileStream() {
    fclose(mFile); // Yes, the binary closes without testing for null.
}

IBStream &IBFileStream::Read(void *pDest, int nSize) {
    fread(pDest, 1, nSize, mFile);
    return *this;
}

IBStream &IBFileStream::Seek(int nOffset, int nWhence) {
    // The table is materialised on the stack at every call, so it was a local rather than a
    // file-scope constant.
    int anWhence[] = {SEEK_SET, SEEK_CUR, SEEK_END};
    fseek(mFile, nOffset, anWhence[nWhence]);
    return *this;
}

int IBFileStream::Tell() {
    return ftell(mFile);
}

int IBFileStream::Eof() {
    return feof(mFile);
}

int IBFileStream::Fail() {
    return mFile == nullptr || ferror(mFile);
}

IBStream &IBFileStream::Flush() {
    return *this;
}
