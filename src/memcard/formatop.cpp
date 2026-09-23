#include "memcard/formatop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

// 0x0055e528
FormatOp::FormatOp(MemcardCBHandler *pHandler, int nPortSlot, int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie) {
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
