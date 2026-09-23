#include "memcard/savepersonasmct.h"

#include "memcard/checkinfoop.h"
#include "memcard/memcard.h"
#include "memcard/memcardsavepaths.h"
#include "memcard/memcarduser.h"
#include "met/metpersonadata.h"

// 0x00178798
SavePersonasMCT::SavePersonasMCT(MemcardUser *pUser,
                                 Memcard *pCard,
                                 int nPortSlot,
                                 int nCookie,
                                 const std::vector<MetPersonaData *> &roster)
    : SaveFileMCT(pUser, pCard, nPortSlot, nCookie),
      mStream(g_abRemixStagingBuffer, kRemixStagingBufferSize) {
    const int nCount = roster.size();
    mStream.Write(&nCount, sizeof(nCount));
    for (int i = 0; i < nCount; ++i) {
        roster[i]->Save(&mStream);
    }
}

// 0x00184d98
SavePersonasMCT::~SavePersonasMCT() {
}

// 0x001864a0
void SavePersonasMCT::OnCheckInfo(CheckInfoOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus != kMemcardStatusUnknown && mStatus != kMemcardStatusNotFormatted) {
        if (pOp->mFree >= kSaveFileMinimumFreeClusters) {
            mStep = kSaveFileStepCreateDir;
            RunStep();
            return;
        }
        mStatus = kMemcardStatusCardFull;
    }
    if (mStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish();
    }
}

// 0x00186538
void SavePersonasMCT::Finish() {
    mState = kMemcardTaskFinished;
    mUser->OnPersonasSaved(mPortSlot, mStatus);
}

// 0x00178960
void SavePersonasMCT::Execute() {
    mState = kMemcardTaskRunning;
    mDirName = g_saveDirBase + g_personasDirSuffix;
    mFileName = g_personasFileName;
    mIconTitle = g_personasIconTitle;
    mData = mStream.mBuffer;
    mDataLength = mStream.Size();
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
