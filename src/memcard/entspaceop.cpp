#include "memcard/entspaceop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

EntSpaceOp::EntSpaceOp(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, void *pCookie)
    : MemcardOp(pHandler, nPortSlot, pCookie), mPath(path) {
}

// 0x0055d418
EntSpaceOp::~EntSpaceOp() {
}

// 0x0055e4a8
void EntSpaceOp::Issue() {
    sceMcGetEntSpace(mPortSlot >> kMemcardPortShift,
                     mPortSlot & kMemcardSlotMask,
                     mPath.mStr != nullptr ? mPath.mStr : g_szEmptyString);
    mIssued = kMemcardOpInFlight;
}

// 0x0055d480
void EntSpaceOp::Complete() {
    InterpretResult();
    mHandler->OnEntSpace(this);
}

// 0x0055e4f8
void EntSpaceOp::InterpretResult() {
    if (mResult >= sceMcResSucceed) {
        mStatus = kMemcardStatusOk;
    } else if (mResult == sceMcResNoFormat) {
        mStatus = kMemcardStatusNotFormatted;
    } else {
        mStatus = kMemcardStatusUnknown;
    }
}
