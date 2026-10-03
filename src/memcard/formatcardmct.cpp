#include "memcard/formatcardmct.h"

#include "memcard/checkinfoop.h"
#include "memcard/formatop.h"
#include "memcard/memcard.h"
#include "memcard/memcarduser.h"

FormatCardMCT::FormatCardMCT(
    MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie, int bUnformat)
    : MemcardTask(pUser, pCard, nPortSlot, nCookie), mUnformat(bUnformat) {
}

// NTSC-U/C: 0x00184d68, PAL: 0x0018a2b8
FormatCardMCT::~FormatCardMCT() {
}

// NTSC-U/C: 0x00186348, PAL: 0x0018be08
void FormatCardMCT::IssueFormat() {
    if (mUnformat == 0) {
        mCard->Format(this, mPortSlot, mCookie);
        return;
    }

    mCard->Unformat(this, mPortSlot, mCookie);
}

// NTSC-U/C: 0x00186390, PAL: 0x0018be50
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

// NTSC-U/C: 0x00186438, PAL: 0x0018bef8
void FormatCardMCT::OnFormat(FormatOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus == kMemcardStatusOk) {
        Finish();
        return;
    }

    mCard->Cancel(mCookie);
    Finish();
}

// NTSC-U/C: 0x001862b0, PAL: 0x0018bd70
void FormatCardMCT::Finish() {
    mState = kMemcardTaskFinished;
    if (mUnformat == 0) {
        mUser->OnCardFormatted(mPortSlot, mStatus);
        return;
    }

    mUser->OnCardUnformatted(mPortSlot, mStatus);
}

// NTSC-U/C: 0x00186318, PAL: 0x0018bdd8
void FormatCardMCT::Execute() {
    mState = kMemcardTaskRunning;
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
