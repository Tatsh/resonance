#include "memcard/seekop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

// 0x0055eaa8
SeekOp::SeekOp(MemcardCBHandler *pHandler, int nFile, int nOffset, int nOrigin, int nCookie)
    : MemcardOp(pHandler, nCookie), mFile(nFile), mOffset(nOffset), mOrigin(nOrigin) {
}

// 0x0055dba8
SeekOp::~SeekOp() {
}

// 0x0055ead8
void SeekOp::Issue() {
    sceMcSeek(mFile, mOffset, mOrigin);
    mIssued = kMemcardOpInFlight;
}

// 0x0055dbd8
void SeekOp::Complete() {
    InterpretResult();
    mHandler->OnSeek(this);
}

// 0x0055eb10
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
