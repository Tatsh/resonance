#include "memcard/memcardtask.h"

MemcardTask::MemcardTask(MemcardUser *pUser, Memcard *pCard, int nPortSlot, void *pCookie)
    : mUser(pUser), mCard(pCard), mCookie(pCookie), mPortSlot(nPortSlot), mState(kMemcardTaskIdle) {
}

// 0x00184530
void MemcardTask::OnUnknown18() {
}
