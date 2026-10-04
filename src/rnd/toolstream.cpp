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

ToolStream::ToolStream()
    : mCursor(0), mBuffer(new char[kToolStreamBufferSize]), mFill(0), mArrived(0), mConsumed(0) {
}

void ToolStream::Connect() {
    // Yes, the binary prints the four addresses through %u; they are 32 bits on the target.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
    printf("ToolStream connect: %u %u %u %u\n", &mFill, &mArrived, &mConsumed, mBuffer);
#pragma GCC diagnostic pop
}

ToolStream::~ToolStream() {
    if (mBuffer != nullptr) {
        delete[] mBuffer;
    }
}

Stream &ToolStream::Read(void *pDest, int nSize) {
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

Stream &ToolStream::Write([[maybe_unused]] const void *pSrc, [[maybe_unused]] int nSize) {
    Rnd::TheDbg.Notify("Can't write to a PS ToolStream\n");
    if (Rnd::TheDbg.mAbortProc != nullptr) {
        Rnd::TheDbg.mAbortProc();
    } else {
        throw; // With no handler the binary rethrows the exception in flight.
    }
    return *this;
}

Stream &ToolStream::Seek([[maybe_unused]] int nOffset, [[maybe_unused]] int nWhence) {
    return *this;
}

int ToolStream::Tell() {
    return 0;
}

Stream &ToolStream::Flush() {
    mCursor = 0;
    mConsumed = mArrived;
    return *this;
}

int ToolStream::Eof() {
    return mArrived == mConsumed;
}

int ToolStream::Fail() {
    return 0;
}

} // namespace Rnd
