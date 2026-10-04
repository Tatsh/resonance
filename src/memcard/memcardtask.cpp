#include "memcard/memcardtask.h"

#include "memcard/memcard.h"
#include "memcard/memcardop.h"

MemcardTask::MemcardTask(MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie)
    : mUser(pUser), mCard(pCard), mCookie(nCookie), mPortSlot(nPortSlot), mState(kMemcardTaskIdle) {
}

#ifndef VIDEO_STANDARD_PAL
void MemcardTask::UnusedHook() {
}
#endif

void MemcardTask::HandleError() {
    if (mStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish();
    }
}
