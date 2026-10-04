#include "memcard/loadfilemct.h"

#include "memcard/checkinfoop.h"
#include "memcard/closeop.h"
#include "memcard/memcard.h"
#include "memcard/memcarduser.h"
#include "memcard/openreadop.h"
#include "memcard/readop.h"

LoadFileMCT::LoadFileMCT(MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie)
    : MemcardTask(pUser, pCard, nPortSlot, nCookie) {
}

LoadFileMCT::~LoadFileMCT() {
}

void LoadFileMCT::Load(const HxStr &path, void *pBuffer, int nLength) {
    mPath = path;
    mBuffer = pBuffer;
    mLength = nLength;
    Execute();
}

void LoadFileMCT::SetState(int nState) {
    mState = nState;
}

void LoadFileMCT::RunStep() {
    switch (mState) {
    case kLoadFileStateOpen:
        mCard->OpenRead(this, mPortSlot, mPath, mCookie);
        SetState(kLoadFileStateRead);
        break;

    case kLoadFileStateRead:
        SetState(kLoadFileStateReport);
        mCard->Read(this, mPortSlot, mFile, mBuffer, mLength, mCookie);
        mCard->Close(this, mFile, mCookie);
        break;

    case kLoadFileStateReport:
        Finish();
        break;

    default:
        break;
    }
}

void LoadFileMCT::OnCheckInfo(CheckInfoOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus != kMemcardStatusUnknown) {
        RunStep();
        return;
    }

    mCard->Cancel(mCookie);
    Finish();
}

void LoadFileMCT::OnRead(ReadOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus == kMemcardStatusOk) {
        mBytesRead = pOp->mBytesTransferred;
        return;
    }

    mCard->Cancel(mCookie);
    Finish();
}

void LoadFileMCT::OnOpenRead(OpenReadOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus == kMemcardStatusOk) {
        mFile = pOp->mFile;
        RunStep();
        return;
    }

    mCard->Cancel(mCookie);
    Finish();
}

void LoadFileMCT::OnClose(CloseOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus == kMemcardStatusOk) {
        RunStep();
        return;
    }

    mCard->Cancel(mCookie);
    Finish();
}

void LoadFileMCT::Finish() {
    SetState(kLoadFileStateDone);
    mUser->OnFileLoaded(mStatus);
}

void LoadFileMCT::Execute() {
    SetState(kLoadFileStateOpen);
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
