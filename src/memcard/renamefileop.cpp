#include "memcard/renamefileop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

// NTSC-U/C: 0x0055f030, PAL: 0x005a0300
RenameFileOp::RenameFileOp(MemcardCBHandler *pHandler,
                           int nPortSlot,
                           const HxStr &oldPath,
                           const HxStr &newPath,
                           int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie), mOldPath(oldPath), mNewPath(newPath) {
}

// NTSC-U/C: 0x0055e128, PAL: 0x0059f3d0
RenameFileOp::~RenameFileOp() {
}

// NTSC-U/C: 0x0055f108, PAL: 0x005a03e8
void RenameFileOp::Issue() {
    sceMcRename(mPortSlot >> kMemcardPortShift,
                mPortSlot & kMemcardSlotMask,
                mOldPath.mStr != nullptr ? mOldPath.mStr : g_szEmptyString,
                mNewPath.mStr != nullptr ? mNewPath.mStr : g_szEmptyString);
    mIssued = kMemcardOpInFlight;
}

// NTSC-U/C: 0x0055e1a0, PAL: 0x0059f470
void RenameFileOp::Complete() {
    InterpretResult();
    mHandler->OnRenameFile(this);
}

// NTSC-U/C: 0x0055f168, PAL: 0x005a0448
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
