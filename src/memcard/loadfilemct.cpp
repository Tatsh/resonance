#include "memcard/loadfilemct.h"

LoadFileMCT::LoadFileMCT(MemcardUser *pUser, Memcard *pCard, int nPortSlot, void *pCookie)
    : MemcardTask(pUser, pCard, nPortSlot, pCookie) {
}

// 0x001847e0
LoadFileMCT::~LoadFileMCT() {
}

// 0x00185e20
void LoadFileMCT::Load(const HxStr &path, void *pBuffer, int nLength) {
    mPath = path;
    mBuffer = pBuffer;
    mLength = nLength;
    Execute();
}

// 0x001f61b0
void LoadFileMCT::SetState(int nState) {
    mState = nState;
}

// 0x00185f08
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

// 0x00185db8
void LoadFileMCT::OnCheckInfo(CheckInfoOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus != kMemcardStatusUnknown) {
        RunStep();
        return;
    }

    mCard->Cancel(mCookie);
    Finish();
}

// 0x00185d00
void LoadFileMCT::OnRead(ReadOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus == kMemcardStatusOk) {
        mBytesRead = pOp->mBytesTransferred;
        return;
    }

    mCard->Cancel(mCookie);
    Finish();
}

// 0x00185ca0
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

// 0x00185d58
void LoadFileMCT::OnClose(CloseOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus == kMemcardStatusOk) {
        RunStep();
        return;
    }

    mCard->Cancel(mCookie);
    Finish();
}

// 0x00185ec0
void LoadFileMCT::Finish() {
    SetState(kLoadFileStateDone);
    mUser->OnFileLoaded(mStatus);
}

// 0x00185e80
void LoadFileMCT::Execute() {
    SetState(kLoadFileStateOpen);
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
