#include "memcard/formatop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

// NTSC-U/C: 0x0055e528, PAL: 0x0059f7f8
FormatOp::FormatOp(MemcardCBHandler *pHandler, int nPortSlot, int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie) {
}

// NTSC-U/C: 0x0055d548, PAL: 0x0059e7a0
FormatOp::~FormatOp() {
}

// NTSC-U/C: 0x0055e550, PAL: 0x0059f820
void FormatOp::Issue() {
    sceMcFormat(mPortSlot >> kMemcardPortShift, mPortSlot & kMemcardSlotMask);
    mIssued = kMemcardOpInFlight;
}

// NTSC-U/C: 0x0055d578, PAL: 0x0059e7d0
void FormatOp::Complete() {
    InterpretResult();
    mHandler->OnFormat(this);
}

// NTSC-U/C: 0x0055e588, PAL: 0x0059f858
void FormatOp::InterpretResult() {
    mStatus = mResult == sceMcResSucceed ? kMemcardStatusOk : kMemcardStatusUnknown;
}
