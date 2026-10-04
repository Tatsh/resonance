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

SaveJukeboxPlayListMCT::~SaveJukeboxPlayListMCT() {
}

#ifndef VIDEO_STANDARD_PAL
void SaveJukeboxPlayListMCT::OnCheckInfo(CheckInfoOp *pOp) {
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

void SaveJukeboxPlayListMCT::Finish() {
    mState = kMemcardTaskFinished;
#ifdef VIDEO_STANDARD_PAL
    mUser->OnJukeboxPlayListSaved(mPortSlot, mStatus, mKilobytes);
#else
    mUser->OnJukeboxPlayListSaved(mPortSlot, mStatus);
#endif
}

void SaveJukeboxPlayListMCT::Execute() {
    mState = kMemcardTaskRunning;
    mDirName = g_saveDirBase + g_jukeboxDirSuffix;
    char szIndex[kIndexTextSize];
    sprintf(szIndex, "%d", mIndex);
#ifdef VIDEO_STANDARD_PAL
    mIconTitle = g_jukeboxIconTitle;
    const SaveFileEntry payload = {
        g_jukeboxFileName + szIndex + ".dat", mStream.mBuffer, mStream.Size()};
    mFiles.push_back(payload);
    SaveFileMCT::Execute();
#else
    mFileName = g_jukeboxFileName + szIndex + ".dat";
    mIconTitle = g_jukeboxIconTitle;
    mData = mStream.mBuffer;
    mDataLength = mStream.Size();
    mCard->CheckInfo(this, mPortSlot, mCookie);
#endif
}
