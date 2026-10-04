#include "memcard/closeop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

CloseOp::CloseOp(MemcardCBHandler *pHandler, int nFile, int nCookie)
    : MemcardOp(pHandler, nCookie), mFile(nFile) {
}

CloseOp::~CloseOp() {
}

void CloseOp::Execute() {
    sceMcClose(mFile);
    mIssued = kMemcardOpInFlight;
}

void CloseOp::NotifyDone() {
    InterpretResult();
    mHandler->OnClose(this);
}

void CloseOp::InterpretResult() {
    switch (mResult) {
    case sceMcResSucceed:
        mStatus = kMemcardStatusOk;
        break;
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
