#include "memcard/deletefileop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

// NTSC-U/C: 0x0055eed8, PAL: 0x005a01a8
DeleteFileOp::DeleteFileOp(MemcardCBHandler *pHandler,
                           int nPortSlot,
                           const HxStr &path,
                           int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie), mPath(path) {
}

// NTSC-U/C: 0x0055dff8, PAL: 0x0059f290
DeleteFileOp::~DeleteFileOp() {
}

// NTSC-U/C: 0x0055ef68, PAL: 0x005a0238
void DeleteFileOp::Issue() {
    sceMcDelete(mPortSlot >> kMemcardPortShift,
                mPortSlot & kMemcardSlotMask,
                mPath.mStr != nullptr ? mPath.mStr : g_szEmptyString);
    mIssued = kMemcardOpInFlight;
}

// NTSC-U/C: 0x0055e060, PAL: 0x0059f308
void DeleteFileOp::Complete() {
    InterpretResult();
    mHandler->OnDeleteFile(this);
}

// NTSC-U/C: 0x0055efb8, PAL: 0x005a0288
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
