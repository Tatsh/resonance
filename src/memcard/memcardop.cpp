#include "memcard/memcardop.h"

MemcardOp::MemcardOp(MemcardCBHandler *pHandler, int nPortSlot, void *pCookie)
    : mCookie(pCookie), mIssued(kMemcardOpNotIssued), mPortSlot(nPortSlot), mHandler(pHandler) {
}

MemcardOp::MemcardOp(MemcardCBHandler *pHandler, void *pCookie)
    : mCookie(pCookie), mIssued(kMemcardOpNotIssued), mHandler(pHandler) {
}

// 0x0055f2e0
MemcardOp::~MemcardOp() {
}

// 0x0055f310
void MemcardOp::Issue() {
}

// 0x0055f318
void MemcardOp::Complete() {
}

// 0x0055f320
void MemcardOp::InterpretResult() {
}
