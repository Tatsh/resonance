#include "memcard/memcardop.h"

MemcardOp::MemcardOp(MemcardCBHandler *pHandler, int nPortSlot, int nCookie)
    : mCookie(nCookie), mIssued(kMemcardOpNotIssued), mPortSlot(nPortSlot), mHandler(pHandler) {
}

MemcardOp::MemcardOp(MemcardCBHandler *pHandler, int nCookie)
    : mCookie(nCookie), mIssued(kMemcardOpNotIssued), mHandler(pHandler) {
}

MemcardOp::~MemcardOp() {
}

void MemcardOp::Execute() {
}

void MemcardOp::NotifyDone() {
}

void MemcardOp::InterpretResult() {
}
