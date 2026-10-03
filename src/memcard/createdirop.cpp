#include "memcard/createdirop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

// NTSC-U/C: 0x0055e628, PAL: 0x0059f8f8
CreateDirOp::CreateDirOp(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie), mPath(path) {
}

// NTSC-U/C: 0x0055d738, PAL: 0x0059e990
CreateDirOp::~CreateDirOp() {
}

// NTSC-U/C: 0x0055e6b8, PAL: 0x0059f988
void CreateDirOp::Issue() {
    sceMcMkdir(mPortSlot >> kMemcardPortShift,
               mPortSlot & kMemcardSlotMask,
               mPath.mStr != nullptr ? mPath.mStr : g_szEmptyString);
    mIssued = kMemcardOpInFlight;
}

// NTSC-U/C: 0x0055d7a0, PAL: 0x0059ea08
void CreateDirOp::Complete() {
    InterpretResult();
    mHandler->OnCreateDir(this);
}

// NTSC-U/C: 0x0055e708, PAL: 0x0059f9d8
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
