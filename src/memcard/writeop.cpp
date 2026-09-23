#include "memcard/writeop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

// 0x0055e9b8
WriteOp::WriteOp(MemcardCBHandler *pHandler,
                 int nPortSlot,
                 int nFile,
                 const void *pBuffer,
                 int nLength,
                 int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie), mFile(nFile), mBuffer(pBuffer), mLength(nLength) {
}

// 0x0055dab0
WriteOp::~WriteOp() {
}

// 0x0055e9e8
void WriteOp::Issue() {
    sceMcWrite(mFile, mBuffer, mLength);
    mIssued = kMemcardOpInFlight;
}

// 0x0055dae0
void WriteOp::Complete() {
    InterpretResult();
    mHandler->OnWrite(this);
}

// 0x0055ea20
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
