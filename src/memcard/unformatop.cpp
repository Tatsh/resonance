#include "memcard/unformatop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

UnformatOp::UnformatOp(MemcardCBHandler *pHandler, int nPortSlot, void *pCookie)
    : MemcardOp(pHandler, nPortSlot, pCookie) {
}

// 0x0055d640
UnformatOp::~UnformatOp() {
}

// 0x0055e5d0
void UnformatOp::Issue() {
    sceMcUnformat(mPortSlot >> kMemcardPortShift, mPortSlot & kMemcardSlotMask);
    mIssued = kMemcardOpInFlight;
}

// 0x0055d670
void UnformatOp::Complete() {
    InterpretResult();
    mHandler->OnUnformat(this);
}

// 0x0055e608
void UnformatOp::InterpretResult() {
    mStatus = mResult == sceMcResSucceed ? kMemcardStatusOk : kMemcardStatusUnknown;
}
