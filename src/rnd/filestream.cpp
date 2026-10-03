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

// NTSC-U/C: 0x0050fe98, PAL: 0x0054f480
FileStream::FileStream(const HxStr &path, int nWrite) {
    mFile = fopen(path.mStr != nullptr ? path.mStr : "", nWrite != 0 ? kWriteMode : kReadMode);
}

// NTSC-U/C: 0x0050ff00, PAL: 0x0054f4e8
FileStream::~FileStream() {
    if (mFile != nullptr) {
        fclose(mFile);
    }
}

// NTSC-U/C: 0x0050ff60, PAL: 0x0054f548
Stream &FileStream::Read(void *pDest, int nSize) {
    fread(pDest, nSize, 1, mFile);
    return *this;
}

// NTSC-U/C: 0x0050ff98, PAL: 0x0054f580
Stream &FileStream::Write(const void *pSrc, int nSize) {
    fwrite(pSrc, nSize, 1, mFile);
    return *this;
}

// NTSC-U/C: 0x0050ffd0, PAL: 0x0054f5b8
Stream &FileStream::Flush() {
    fflush(mFile);
    return *this;
}

// NTSC-U/C: 0x00510000, PAL: 0x0054f5e8
Stream &FileStream::Seek(int nOffset, int nWhence) {
    fseek(mFile, nOffset, kWhence[nWhence]);
    return *this;
}

// NTSC-U/C: 0x00510058, PAL: 0x0054f640
int FileStream::Tell() {
    return ftell(mFile);
}

// NTSC-U/C: 0x00510080, PAL: 0x0054f668
int FileStream::Eof() {
    return feof(mFile);
}

// NTSC-U/C: 0x00510098, PAL: 0x0054f680
int FileStream::Fail() {
    return mFile == nullptr || ferror(mFile);
}

} // namespace Rnd
