#include "memcard/writeop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

WriteOp::WriteOp(MemcardCBHandler *pHandler,
                 int nPortSlot,
                 int nFile,
                 const void *pBuffer,
                 int nLength,
                 int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie), mFile(nFile), mBuffer(pBuffer), mLength(nLength) {
}

WriteOp::~WriteOp() {
}

void WriteOp::Execute() {
    sceMcWrite(mFile, mBuffer, mLength);
    mIssued = kMemcardOpInFlight;
}

void WriteOp::NotifyDone() {
    InterpretResult();
    mHandler->OnWrite(this);
}

void WriteOp::InterpretResult() {
    if (mResult >= sceMcResSucceed) {
        mBytesTransferred = mResult;
        mStatus = kMemcardStatusOk;
        return;
    }

    switch (mResult) {
    case sceMcResNoFormat:
        mStatus = kMemcardStatusNotFormatted;
        break;
    case sceMcResFullDevice:
        mStatus = kMemcardStatusCardFull;
        break;
    case sceMcResNoEntry:
        mStatus = kMemcardStatusBadFile;
        break;
    case sceMcResDeniedPermit:
        mStatus = kMemcardStatusWriteDenied;
        break;
    case sceMcResFailReplace:
        mStatus = kMemcardStatusFailed;
        break;
    default:
        mStatus = kMemcardStatusUnknown;
        break;
    }
}
