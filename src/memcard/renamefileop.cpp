#include "memcard/renamefileop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

RenameFileOp::RenameFileOp(MemcardCBHandler *pHandler,
                           int nPortSlot,
                           const HxStr &oldPath,
                           const HxStr &newPath,
                           int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie), mOldPath(oldPath), mNewPath(newPath) {
}

RenameFileOp::~RenameFileOp() {
}

void RenameFileOp::Execute() {
    sceMcRename(mPortSlot >> kMemcardPortShift,
                mPortSlot & kMemcardSlotMask,
                mOldPath.mStr != nullptr ? mOldPath.mStr : g_szEmptyString,
                mNewPath.mStr != nullptr ? mNewPath.mStr : g_szEmptyString);
    mIssued = kMemcardOpInFlight;
}

void RenameFileOp::NotifyDone() {
    InterpretResult();
    mHandler->OnRenameFile(this);
}

void RenameFileOp::InterpretResult() {
    switch (mResult) {
    case sceMcResSucceed:
        mStatus = kMemcardStatusOk;
        break;
    case sceMcResNoFormat:
        mStatus = kMemcardStatusNotFormatted;
        break;
    case sceMcResFullDevice:
        mStatus = kMemcardStatusCardFull;
        break;
    case sceMcResNoEntry:
        mStatus = kMemcardStatusNoEntry;
        break;
    default:
        mStatus = kMemcardStatusUnknown;
        break;
    }
}
