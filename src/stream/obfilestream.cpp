#include "stream/obfilestream.h"

#include <stdio.h>

#include "os/hxstr.h"
#include "stream/obstream.h"

static const char kWriteMode[] = "wb";

// 0x004edee0
OBFileStream::OBFileStream(const HxStr &path) {
    mFile = fopen(path.mStr != nullptr ? path.mStr : "", kWriteMode);
}

// 0x004edf30
OBFileStream::~OBFileStream() {
    fclose(mFile); // Yes, the binary closes without testing for null.
}

// 0x004edf88
OBStream &OBFileStream::WriteBytes(const void *pSrc, int nSize) {
    fwrite(pSrc, 1, nSize, mFile);
    return *this;
}

// 0x004edfc0
OBStream &OBFileStream::Reset() {
    return *this;
}

// 0x004edff0
int OBFileStream::Fail() {
    return mFile == nullptr || ferror(mFile);
}

// 0x004edfc8
int OBFileStream::Tell() {
    return ftell(mFile);
}
