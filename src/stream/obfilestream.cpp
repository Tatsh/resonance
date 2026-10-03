#include "stream/obfilestream.h"

#include <stdio.h>

#include "os/hxstr.h"
#include "stream/obstream.h"

static const char kWriteMode[] = "wb";

// NTSC-U/C: 0x004edee0, PAL: 0x0052ca88
OBFileStream::OBFileStream(const HxStr &path) {
    mFile = fopen(path.mStr != nullptr ? path.mStr : "", kWriteMode);
}

// NTSC-U/C: 0x004edf30, PAL: 0x0052cad8
OBFileStream::~OBFileStream() {
    fclose(mFile); // Yes, the binary closes without testing for null.
}

// NTSC-U/C: 0x004edf88, PAL: 0x0052cb30
OBStream &OBFileStream::WriteBytes(const void *pSrc, int nSize) {
    fwrite(pSrc, 1, nSize, mFile);
    return *this;
}

// NTSC-U/C: 0x004edfc0, PAL: 0x0052cb68
OBStream &OBFileStream::Reset() {
    return *this;
}

// NTSC-U/C: 0x004edff0, PAL: 0x0052cb98
int OBFileStream::Fail() {
    return mFile == nullptr || ferror(mFile);
}

// NTSC-U/C: 0x004edfc8, PAL: 0x0052cb70
int OBFileStream::Tell() {
    return ftell(mFile);
}
