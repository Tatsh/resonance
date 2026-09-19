#include "memcard/checkinfoop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

CheckInfoOp::CheckInfoOp(MemcardCBHandler *pHandler, int nPortSlot, void *pCookie)
    : MemcardOp(pHandler, nPortSlot, pCookie) {
}

// 0x0055d320
CheckInfoOp::~CheckInfoOp() {
}

// 0x0055e390
void CheckInfoOp::Issue() {
    sceMcGetInfo(
        mPortSlot >> kMemcardPortShift, mPortSlot & kMemcardSlotMask, &mType, &mFree, &mFormatted);
    mIssued = kMemcardOpInFlight;
}

// 0x0055d350
void CheckInfoOp::Complete() {
    InterpretResult();
    mHandler->OnCheckInfo(this);
}

// 0x0055e3d8
void CheckInfoOp::InterpretResult() {
    if (mResult == sceMcResNoFormat) {
        mStatus = kMemcardStatusNotFormatted;
    } else if (mResult < sceMcResNoFormat || mResult > sceMcResSucceed) {
        mStatus = kMemcardStatusUnknown;
    } else {
        mStatus = kMemcardStatusOk;
    }
}
