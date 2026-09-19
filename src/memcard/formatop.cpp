#include "memcard/formatop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

FormatOp::FormatOp(MemcardCBHandler *pHandler, int nPortSlot, void *pCookie)
    : MemcardOp(pHandler, nPortSlot, pCookie) {
}

// 0x0055d548
FormatOp::~FormatOp() {
}

// 0x0055e550
void FormatOp::Issue() {
    sceMcFormat(mPortSlot >> kMemcardPortShift, mPortSlot & kMemcardSlotMask);
    mIssued = kMemcardOpInFlight;
}

// 0x0055d578
void FormatOp::Complete() {
    InterpretResult();
    mHandler->OnFormat(this);
}

// 0x0055e588
void FormatOp::InterpretResult() {
    mStatus = mResult == sceMcResSucceed ? kMemcardStatusOk : kMemcardStatusUnknown;
}
