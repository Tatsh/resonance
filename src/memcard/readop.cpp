#include "memcard/readop.h"

#include <libmc.h>

#include "memcard/memcardcbhandler.h"

// NTSC-U/C: 0x0055e8e0, PAL: 0x0059fbb0
ReadOp::ReadOp(
    MemcardCBHandler *pHandler, int nPortSlot, int nFile, void *pBuffer, int nLength, int nCookie)
    : MemcardOp(pHandler, nPortSlot, nCookie), mFile(nFile), mBuffer(pBuffer), mLength(nLength) {
}

// NTSC-U/C: 0x0055d9b8, PAL: 0x0059ec30
ReadOp::~ReadOp() {
}

// NTSC-U/C: 0x0055e910, PAL: 0x0059fbe0
void ReadOp::Issue() {
    sceMcRead(mFile, mBuffer, mLength);
    mIssued = kMemcardOpInFlight;
}

// NTSC-U/C: 0x0055d9e8, PAL: 0x0059ec60
void ReadOp::Complete() {
    InterpretResult();
    mHandler->OnRead(this);
}

// NTSC-U/C: 0x0055e948, PAL: 0x0059fc18
void ReadOp::InterpretResult() {
    if (mResult >= sceMcResSucceed) {
        mBytesTransferred = mResult;
        mStatus = kMemcardStatusOk;
        return;
    }

    switch (mResult) {
    case sceMcResNoFormat:
        mStatus = kMemcardStatusNotFormatted;
        break;
    case sceMcResNoEntry:
        mStatus = kMemcardStatusBadFile;
        break;
    case sceMcResDeniedPermit:
        mStatus = kMemcardStatusNoEntry;
        break;
    default:
        mStatus = kMemcardStatusUnknown;
        break;
    }
}
