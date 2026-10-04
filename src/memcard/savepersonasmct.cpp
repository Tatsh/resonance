#include "memcard/savepersonasmct.h"

#include "memcard/checkinfoop.h"
#include "memcard/memcard.h"
#include "memcard/memcardsavepaths.h"
#include "memcard/memcarduser.h"
#include "met/metpersonadata.h"

SavePersonasMCT::SavePersonasMCT(MemcardUser *pUser,
                                 Memcard *pCard,
                                 int nPortSlot,
                                 int nCookie,
                                 const std::vector<MetPersonaData *> &roster)
    : SaveFileMCT(pUser, pCard, nPortSlot, nCookie),
      mStream(g_abRemixStagingBuffer, kRemixStagingBufferSize) {
    const int nCount = roster.size();
    mStream.WriteLE(&nCount, sizeof(nCount));
    for (int i = 0; i < nCount; ++i) {
        roster[i]->Save(&mStream);
    }
}

SavePersonasMCT::~SavePersonasMCT() {
}

#ifndef VIDEO_STANDARD_PAL
void SavePersonasMCT::OnCheckInfo(CheckInfoOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus != kMemcardStatusUnknown && mStatus != kMemcardStatusNotFormatted) {
        if (static_cast<unsigned>(pOp->mFree) >= kSaveFileMinimumFreeClusters) {
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
#endif

void SavePersonasMCT::Finish() {
    mState = kMemcardTaskFinished;
#ifdef VIDEO_STANDARD_PAL
    mUser->OnPersonasSaved(mPortSlot, mStatus, mKilobytes);
#else
    mUser->OnPersonasSaved(mPortSlot, mStatus);
#endif
}

void SavePersonasMCT::Execute() {
    mState = kMemcardTaskRunning;
    mDirName = g_saveDirBase + g_personasDirSuffix;
#ifdef VIDEO_STANDARD_PAL
    const SaveFileEntry payload = {g_personasFileName, mStream.mBuffer, mStream.Size()};
    mFiles.push_back(payload);
    mIconTitle = g_personasIconTitle;
    SaveFileMCT::Execute();
#else
    mFileName = g_personasFileName;
    mIconTitle = g_personasIconTitle;
    mData = mStream.mBuffer;
    mDataLength = mStream.Size();
    mCard->CheckInfo(this, mPortSlot, mCookie);
#endif
}
