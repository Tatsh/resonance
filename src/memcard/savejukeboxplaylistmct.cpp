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

// NTSC-U/C: 0x00178d58, PAL: 0x0017c7c8
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

// NTSC-U/C: 0x00184ee8, PAL: 0x0018a5e8
SaveJukeboxPlayListMCT::~SaveJukeboxPlayListMCT() {
}

#ifndef VIDEO_STANDARD_PAL
// NTSC-U/C: 0x00186650
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

// NTSC-U/C: 0x001866e8, PAL: 0x0018bfe0
void SaveJukeboxPlayListMCT::Finish() {
    mState = kMemcardTaskFinished;
#ifdef VIDEO_STANDARD_PAL
    mUser->OnJukeboxPlayListSaved(mPortSlot, mStatus, mKilobytes);
#else
    mUser->OnJukeboxPlayListSaved(mPortSlot, mStatus);
#endif
}

// NTSC-U/C: 0x00178ec0, PAL: 0x0017ca28
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
