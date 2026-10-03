#include "memcard/writeop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

// NTSC-U/C: 0x0055e9b8, PAL: 0x0059fc88
WriteOp::WriteOp(MemcardCBHandler *pHandler,
                 int nPortSlot,
                 int nFile,
                 const void *pBuffer,
                 int nLength,
                 int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie), mFile(nFile), mBuffer(pBuffer), mLength(nLength) {
}

// NTSC-U/C: 0x0055dab0, PAL: 0x0059ed28
WriteOp::~WriteOp() {
}

// NTSC-U/C: 0x0055e9e8, PAL: 0x0059fcb8
void WriteOp::Issue() {
    sceMcWrite(mFile, mBuffer, mLength);
    mIssued = kMemcardOpInFlight;
}

// NTSC-U/C: 0x0055dae0, PAL: 0x0059ed58
void WriteOp::Complete() {
    InterpretResult();
    mHandler->OnWrite(this);
}

// NTSC-U/C: 0x0055ea20, PAL: 0x0059fcf0
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
