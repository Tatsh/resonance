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

// 0x001797e8
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

// 0x00185918
LoadJukeboxPlayListMCT::~LoadJukeboxPlayListMCT() {
}

// 0x001867a8
void LoadJukeboxPlayListMCT::Finish() {
    MemcardTask::mState = kMemcardTaskFinished;
    if (mStatus == kMemcardStatusOk) {
        mStream.SetSize(mBytesRead);
        mPlayList->load(&mStream);
    }
    mUser->OnJukeboxPlayListLoaded(mPortSlot, mStatus);
}

// 0x001798e0
void LoadJukeboxPlayListMCT::Execute() {
    MemcardTask::mState = kMemcardTaskRunning;
    char szIndex[kIndexTextSize];
    sprintf(szIndex, "%d", mIndex);
    mPath = g_saveDirBase + g_jukeboxDirSuffix + g_jukeboxFileName + szIndex + ".dat";
    mBuffer = mStream.mBuffer;
    mLength = mStream.Capacity();
    SetState(kLoadFileStateOpen);
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
