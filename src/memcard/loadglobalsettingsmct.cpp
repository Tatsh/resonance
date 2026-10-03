#include "memcard/loadglobalsettingsmct.h"

#include "game/globalsettings.h"
#include "memcard/memcard.h"
#include "memcard/memcardop.h"
#include "memcard/memcardsavepaths.h"
#include "memcard/memcarduser.h"

// NTSC-U/C: 0x00179518, PAL: 0x0017d318
LoadGlobalSettingsMCT::LoadGlobalSettingsMCT(
    MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie, GlobalSettings *pSettings)
    : LoadFileMCT(pUser, pCard, nPortSlot, nCookie),
      mStream(g_abRemixStagingBuffer, kRemixStagingBufferSize), mSettings(pSettings) {
}

// NTSC-U/C: 0x00185898, PAL: 0x0018b290
LoadGlobalSettingsMCT::~LoadGlobalSettingsMCT() {
}

// NTSC-U/C: 0x00186728, PAL: 0x0018c020
void LoadGlobalSettingsMCT::Finish() {
    MemcardTask::mState = kMemcardTaskFinished;
    if (mStatus == kMemcardStatusOk) {
        mStream.SetSize(mBytesRead);
        mSettings->Load(mStream);
    }
    mUser->OnGlobalSettingsLoaded(mPortSlot, mStatus);
}

// NTSC-U/C: 0x00179600, PAL: 0x0017d418
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
