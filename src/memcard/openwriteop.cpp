#include "memcard/openwriteop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

// NTSC-U/C: 0x0055eb58, PAL: 0x0059fe28
OpenWriteOp::OpenWriteOp(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie), mPath(path) {
}

// NTSC-U/C: 0x0055dca0, PAL: 0x0059ef18
OpenWriteOp::~OpenWriteOp() {
}

// NTSC-U/C: 0x0055ebe8, PAL: 0x0059feb8
void OpenWriteOp::Execute() {
    sceMcOpen(mPortSlot >> kMemcardPortShift,
              mPortSlot & kMemcardSlotMask,
              mPath.mStr != nullptr ? mPath.mStr : g_szEmptyString,
              sceMcFileCreateFile | sceMcFileAttrWriteable);
    mIssued = kMemcardOpInFlight;
}

// NTSC-U/C: 0x0055dd08, PAL: 0x0059ef90
void OpenWriteOp::NotifyDone() {
    InterpretResult();
    mHandler->OnOpenWrite(this);
}

// NTSC-U/C: 0x0055ec38, PAL: 0x0059ff08
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
