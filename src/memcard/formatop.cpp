#include "memcard/formatop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

FormatOp::FormatOp(MemcardCBHandler *pHandler, int nPortSlot, int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie) {
}

FormatOp::~FormatOp() {
}

void FormatOp::Execute() {
    sceMcFormat(mPortSlot >> kMemcardPortShift, mPortSlot & kMemcardSlotMask);
    mIssued = kMemcardOpInFlight;
}

void FormatOp::NotifyDone() {
    InterpretResult();
    mHandler->OnFormat(this);
}

void FormatOp::InterpretResult() {
    mStatus = mResult == sceMcResSucceed ? kMemcardStatusOk : kMemcardStatusUnknown;
}
