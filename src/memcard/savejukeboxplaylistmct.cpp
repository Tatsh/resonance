#include "memcard/savejukeboxplaylistmct.h"

#include <stdio.h>

#include "game/jukeboxplaylist.h"
#include "memcard/checkinfoop.h"
#include "memcard/memcard.h"
#include "memcard/memcardsavepaths.h"
#include "memcard/memcarduser.h"

namespace {

// Bytes of the stack buffer Execute() formats the playlist number into.
constexpr int kIndexTextSize = 16;

} // namespace

// 0x00178d58
SaveJukeboxPlayListMCT::SaveJukeboxPlayListMCT(MemcardUser *pUser,
                                               Memcard *pCard,
                                               int nPortSlot,
                                               int nCookie,
                                               JukeboxPlayList *pPlayList,
                                               int nIndex)
    : SaveFileMCT(pUser, pCard, nPortSlot, nCookie),
      mStream(g_abRemixStagingBuffer, kRemixStagingBufferSize), mIndex(nIndex) {
    pPlayList->save(&mStream);
}

// 0x00184ee8
SaveJukeboxPlayListMCT::~SaveJukeboxPlayListMCT() {
}

// 0x00186650
void SaveJukeboxPlayListMCT::OnCheckInfo(CheckInfoOp *pOp) {
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

// 0x001866e8
void SaveJukeboxPlayListMCT::Finish() {
    mState = kMemcardTaskFinished;
    mUser->OnJukeboxPlayListSaved(mPortSlot, mStatus);
}

// 0x00178ec0
void SaveJukeboxPlayListMCT::Execute() {
    mState = kMemcardTaskRunning;
    mDirName = g_saveDirBase + g_jukeboxDirSuffix;
    char szIndex[kIndexTextSize];
    sprintf(szIndex, "%d", mIndex);
    mFileName = g_jukeboxFileName + szIndex + ".dat";
    mIconTitle = g_jukeboxIconTitle;
    mData = mStream.mBuffer;
    mDataLength = mStream.Size();
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
