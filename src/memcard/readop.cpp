#include "memcard/readop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

// 0x0055e8e0
ReadOp::ReadOp(
    MemcardCBHandler *pHandler, int nPortSlot, int nFile, void *pBuffer, int nLength, int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie), mFile(nFile), mBuffer(pBuffer), mLength(nLength) {
}

// 0x0055d9b8
ReadOp::~ReadOp() {
}

// 0x0055e910
void ReadOp::Issue() {
    sceMcRead(mFile, mBuffer, mLength);
    mIssued = kMemcardOpInFlight;
}

// 0x0055d9e8
void ReadOp::Complete() {
    InterpretResult();
    mHandler->OnRead(this);
}

// 0x0055e948
void ReadOp::InterpretResult() {
    if (mResult >= sceMcResSucceed) {
        mBytesTransferred = mResult;
        mStatus = kMemcardStatusOk;
        return;
    }

    switch (mResult) {
    case sceMcResNoFormat:
        mStatus = kMemcardStatusNotFormatted;
        break;
    case sceMcResNoEntry:
        mStatus = kMemcardStatusBadFile;
        break;
    case sceMcResDeniedPermit:
        mStatus = kMemcardStatusNoEntry;
        break;
    default:
        mStatus = kMemcardStatusUnknown;
        break;
    }
}
