#include "stream/ibfilestream.h"

#include <stdio.h>

#include "os/hxstr.h"
#include "stream/ibstream.h"

static const char kReadMode[] = "rb";

// NTSC-U/C: 0x004edd38, PAL: 0x0052c8e0
IBFileStream::IBFileStream(const HxStr &path) {
    mFile = fopen(path.mStr != nullptr ? path.mStr : "", kReadMode);
}

// NTSC-U/C: 0x004edd88, PAL: 0x0052c930
IBFileStream::~IBFileStream() {
    fclose(mFile); // Yes, the binary closes without testing for null.
}

// NTSC-U/C: 0x004edde0, PAL: 0x0052c988
IBStream &IBFileStream::Read(void *pDest, int nSize) {
    fread(pDest, 1, nSize, mFile);
    return *this;
}

// NTSC-U/C: 0x004ede20, PAL: 0x0052c9c8
IBStream &IBFileStream::Seek(int nOffset, int nWhence) {
    // The table is materialised on the stack at every call, so it was a local rather than a
    // file-scope constant.
    int anWhence[] = {SEEK_SET, SEEK_CUR, SEEK_END};
    fseek(mFile, nOffset, anWhence[nWhence]);
    return *this;
}

// NTSC-U/C: 0x004ede78, PAL: 0x0052ca20
int IBFileStream::Tell() {
    return ftell(mFile);
}

// NTSC-U/C: 0x004edea0, PAL: 0x0052ca48
int IBFileStream::Eof() {
    return feof(mFile);
}

// NTSC-U/C: 0x004edeb8, PAL: 0x0052ca60
int IBFileStream::Fail() {
    return mFile == nullptr || ferror(mFile);
}

// NTSC-U/C: 0x004ede18, PAL: 0x0052c9c0
IBStream &IBFileStream::Flush() {
    return *this;
}
