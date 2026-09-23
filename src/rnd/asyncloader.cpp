#include "rnd/asyncloader.h"

// 0x003f7e50
RndAsyncLoader::RndAsyncLoader() : mPending(1), mStarted(0), mFinished(0), mPriority(-1) {
}

// 0x003fc708
void RndAsyncLoader::Restart(const HxStr &directory, const HxStr &file) {
    Cancel();
    mDirectory = directory;
    mFile = file;
    mFinished = 0;
    mPending = 1;
    mStarted = 0;
}
