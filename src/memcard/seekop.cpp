#include "memcard/seekop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

SeekOp::SeekOp(MemcardCBHandler *pHandler, int nFile, int nOffset, int nOrigin, int nCookie)
    : MemcardOp(pHandler, nCookie), mFile(nFile), mOffset(nOffset), mOrigin(nOrigin) {
}

SeekOp::~SeekOp() {
}

void SeekOp::Execute() {
    sceMcSeek(mFile, mOffset, mOrigin);
    mIssued = kMemcardOpInFlight;
}

void SeekOp::NotifyDone() {
    InterpretResult();
    mHandler->OnSeek(this);
}

void SeekOp::InterpretResult() {
    if (mResult >= sceMcResSucceed) {
        mPosition = mResult; // Yes, the binary returns here without writing mStatus.
        return;
    }

    switch (mResult) {
    case sceMcResNoFormat:
        mStatus = kMemcardStatusNotFormatted;
        break;
    case sceMcResNoEntry:
        mStatus = kMemcardStatusBadFile;
        break;
    default:
        mStatus = kMemcardStatusUnknown;
        break;
    }
}
