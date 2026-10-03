#include "rnd/toolstream.h"

#include <string.h>

#include "os/dbg.h"
#include "os/log.h"
#include "os/mem.h"
#include "rnd/stream.h"

namespace Rnd {

namespace {

// The buffer the constructor allocates.
constexpr size_t kToolStreamBufferSize = 0x4000;

} // namespace

// NTSC-U/C: 0x00510238, PAL: 0x0054f820
ToolStream::ToolStream()
    : mCursor(0), mBuffer(new char[kToolStreamBufferSize]), mFill(0), mArrived(0), mConsumed(0) {
}

// NTSC-U/C: 0x00510480, PAL: 0x0054fa68
void ToolStream::Connect() {
    // Yes, the binary prints the four addresses through %u; they are 32 bits on the target.
    LogPrintf("ToolStream connect: %u %u %u %u\n", &mFill, &mArrived, &mConsumed, mBuffer);
}

// NTSC-U/C: 0x00510288, PAL: 0x0054f870
ToolStream::~ToolStream() {
    if (mBuffer != nullptr) {
        delete[] mBuffer;
    }
}

// NTSC-U/C: 0x00510308, PAL: 0x0054f8f0
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
    mCursor += nSize;
    return *this;
}

// NTSC-U/C: 0x00510400, PAL: 0x0054f9e8
Stream &ToolStream::WriteBytes([[maybe_unused]] const void *pSrc, [[maybe_unused]] int nSize) {
    g_failSink.Report("Can't write to a PS ToolStream\n");
    if (g_failSink.mAbortProc != nullptr) {
        g_failSink.mAbortProc();
    } else {
        throw; // With no handler the binary rethrows the exception in flight.
    }
    return *this;
}

// NTSC-U/C: 0x0050fde0, PAL: 0x0054f3c8
Stream &ToolStream::Seek([[maybe_unused]] int nOffset, [[maybe_unused]] int nWhence) {
    return *this;
}

// NTSC-U/C: 0x0050fde8, PAL: 0x0054f3d0
int ToolStream::Tell() {
    return 0;
}

// NTSC-U/C: 0x00510468, PAL: 0x0054fa50
Stream &ToolStream::Flush() {
    mCursor = 0;
    mConsumed = mArrived;
    return *this;
}

// NTSC-U/C: 0x005102f0, PAL: 0x0054f8d8
int ToolStream::Eof() {
    return mArrived == mConsumed;
}

// NTSC-U/C: 0x005102e8, PAL: 0x0054f8d0
int ToolStream::Fail() {
    return 0;
}

} // namespace Rnd
