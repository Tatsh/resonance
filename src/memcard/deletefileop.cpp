#include "memcard/deletefileop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

DeleteFileOp::DeleteFileOp(MemcardCBHandler *pHandler,
                           int nPortSlot,
                           const HxStr &path,
                           void *pCookie)
    : MemcardOp(pHandler, nPortSlot, pCookie), mPath(path) {
}

// 0x0055dff8
DeleteFileOp::~DeleteFileOp() {
}

// 0x0055ef68
void DeleteFileOp::Issue() {
    sceMcDelete(mPortSlot >> kMemcardPortShift,
                mPortSlot & kMemcardSlotMask,
                mPath.mStr != nullptr ? mPath.mStr : g_szEmptyString);
    mIssued = kMemcardOpInFlight;
}

// 0x0055e060
void DeleteFileOp::Complete() {
    InterpretResult();
    mHandler->OnDeleteFile(this);
}

// 0x0055efb8
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
