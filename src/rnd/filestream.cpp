#include "rnd/filestream.h"

#include <stdio.h>

#include "os/hxstr.h"
#include "rnd/stream.h"

namespace Rnd {

static const char kReadMode[] = "rb";
static const char kWriteMode[] = "wb";

// Seek() indexes this rather than passing its argument through, because the SeekOrigin values are
// the renderer's own rather than the C library's.
static const int kWhence[] = {SEEK_SET, SEEK_CUR, SEEK_END};

FileStream::FileStream(const HxStr &path, int nWrite) {
    mFile = fopen(path.mStr != nullptr ? path.mStr : "", nWrite != 0 ? kWriteMode : kReadMode);
}

FileStream::~FileStream() {
    if (mFile != nullptr) {
        fclose(mFile);
    }
}

Stream &FileStream::Read(void *pDest, int nSize) {
    fread(pDest, nSize, 1, mFile);
    return *this;
}

Stream &FileStream::Write(const void *pSrc, int nSize) {
    fwrite(pSrc, nSize, 1, mFile);
    return *this;
}

Stream &FileStream::Flush() {
    fflush(mFile);
    return *this;
}

Stream &FileStream::Seek(int nOffset, int nWhence) {
    fseek(mFile, nOffset, kWhence[nWhence]);
    return *this;
}

int FileStream::Tell() {
    return ftell(mFile);
}

int FileStream::Eof() {
    return feof(mFile);
}

int FileStream::Fail() {
    return mFile == nullptr || ferror(mFile);
}

} // namespace Rnd
