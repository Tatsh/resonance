#include "stream/obfilestream.h"

#include <stdio.h>

#include "os/hxstr.h"
#include "stream/obstream.h"

static const char kWriteMode[] = "wb";

OBFileStream::OBFileStream(const HxStr &path) {
    mFile = fopen(path.mStr != nullptr ? path.mStr : "", kWriteMode);
}

OBFileStream::~OBFileStream() {
    fclose(mFile); // Yes, the binary closes without testing for null.
}

OBStream &OBFileStream::Write(const void *pSrc, int nSize) {
    fwrite(pSrc, 1, nSize, mFile);
    return *this;
}

OBStream &OBFileStream::Reset() {
    return *this;
}

int OBFileStream::Fail() {
    return mFile == nullptr || ferror(mFile);
}

int OBFileStream::Tell() {
    return ftell(mFile);
}
