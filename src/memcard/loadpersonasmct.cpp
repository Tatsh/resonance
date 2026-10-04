#include "memcard/loadpersonasmct.h"

#include "memcard/memcard.h"
#include "memcard/memcardop.h"
#include "memcard/memcardsavepaths.h"
#include "memcard/memcarduser.h"
#include "met/metpersonadata.h"
#include "stream/iobpreallocmemstream.h"

LoadPersonasMCT::LoadPersonasMCT(MemcardUser *pUser,
                                 Memcard *pCard,
                                 int nPortSlot,
                                 int nCookie,
                                 std::vector<MetPersonaData *> *pRoster)
    : LoadFileMCT(pUser, pCard, nPortSlot, nCookie), mRoster(pRoster) {
}

LoadPersonasMCT::~LoadPersonasMCT() {
}

void LoadPersonasMCT::Finish() {
    MemcardTask::mState = kMemcardTaskFinished;
    if (mStatus == kMemcardStatusOk) {
        IOBPreallocMemStream stream(static_cast<char *>(mBuffer), kRemixStagingBufferSize);
        stream.SetSize(mBytesRead);
        int nCount;
        stream.ReadLE(&nCount, sizeof(nCount));
        for (int i = 0; i < nCount; ++i) {
            MetPersonaData *pPersona = new MetPersonaData;
            pPersona->Load(&stream);
            mRoster->push_back(pPersona);
        }
    }
    mUser->OnPersonasLoaded(mPortSlot, mStatus);
}

void LoadPersonasMCT::Execute() {
    MemcardTask::mState = kMemcardTaskRunning;
#ifdef VIDEO_STANDARD_PAL
    mPath = g_saveDirBase + g_personasDirSuffix + "/" + g_personasFileName;
#else
    mPath = g_saveDirBase + g_personasDirSuffix + g_personasFileName;
#endif
    mLength = kRemixStagingBufferSize;
    mBuffer = g_abRemixStagingBuffer;
    SetState(kLoadFileStateOpen);
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
