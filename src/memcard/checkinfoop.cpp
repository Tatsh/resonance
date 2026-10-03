#include "memcard/checkinfoop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

// NTSC-U/C: 0x0055e368, PAL: 0x0059f638
CheckInfoOp::CheckInfoOp(MemcardCBHandler *pHandler, int nPortSlot, int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie) {
}

// NTSC-U/C: 0x0055d320, PAL: 0x0059e568
CheckInfoOp::~CheckInfoOp() {
}

// NTSC-U/C: 0x0055e390, PAL: 0x0059f660
void CheckInfoOp::Issue() {
    sceMcGetInfo(
        mPortSlot >> kMemcardPortShift, mPortSlot & kMemcardSlotMask, &mType, &mFree, &mFormatted);
    mIssued = kMemcardOpInFlight;
}

// NTSC-U/C: 0x0055d350, PAL: 0x0059e598
void CheckInfoOp::Complete() {
    InterpretResult();
    mHandler->OnCheckInfo(this);
}

// NTSC-U/C: 0x0055e3d8, PAL: 0x0059f6a8
void CheckInfoOp::InterpretResult() {
    if (mResult == sceMcResNoFormat) {
        mStatus = kMemcardStatusNotFormatted;
    } else if (mResult < sceMcResNoFormat || mResult > sceMcResSucceed) {
        mStatus = kMemcardStatusUnknown;
    } else {
        mStatus = kMemcardStatusOk;
    }
}
