#include "rnd/toolstream.h"

#include <string.h>

#include "os/failsink.h"
#include "os/log.h"
#include "os/mem.h"
#include "rnd/stream.h"

namespace Rnd {

namespace {

// The buffer the constructor allocates.
constexpr size_t kToolStreamBufferSize = 0x4000;

} // namespace

// 0x00510238
ToolStream::ToolStream()
    : mCursor(0), mBuffer(new char[kToolStreamBufferSize]), mFill(0), mArrived(0), mConsumed(0) {
}

// 0x00510480
void ToolStream::Connect() {
    // Yes, the binary prints the four addresses through %u; they are 32 bits on the target.
    LogPrintf("ToolStream connect: %u %u %u %u\n", &mFill, &mArrived, &mConsumed, mBuffer);
}

// 0x00510288
ToolStream::~ToolStream() {
    if (mBuffer != nullptr) {
        delete[] mBuffer;
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
Stream &ToolStream::WriteBytes([[maybe_unused]] const void *pSrc, [[maybe_unused]] int nSize) {
    g_failSink.Report("Can't write to a PS ToolStream\n");
    if (g_failSink.mAbortProc != nullptr) {
        g_failSink.mAbortProc();
    } else {
        throw; // With no handler the binary rethrows the exception in flight.
    }
    return *this;
}

// 0x0050fde0
Stream &ToolStream::Seek([[maybe_unused]] int nOffset, [[maybe_unused]] int nWhence) {
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
