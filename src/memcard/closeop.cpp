#include "memcard/closeop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

// 0x0055ee28
CloseOp::CloseOp(MemcardCBHandler *pHandler, int nFile, int nCookie)
    : MemcardOp(pHandler, nCookie), mFile(nFile) {
}

// 0x0055df00
CloseOp::~CloseOp() {
}

// 0x0055ee50
void CloseOp::Issue() {
    sceMcClose(mFile);
    mIssued = kMemcardOpInFlight;
}

// 0x0055df30
void CloseOp::Complete() {
    InterpretResult();
    mHandler->OnClose(this);
}

// 0x0055ee80
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
