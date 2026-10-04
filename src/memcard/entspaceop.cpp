#include "memcard/entspaceop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

EntSpaceOp::EntSpaceOp(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie), mPath(path) {
}

EntSpaceOp::~EntSpaceOp() {
}

void EntSpaceOp::Execute() {
    sceMcGetEntSpace(mPortSlot >> kMemcardPortShift,
                     mPortSlot & kMemcardSlotMask,
                     mPath.mStr != nullptr ? mPath.mStr : g_szEmptyString);
    mIssued = kMemcardOpInFlight;
}

void EntSpaceOp::NotifyDone() {
    InterpretResult();
    mHandler->OnEntSpace(this);
}

void EntSpaceOp::InterpretResult() {
    if (mResult >= sceMcResSucceed) {
        mStatus = kMemcardStatusOk;
    } else if (mResult == sceMcResNoFormat) {
        mStatus = kMemcardStatusNotFormatted;
    } else {
        mStatus = kMemcardStatusUnknown;
    }
}
