#include "memcard/openreadop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

OpenReadOp::OpenReadOp(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, void *pCookie)
    : MemcardOp(pHandler, nPortSlot, pCookie), mPath(path) {
}

// 0x0055ddd0
OpenReadOp::~OpenReadOp() {
}

// 0x0055ed50
void OpenReadOp::Issue() {
    sceMcOpen(mPortSlot >> kMemcardPortShift,
              mPortSlot & kMemcardSlotMask,
              mPath.mStr != nullptr ? mPath.mStr : g_szEmptyString,
              sceMcFileAttrReadable);
    mIssued = kMemcardOpInFlight;
}

// 0x0055de38
void OpenReadOp::Complete() {
    InterpretResult();
    mHandler->OnOpenRead(this);
}

// 0x0055eda0
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
