#include "memcard/renamefileop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

RenameFileOp::RenameFileOp(MemcardCBHandler *pHandler,
                           int nPortSlot,
                           const HxStr &oldPath,
                           const HxStr &newPath,
                           void *pCookie)
    : MemcardOp(pHandler, nPortSlot, pCookie), mOldPath(oldPath), mNewPath(newPath) {
}

// 0x0055e128
RenameFileOp::~RenameFileOp() {
}

// 0x0055f108
void RenameFileOp::Issue() {
    sceMcRename(mPortSlot >> kMemcardPortShift,
                mPortSlot & kMemcardSlotMask,
                mOldPath.mStr != nullptr ? mOldPath.mStr : g_szEmptyString,
                mNewPath.mStr != nullptr ? mNewPath.mStr : g_szEmptyString);
    mIssued = kMemcardOpInFlight;
}

// 0x0055e1a0
void RenameFileOp::Complete() {
    InterpretResult();
    mHandler->OnRenameFile(this);
}

// 0x0055f168
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
