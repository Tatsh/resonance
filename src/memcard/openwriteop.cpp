#include "memcard/openwriteop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

OpenWriteOp::OpenWriteOp(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie), mPath(path) {
}

OpenWriteOp::~OpenWriteOp() {
}

void OpenWriteOp::Execute() {
    sceMcOpen(mPortSlot >> kMemcardPortShift,
              mPortSlot & kMemcardSlotMask,
              mPath.mStr != nullptr ? mPath.mStr : g_szEmptyString,
              sceMcFileCreateFile | sceMcFileAttrWriteable);
    mIssued = kMemcardOpInFlight;
}

void OpenWriteOp::NotifyDone() {
    InterpretResult();
    mHandler->OnOpenWrite(this);
}

void OpenWriteOp::InterpretResult() {
    if (mResult >= sceMcResSucceed) {
        mFile = mResult;
        mStatus = kMemcardStatusOk;
        return;
    }

    switch (mResult) {
    case sceMcResNoFormat:
        mStatus = kMemcardStatusNotFormatted;
        break;
    case sceMcResFullDevice:
        mStatus = kMemcardStatusCardFull;
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
