#include "memcard/loadjukeboxplaylistmct.h"

#include <stdio.h>

#include "game/jukeboxplaylist.h"
#include "memcard/memcard.h"
#include "memcard/memcardop.h"
#include "memcard/memcardsavepaths.h"
#include "memcard/memcarduser.h"

namespace {

// Bytes of the stack buffer Execute() formats the playlist number into.
constexpr int kIndexTextSize = 16;

} // namespace

LoadJukeboxPlayListMCT::LoadJukeboxPlayListMCT(MemcardUser *pUser,
                                               Memcard *pCard,
                                               int nPortSlot,
                                               int nCookie,
                                               JukeboxPlayList *pPlayList,
                                               int nIndex)
    : LoadFileMCT(pUser, pCard, nPortSlot, nCookie),
      mStream(g_abRemixStagingBuffer, kRemixStagingBufferSize), mPlayList(pPlayList),
      mIndex(nIndex) {
}

LoadJukeboxPlayListMCT::~LoadJukeboxPlayListMCT() {
}

void LoadJukeboxPlayListMCT::Finish() {
    MemcardTask::mState = kMemcardTaskFinished;
    if (mStatus == kMemcardStatusOk) {
        mStream.SetSize(mBytesRead);
        mPlayList->load(&mStream);
    }
    mUser->OnJukeboxPlayListLoaded(mPortSlot, mStatus);
}

void LoadJukeboxPlayListMCT::Execute() {
    MemcardTask::mState = kMemcardTaskRunning;
    char szIndex[kIndexTextSize];
    sprintf(szIndex, "%d", mIndex);
#ifdef VIDEO_STANDARD_PAL
    mPath = g_saveDirBase + g_jukeboxDirSuffix + "/" + g_jukeboxFileName + szIndex + ".dat";
#else
    mPath = g_saveDirBase + g_jukeboxDirSuffix + g_jukeboxFileName + szIndex + ".dat";
#endif
    mBuffer = mStream.mBuffer;
    mLength = mStream.Capacity();
    SetState(kLoadFileStateOpen);
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
