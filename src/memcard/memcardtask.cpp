#include "memcard/memcardtask.h"

MemcardTask::MemcardTask(MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie)
    : mUser(pUser), mCard(pCard), mCookie(nCookie), mPortSlot(nPortSlot), mState(kMemcardTaskIdle) {
}

// 0x00184530
void MemcardTask::OnUnknown18() {
}
