#include "memcard/createdirop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

CreateDirOp::CreateDirOp(MemcardCBHandler *pHandler,
                         int nPortSlot,
                         const HxStr &path,
                         void *pCookie)
    : MemcardOp(pHandler, nPortSlot, pCookie), mPath(path) {
}

// 0x0055d738
CreateDirOp::~CreateDirOp() {
}

// 0x0055e6b8
void CreateDirOp::Issue() {
    sceMcMkDir(mPortSlot >> kMemcardPortShift,
               mPortSlot & kMemcardSlotMask,
               mPath.mStr != nullptr ? mPath.mStr : g_pszEmptyString);
    mIssued = kMemcardOpInFlight;
}

// 0x0055d7a0
void CreateDirOp::Complete() {
    InterpretResult();
    mHandler->OnCreateDir(this);
}

// 0x0055e708
void CreateDirOp::InterpretResult() {
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
