#include "memcard/memcardtask.h"

#include "memcard/memcard.h"
#include "memcard/memcardop.h"

MemcardTask::MemcardTask(MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie)
    : mUser(pUser), mCard(pCard), mCookie(nCookie), mPortSlot(nPortSlot), mState(kMemcardTaskIdle) {
}

#ifndef VIDEO_STANDARD_PAL
// NTSC-U/C: 0x00184530
void MemcardTask::UnusedHook() {
}
#endif

// NTSC-U/C: 0x00185998, PAL: 0x0018b3c0
void MemcardTask::AbortOnError() {
    if (mStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish();
    }
}
