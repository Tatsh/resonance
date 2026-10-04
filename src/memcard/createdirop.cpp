#include "memcard/createdirop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

CreateDirOp::CreateDirOp(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie), mPath(path) {
}

CreateDirOp::~CreateDirOp() {
}

void CreateDirOp::Execute() {
    sceMcMkdir(mPortSlot >> kMemcardPortShift,
               mPortSlot & kMemcardSlotMask,
               mPath.mStr != nullptr ? mPath.mStr : g_szEmptyString);
    mIssued = kMemcardOpInFlight;
}

void CreateDirOp::NotifyDone() {
    InterpretResult();
    mHandler->OnCreateDir(this);
}

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
