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

// 0x0050fe98
FileStream::FileStream(const HxStr &path, int nWrite) {
    mFile = fopen(path.mStr != nullptr ? path.mStr : "", nWrite != 0 ? kWriteMode : kReadMode);
}

// 0x0050ff00
FileStream::~FileStream() {
    if (mFile != nullptr) {
        fclose(mFile);
    }
}

// 0x0050ff60
Stream &FileStream::ReadBytes(void *pDest, int nSize) {
    fread(pDest, nSize, 1, mFile);
    return *this;
}

// 0x0050ff98
Stream &FileStream::WriteBytes(const void *pSrc, int nSize) {
    fwrite(pSrc, nSize, 1, mFile);
    return *this;
}

// 0x0050ffd0
Stream &FileStream::Flush() {
    fflush(mFile);
    return *this;
}

// 0x00510000
Stream &FileStream::Seek(int nOffset, int nWhence) {
    fseek(mFile, nOffset, kWhence[nWhence]);
    return *this;
}

// 0x00510058
int FileStream::Tell() {
    return ftell(mFile);
}

// 0x00510080
int FileStream::Eof() {
    return feof(mFile);
}

// 0x00510098
int FileStream::Fail() {
    return mFile == nullptr || ferror(mFile);
}

} // namespace Rnd
