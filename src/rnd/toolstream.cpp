#include "rnd/toolstream.h"

#include <string.h>

#include "os/failsink.h"
#include "rnd/stream.h"

namespace Rnd {

// 0x00510288
ToolStream::~ToolStream() {
    if (mBuffer != nullptr) {
        MemFree(mBuffer);
    }
}

// 0x00510308
Stream &ToolStream::ReadBytes(void *pDest, int nSize) {
    char *pCursor = static_cast<char *>(pDest);
    while (Eof()) {
    }

    while (mFill < mCursor + nSize) {
        const int nChunk = mFill - mCursor;
        memcpy(pCursor, mBuffer + mCursor, nChunk);
        pCursor += nChunk;
        nSize -= nChunk;

        Flush();
        while (Eof()) {
        }
    }

    memcpy(pCursor, mBuffer + mCursor, nSize);
    return *this;
}

// 0x00510400
Stream &ToolStream::WriteBytes(const void *pSrc, int nSize) {
    g_failSink.Report("Can't write to a PS ToolStream\n");
    if (g_failSink.mAbortProc != nullptr) {
        g_failSink.mAbortProc();
    }
    return *this;
}

// 0x0050fde0
Stream &ToolStream::Seek(int nOffset, int nWhence) {
    return *this;
}

// 0x0050fde8
int ToolStream::Tell() {
    return 0;
}

// 0x00510468
Stream &ToolStream::Flush() {
    mCursor = 0;
    mConsumed = mArrived;
    return *this;
}

// 0x005102f0
int ToolStream::Eof() {
    return mArrived == mConsumed;
}

// 0x005102e8
int ToolStream::Fail() {
    return 0;
}

} // namespace Rnd
