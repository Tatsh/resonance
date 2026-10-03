#include "memcard/memcardop.h"

MemcardOp::MemcardOp(MemcardCBHandler *pHandler, int nPortSlot, int nCookie)
    : mCookie(nCookie), mIssued(kMemcardOpNotIssued), mPortSlot(nPortSlot), mHandler(pHandler) {
}

MemcardOp::MemcardOp(MemcardCBHandler *pHandler, int nCookie)
    : mCookie(nCookie), mIssued(kMemcardOpNotIssued), mHandler(pHandler) {
}

// NTSC-U/C: 0x0055f2e0, PAL: 0x005a05c0
MemcardOp::~MemcardOp() {
}

// NTSC-U/C: 0x0055f310, PAL: 0x005a05f0
void MemcardOp::Execute() {
}

// NTSC-U/C: 0x0055f318, PAL: 0x005a05f8
void MemcardOp::NotifyDone() {
}

// NTSC-U/C: 0x0055f320, PAL: 0x005a0600
void MemcardOp::InterpretResult() {
}
