#include "memcard/saveglobalsettingsmct.h"

#include "game/globalsettings.h"
#include "memcard/checkinfoop.h"
#include "memcard/memcard.h"
#include "memcard/memcardsavepaths.h"
#include "memcard/memcarduser.h"

// 0x00178ab0
SaveGlobalSettingsMCT::SaveGlobalSettingsMCT(
    MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie, GlobalSettings *pSettings)
    : SaveFileMCT(pUser, pCard, nPortSlot, nCookie),
      mStream(g_abRemixStagingBuffer, kRemixStagingBufferSize) {
    pSettings->Save(mStream);
}

// 0x00184e40
SaveGlobalSettingsMCT::~SaveGlobalSettingsMCT() {
}

// 0x00186578
void SaveGlobalSettingsMCT::OnCheckInfo(CheckInfoOp *pOp) {
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

// 0x00186610
void SaveGlobalSettingsMCT::Finish() {
    mState = kMemcardTaskFinished;
    mUser->OnGlobalSettingsSaved(mPortSlot, mStatus);
}

// 0x00178c08
void SaveGlobalSettingsMCT::Execute() {
    mState = kMemcardTaskRunning;
    mDirName = g_saveDirBase + g_globalSettingsDirSuffix;
    mFileName = g_globalSettingsFileName;
    mIconTitle = g_globalSettingsIconTitle;
    mData = mStream.mBuffer;
    mDataLength = mStream.Size();
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
