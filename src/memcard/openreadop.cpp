#include "memcard/openreadop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

// NTSC-U/C: 0x0055ecc0, PAL: 0x0059ff90
OpenReadOp::OpenReadOp(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie), mPath(path) {
}

// NTSC-U/C: 0x0055ddd0, PAL: 0x0059f058
OpenReadOp::~OpenReadOp() {
}

// NTSC-U/C: 0x0055ed50, PAL: 0x005a0020
void OpenReadOp::Execute() {
    sceMcOpen(mPortSlot >> kMemcardPortShift,
              mPortSlot & kMemcardSlotMask,
              mPath.mStr != nullptr ? mPath.mStr : g_szEmptyString,
              sceMcFileAttrReadable);
    mIssued = kMemcardOpInFlight;
}

// NTSC-U/C: 0x0055de38, PAL: 0x0059f0d0
void OpenReadOp::NotifyDone() {
    InterpretResult();
    mHandler->OnOpenRead(this);
}

// NTSC-U/C: 0x0055eda0, PAL: 0x005a0070
void OpenReadOp::InterpretResult() {
    mFile = mResult; // Yes, a failed open stores its error code in mFile as well.
    if (mResult >= sceMcResSucceed) {
        mStatus = kMemcardStatusOk;
        return;
    }

    switch (mResult) {
    case sceMcResNoFormat:
        mStatus = kMemcardStatusNotFormatted;
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
