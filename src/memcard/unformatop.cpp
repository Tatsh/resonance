#include "memcard/unformatop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

// NTSC-U/C: 0x0055e5a8, PAL: 0x0059f878
UnformatOp::UnformatOp(MemcardCBHandler *pHandler, int nPortSlot, int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie) {
}

// NTSC-U/C: 0x0055d640, PAL: 0x0059e898
UnformatOp::~UnformatOp() {
}

// NTSC-U/C: 0x0055e5d0, PAL: 0x0059f8a0
void UnformatOp::Issue() {
    sceMcUnformat(mPortSlot >> kMemcardPortShift, mPortSlot & kMemcardSlotMask);
    mIssued = kMemcardOpInFlight;
}

// NTSC-U/C: 0x0055d670, PAL: 0x0059e8c8
void UnformatOp::Complete() {
    InterpretResult();
    mHandler->OnUnformat(this);
}

// NTSC-U/C: 0x0055e608, PAL: 0x0059f8d8
void UnformatOp::InterpretResult() {
    mStatus = mResult == sceMcResSucceed ? kMemcardStatusOk : kMemcardStatusUnknown;
}
