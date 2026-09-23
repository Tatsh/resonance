#include "memcard/memcardtask.h"

#include "memcard/memcard.h"
#include "memcard/memcardop.h"

MemcardTask::MemcardTask(MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie)
    : mUser(pUser), mCard(pCard), mCookie(nCookie), mPortSlot(nPortSlot), mState(kMemcardTaskIdle) {
}

// 0x00184530
void MemcardTask::OnUnknown18() {
}

// 0x00185998
void MemcardTask::AbortOnError() {
    if (mStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish();
    }
}
