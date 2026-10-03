#include "memcard/entspaceop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

// NTSC-U/C: 0x0055e418, PAL: 0x0059f6e8
EntSpaceOp::EntSpaceOp(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie), mPath(path) {
}

// NTSC-U/C: 0x0055d418, PAL: 0x0059e660
EntSpaceOp::~EntSpaceOp() {
}

// NTSC-U/C: 0x0055e4a8, PAL: 0x0059f778
void EntSpaceOp::Execute() {
    sceMcGetEntSpace(mPortSlot >> kMemcardPortShift,
                     mPortSlot & kMemcardSlotMask,
                     mPath.mStr != nullptr ? mPath.mStr : g_szEmptyString);
    mIssued = kMemcardOpInFlight;
}

// NTSC-U/C: 0x0055d480, PAL: 0x0059e6d8
void EntSpaceOp::NotifyDone() {
    InterpretResult();
    mHandler->OnEntSpace(this);
}

// NTSC-U/C: 0x0055e4f8, PAL: 0x0059f7c8
void EntSpaceOp::InterpretResult() {
    if (mResult >= sceMcResSucceed) {
        mStatus = kMemcardStatusOk;
    } else if (mResult == sceMcResNoFormat) {
        mStatus = kMemcardStatusNotFormatted;
    } else {
        mStatus = kMemcardStatusUnknown;
    }
}
