#include "memcard/closeop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

// NTSC-U/C: 0x0055ee28, PAL: 0x005a00f8
CloseOp::CloseOp(MemcardCBHandler *pHandler, int nFile, int nCookie)
    : MemcardOp(pHandler, nCookie), mFile(nFile) {
}

// NTSC-U/C: 0x0055df00, PAL: 0x0059f198
CloseOp::~CloseOp() {
}

// NTSC-U/C: 0x0055ee50, PAL: 0x005a0120
void CloseOp::Execute() {
    sceMcClose(mFile);
    mIssued = kMemcardOpInFlight;
}

// NTSC-U/C: 0x0055df30, PAL: 0x0059f1c8
void CloseOp::NotifyDone() {
    InterpretResult();
    mHandler->OnClose(this);
}

// NTSC-U/C: 0x0055ee80, PAL: 0x005a0150
void CloseOp::InterpretResult() {
    switch (mResult) {
    case sceMcResSucceed:
        mStatus = kMemcardStatusOk;
        break;
    case sceMcResNoFormat:
        mStatus = kMemcardStatusNotFormatted;
        break;
    case sceMcResNoEntry:
        mStatus = kMemcardStatusBadFile;
        break;
    default:
        mStatus = kMemcardStatusUnknown;
        break;
    }
}
