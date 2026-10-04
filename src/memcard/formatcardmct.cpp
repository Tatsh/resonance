#include "memcard/formatcardmct.h"

#include "memcard/checkinfoop.h"
#include "memcard/formatop.h"
#include "memcard/memcard.h"
#include "memcard/memcarduser.h"

FormatCardMCT::FormatCardMCT(
    MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie, int bUnformat)
    : MemcardTask(pUser, pCard, nPortSlot, nCookie), mUnformat(bUnformat) {
}

FormatCardMCT::~FormatCardMCT() {
}

void FormatCardMCT::IssueFormat() {
    if (mUnformat == 0) {
        mCard->Format(this, mPortSlot, mCookie);
        return;
    }

    mCard->Unformat(this, mPortSlot, mCookie);
}

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

void FormatCardMCT::OnFormat(FormatOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus == kMemcardStatusOk) {
        Finish();
        return;
    }

    mCard->Cancel(mCookie);
    Finish();
}

void FormatCardMCT::Finish() {
    mState = kMemcardTaskFinished;
    if (mUnformat == 0) {
        mUser->OnCardFormatted(mPortSlot, mStatus);
        return;
    }

    mUser->OnCardUnformatted(mPortSlot, mStatus);
}

void FormatCardMCT::Execute() {
    mState = kMemcardTaskRunning;
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
