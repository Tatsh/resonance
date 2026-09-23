#include "memcard/loadglobalsettingsmct.h"

#include "game/globalsettings.h"
#include "memcard/memcard.h"
#include "memcard/memcardop.h"
#include "memcard/memcardsavepaths.h"
#include "memcard/memcarduser.h"

// 0x00179518
LoadGlobalSettingsMCT::LoadGlobalSettingsMCT(
    MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie, GlobalSettings *pSettings)
    : LoadFileMCT(pUser, pCard, nPortSlot, nCookie),
      mStream(g_abRemixStagingBuffer, kRemixStagingBufferSize), mSettings(pSettings) {
}

// 0x00185898
LoadGlobalSettingsMCT::~LoadGlobalSettingsMCT() {
}

// 0x00186728
void LoadGlobalSettingsMCT::Finish() {
    MemcardTask::mState = kMemcardTaskFinished;
    if (mStatus == kMemcardStatusOk) {
        mStream.SetSize(mBytesRead);
        mSettings->Load(mStream);
    }
    mUser->OnGlobalSettingsLoaded(mPortSlot, mStatus);
}

// 0x00179600
void LoadGlobalSettingsMCT::Execute() {
    MemcardTask::mState = kMemcardTaskRunning;
    mPath = g_saveDirBase + g_globalSettingsDirSuffix + g_globalSettingsFileName;
    mBuffer = mStream.mBuffer;
    mLength = mStream.Capacity();
    SetState(kLoadFileStateOpen);
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
