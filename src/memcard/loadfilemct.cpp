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

// NTSC-U/C: 0x001847e0, PAL: 0x00189cd8
LoadFileMCT::~LoadFileMCT() {
}

// NTSC-U/C: 0x00185e20, PAL: 0x0018b8d8
void LoadFileMCT::Load(const HxStr &path, void *pBuffer, int nLength) {
    mPath = path;
    mBuffer = pBuffer;
    mLength = nLength;
    Execute();
}

// NTSC-U/C: 0x001f61b0, PAL: 0x001fcbc0
void LoadFileMCT::SetState(int nState) {
    mState = nState;
}

// NTSC-U/C: 0x00185f08, PAL: 0x0018b9c0
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

// NTSC-U/C: 0x00185db8, PAL: 0x0018b870
void LoadFileMCT::OnCheckInfo(CheckInfoOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus != kMemcardStatusUnknown) {
        RunStep();
        return;
    }

    mCard->Cancel(mCookie);
    Finish();
}

// NTSC-U/C: 0x00185d00, PAL: 0x0018b7b8
void LoadFileMCT::OnRead(ReadOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus == kMemcardStatusOk) {
        mBytesRead = pOp->mBytesTransferred;
        return;
    }

    mCard->Cancel(mCookie);
    Finish();
}

// NTSC-U/C: 0x00185ca0, PAL: 0x0018b758
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

// NTSC-U/C: 0x00185d58, PAL: 0x0018b810
void LoadFileMCT::OnClose(CloseOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus == kMemcardStatusOk) {
        RunStep();
        return;
    }

    mCard->Cancel(mCookie);
    Finish();
}

// NTSC-U/C: 0x00185ec0, PAL: 0x0018b978
void LoadFileMCT::Finish() {
    SetState(kLoadFileStateDone);
    mUser->OnFileLoaded(mStatus);
}

// NTSC-U/C: 0x00185e80, PAL: 0x0018b938
void LoadFileMCT::Execute() {
    SetState(kLoadFileStateOpen);
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
