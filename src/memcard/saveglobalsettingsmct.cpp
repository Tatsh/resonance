#include "memcard/saveglobalsettingsmct.h"

#include "game/globalsettings.h"
#include "memcard/checkinfoop.h"
#include "memcard/memcard.h"
#include "memcard/memcardsavepaths.h"
#include "memcard/memcarduser.h"

// NTSC-U/C: 0x00178ab0, PAL: 0x0017c348
SaveGlobalSettingsMCT::SaveGlobalSettingsMCT(
    MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie, GlobalSettings *pSettings)
    : SaveFileMCT(pUser, pCard, nPortSlot, nCookie),
      mStream(g_abRemixStagingBuffer, kRemixStagingBufferSize) {
    pSettings->Save(mStream);
}

// NTSC-U/C: 0x00184e40, PAL: 0x0018a468
SaveGlobalSettingsMCT::~SaveGlobalSettingsMCT() {
}

#ifndef VIDEO_STANDARD_PAL
// NTSC-U/C: 0x00186578
void SaveGlobalSettingsMCT::OnCheckInfo(CheckInfoOp *pOp) {
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

// NTSC-U/C: 0x00186610, PAL: 0x0018bfa0
void SaveGlobalSettingsMCT::Finish() {
    mState = kMemcardTaskFinished;
#ifdef VIDEO_STANDARD_PAL
    mUser->OnGlobalSettingsSaved(mPortSlot, mStatus, mKilobytes);
#else
    mUser->OnGlobalSettingsSaved(mPortSlot, mStatus);
#endif
}

// NTSC-U/C: 0x00178c08, PAL: 0x0017c590
void SaveGlobalSettingsMCT::Execute() {
    mState = kMemcardTaskRunning;
    mDirName = g_saveDirBase + g_globalSettingsDirSuffix;
#ifdef VIDEO_STANDARD_PAL
    const SaveFileEntry payload = {g_globalSettingsFileName, mStream.mBuffer, mStream.Size()};
    mFiles.push_back(payload);
    mIconTitle = g_globalSettingsIconTitle;
    SaveFileMCT::Execute();
#else
    mFileName = g_globalSettingsFileName;
    mIconTitle = g_globalSettingsIconTitle;
    mData = mStream.mBuffer;
    mDataLength = mStream.Size();
    mCard->CheckInfo(this, mPortSlot, mCookie);
#endif
}
