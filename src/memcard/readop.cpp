#include "memcard/readop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

ReadOp::ReadOp(
    MemcardCBHandler *pHandler, int nPortSlot, int nFile, void *pBuffer, int nLength, int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie), mFile(nFile), mBuffer(pBuffer), mLength(nLength) {
}

ReadOp::~ReadOp() {
}

void ReadOp::Execute() {
    sceMcRead(mFile, mBuffer, mLength);
    mIssued = kMemcardOpInFlight;
}

void ReadOp::NotifyDone() {
    InterpretResult();
    mHandler->OnRead(this);
}

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
