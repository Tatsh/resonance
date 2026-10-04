#include "memcard/loadglobalsettingsmct.h"

#include "game/globalsettings.h"
#include "memcard/memcard.h"
#include "memcard/memcardop.h"
#include "memcard/memcardsavepaths.h"
#include "memcard/memcarduser.h"

LoadGlobalSettingsMCT::LoadGlobalSettingsMCT(
    MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie, GlobalSettings *pSettings)
    : LoadFileMCT(pUser, pCard, nPortSlot, nCookie),
      mStream(g_abRemixStagingBuffer, kRemixStagingBufferSize), mSettings(pSettings) {
}

LoadGlobalSettingsMCT::~LoadGlobalSettingsMCT() {
}

void LoadGlobalSettingsMCT::Finish() {
    MemcardTask::mState = kMemcardTaskFinished;
    if (mStatus == kMemcardStatusOk) {
        mStream.SetSize(mBytesRead);
        mSettings->Load(mStream);
    }
    mUser->OnGlobalSettingsLoaded(mPortSlot, mStatus);
}

void LoadGlobalSettingsMCT::Execute() {
    MemcardTask::mState = kMemcardTaskRunning;
#ifdef VIDEO_STANDARD_PAL
    mPath = g_saveDirBase + g_globalSettingsDirSuffix + "/" + g_globalSettingsFileName;
#else
    mPath = g_saveDirBase + g_globalSettingsDirSuffix + g_globalSettingsFileName;
#endif
    mBuffer = mStream.mBuffer;
    mLength = mStream.Capacity();
    SetState(kLoadFileStateOpen);
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
