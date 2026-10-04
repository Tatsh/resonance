#include "memcard/openreadop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

OpenReadOp::OpenReadOp(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie), mPath(path) {
}

OpenReadOp::~OpenReadOp() {
}

void OpenReadOp::Execute() {
    sceMcOpen(mPortSlot >> kMemcardPortShift,
              mPortSlot & kMemcardSlotMask,
              mPath.mStr != nullptr ? mPath.mStr : g_szEmptyString,
              sceMcFileAttrReadable);
    mIssued = kMemcardOpInFlight;
}

void OpenReadOp::NotifyDone() {
    InterpretResult();
    mHandler->OnOpenRead(this);
}

void OpenReadOp::InterpretResult() {
    mFile = mResult; // Yes, a failed open stores its error code in mFile as well.
    if (mResult >= sceMcResSucceed) {
        mStatus = kMemcardStatusOk;
        return;
    }

    switch (mResult) {
    case sceMcResNoFormat:
        mStatus = kMemcardStatusNotFormatted;
        break;
    case sceMcResNoEntry:
        mStatus = kMemcardStatusNoEntry;
        break;
    case sceMcResDeniedPermit:
        mStatus = kMemcardStatusDenied;
        break;
    case sceMcResUpLimitHandle:
        mStatus = kMemcardStatusTooManyOpen;
        break;
    default:
        mStatus = kMemcardStatusUnknown;
        break;
    }
}
