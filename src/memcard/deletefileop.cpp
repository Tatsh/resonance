#include "memcard/deletefileop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

DeleteFileOp::DeleteFileOp(MemcardCBHandler *pHandler,
                           int nPortSlot,
                           const HxStr &path,
                           int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie), mPath(path) {
}

DeleteFileOp::~DeleteFileOp() {
}

void DeleteFileOp::Execute() {
    sceMcDelete(mPortSlot >> kMemcardPortShift,
                mPortSlot & kMemcardSlotMask,
                mPath.mStr != nullptr ? mPath.mStr : g_szEmptyString);
    mIssued = kMemcardOpInFlight;
}

void DeleteFileOp::NotifyDone() {
    InterpretResult();
    mHandler->OnDeleteFile(this);
}

void DeleteFileOp::InterpretResult() {
    switch (mResult) {
    case sceMcResSucceed:
        mStatus = kMemcardStatusOk;
        break;
    case sceMcResNoFormat:
        mStatus = kMemcardStatusNotFormatted;
        break;
    case sceMcResNoEntry:
        mStatus = kMemcardStatusNoFile;
        break;
    case sceMcResDeniedPermit:
        mStatus = kMemcardStatusDenied;
        break;
    case sceMcResNotEmpty:
        mStatus = kMemcardStatusFailed;
        break;
    default:
        mStatus = kMemcardStatusUnknown;
        break;
    }
}
