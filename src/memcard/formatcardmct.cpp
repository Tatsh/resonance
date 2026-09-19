#include "memcard/formatcardmct.h"

#include "memcard/checkinfoop.h"
#include "memcard/formatop.h"
#include "memcard/memcard.h"
#include "memcard/memcarduser.h"

FormatCardMCT::FormatCardMCT(MemcardUser *pUser, Memcard *pCard, int nPortSlot, void *pCookie)
    : MemcardTask(pUser, pCard, nPortSlot, pCookie), mUnformat(0) {
}

// 0x00184d68
FormatCardMCT::~FormatCardMCT() {
}

// 0x00186348
void FormatCardMCT::IssueFormat() {
    if (mUnformat == 0) {
        mCard->Format(this, mPortSlot, mCookie);
        return;
    }

    mCard->Unformat(this, mPortSlot, mCookie);
}

// 0x00186390
void FormatCardMCT::OnCheckInfo(CheckInfoOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus != kMemcardStatusUnknown) {
        if (pOp->mFormatted == 0) {
            IssueFormat();
            return;
        }

        mStatus = kMemcardStatusAlreadyFormatted;
    }

    if (mStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish();
    }
}

// 0x00186438
void FormatCardMCT::OnFormat(FormatOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus == kMemcardStatusOk) {
        Finish();
        return;
    }

    mCard->Cancel(mCookie);
    Finish();
}

// 0x001862b0
void FormatCardMCT::Finish() {
    mState = kMemcardTaskFinished;
    if (mUnformat == 0) {
        mUser->OnCardFormatted(mPortSlot, mStatus);
        return;
    }

    mUser->OnCardUnformatted(mPortSlot, mStatus);
}

// 0x00186318
void FormatCardMCT::Execute() {
    mState = kMemcardTaskRunning;
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
