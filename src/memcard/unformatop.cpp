#include "memcard/unformatop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

UnformatOp::UnformatOp(MemcardCBHandler *pHandler, int nPortSlot, int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie) {
}

UnformatOp::~UnformatOp() {
}

void UnformatOp::Execute() {
    sceMcUnformat(mPortSlot >> kMemcardPortShift, mPortSlot & kMemcardSlotMask);
    mIssued = kMemcardOpInFlight;
}

void UnformatOp::NotifyDone() {
    InterpretResult();
    mHandler->OnUnformat(this);
}

void UnformatOp::InterpretResult() {
    mStatus = mResult == sceMcResSucceed ? kMemcardStatusOk : kMemcardStatusUnknown;
}
