#include "memcard/savepersonasmct.h"

#include "memcard/checkinfoop.h"
#include "memcard/memcard.h"
#include "memcard/memcardsavepaths.h"
#include "memcard/memcarduser.h"
#include "met/metpersonadata.h"

// NTSC-U/C: 0x00178798, PAL: 0x0017be58
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

// NTSC-U/C: 0x00184d98, PAL: 0x0018a2e8
SavePersonasMCT::~SavePersonasMCT() {
}

#ifndef VIDEO_STANDARD_PAL
// NTSC-U/C: 0x001864a0
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

// NTSC-U/C: 0x00186538, PAL: 0x0018bf60
void SavePersonasMCT::Finish() {
    mState = kMemcardTaskFinished;
#ifdef VIDEO_STANDARD_PAL
    mUser->OnPersonasSaved(mPortSlot, mStatus, mKilobytes);
#else
    mUser->OnPersonasSaved(mPortSlot, mStatus);
#endif
}

// NTSC-U/C: 0x00178960, PAL: 0x0017c110
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
