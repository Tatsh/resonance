#include "memcard/checkinfoop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

CheckInfoOp::CheckInfoOp(MemcardCBHandler *pHandler, int nPortSlot, int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie) {
}

CheckInfoOp::~CheckInfoOp() {
}

void CheckInfoOp::Execute() {
    sceMcGetInfo(
        mPortSlot >> kMemcardPortShift, mPortSlot & kMemcardSlotMask, &mType, &mFree, &mFormatted);
    mIssued = kMemcardOpInFlight;
}

void CheckInfoOp::NotifyDone() {
    InterpretResult();
    mHandler->OnCheckInfo(this);
}

void CheckInfoOp::InterpretResult() {
    if (mResult == sceMcResNoFormat) {
        mStatus = kMemcardStatusNotFormatted;
    } else if (mResult < sceMcResNoFormat || mResult > sceMcResSucceed) {
        mStatus = kMemcardStatusUnknown;
    } else {
        mStatus = kMemcardStatusOk;
    }
}
