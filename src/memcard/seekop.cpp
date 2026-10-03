#include "memcard/seekop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

// NTSC-U/C: 0x0055eaa8, PAL: 0x0059fd78
SeekOp::SeekOp(MemcardCBHandler *pHandler, int nFile, int nOffset, int nOrigin, int nCookie)
    : MemcardOp(pHandler, nCookie), mFile(nFile), mOffset(nOffset), mOrigin(nOrigin) {
}

// NTSC-U/C: 0x0055dba8, PAL: 0x0059ee20
SeekOp::~SeekOp() {
}

// NTSC-U/C: 0x0055ead8, PAL: 0x0059fda8
void SeekOp::Execute() {
    sceMcSeek(mFile, mOffset, mOrigin);
    mIssued = kMemcardOpInFlight;
}

// NTSC-U/C: 0x0055dbd8, PAL: 0x0059ee50
void SeekOp::NotifyDone() {
    InterpretResult();
    mHandler->OnSeek(this);
}

// NTSC-U/C: 0x0055eb10, PAL: 0x0059fde0
void SeekOp::InterpretResult() {
    if (mResult >= sceMcResSucceed) {
        mPosition = mResult; // Yes, the binary returns here without writing mStatus.
        return;
    }

    switch (mResult) {
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
