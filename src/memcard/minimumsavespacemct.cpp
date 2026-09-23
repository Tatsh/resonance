#include "memcard/minimumsavespacemct.h"

#include "game/globalsettings.h"
#include "memcard/closeop.h"
#include "memcard/memcard.h"
#include "memcard/memcardsavepaths.h"
#include "memcard/memcarduser.h"
#include "memcard/openreadop.h"

namespace {

// Execute() seeds the estimate with the global settings count less this many clusters.
constexpr int kMinimumSaveSpaceSettingsAllowance = 100;

// The enquiry the first step queues addresses port 1 slot 1 rather than the slot the task was
// constructed for, which every later step does use.
constexpr int kMinimumSaveSpaceEnquirySlot = 0;

} // namespace

MinimumSaveSpaceMCT::MinimumSaveSpaceMCT(MemcardUser *pUser,
                                         Memcard *pCard,
                                         int nPortSlot,
                                         int nCookie)
    : MemcardTask(pUser, pCard, nPortSlot, nCookie) {
}

// 0x00184c18
MinimumSaveSpaceMCT::~MinimumSaveSpaceMCT() {
}

// 0x00178328
void MinimumSaveSpaceMCT::Execute() {
    mState = kMemcardTaskRunning;
    mSpace = GlobalSettings::shared()->mUnknown6c - kMinimumSaveSpaceSettingsAllowance;
    mPersonaPath = g_saveDirBase + g_personasDirSuffix + g_personasFileName;
    mSettingsPath = g_saveDirBase + g_globalSettingsDirSuffix + g_globalSettingsFileName;
    mStep = kMinimumSaveSpaceStepCheckInfo;
    RunStep();
}

// 0x00178660
void MinimumSaveSpaceMCT::RunStep() {
    switch (mStep) {
    case kMinimumSaveSpaceStepCheckInfo:
        mCard->CheckInfo(this, kMinimumSaveSpaceEnquirySlot, mCookie);
        mStep = kMinimumSaveSpaceStepOpenPersonas;
        break;

    case kMinimumSaveSpaceStepOpenPersonas:
        mCard->OpenRead(this, mPortSlot, mPersonaPath, mCookie);
        mStep = kMinimumSaveSpaceStepAfterPersonas;
        break;

    case kMinimumSaveSpaceStepAfterPersonas:
        mStep = kMinimumSaveSpaceStepOpenSettings;
        if (mFile >= 0) {
            mCard->Close(this, mFile, mCookie);
            mSpace += kMinimumSaveSpaceExistingFile;
            break;
        }

        mSpace += kMinimumSaveSpaceNewFile;
        mCard->OpenRead(this, mPortSlot, mSettingsPath, mCookie);
        mStep = kMinimumSaveSpaceStepAfterSettings;
        break;

    case kMinimumSaveSpaceStepOpenSettings:
        mSpace += kMinimumSaveSpaceNewFile;
        mCard->OpenRead(this, mPortSlot, mSettingsPath, mCookie);
        mStep = kMinimumSaveSpaceStepAfterSettings;
        break;

    case kMinimumSaveSpaceStepAfterSettings:
        mStep = kMinimumSaveSpaceStepReport;
        if (mFile >= 0) {
            mCard->Close(this, mFile, mCookie);
            break;
        }

        mSpace += kMinimumSaveSpaceNewFile;
        Finish();
        break;

    case kMinimumSaveSpaceStepReport:
        mSpace += kMinimumSaveSpaceNewFile;
        Finish();
        break;

    default:
        break;
    }
}

// 0x00186200
void MinimumSaveSpaceMCT::OnCheckInfo([[maybe_unused]] CheckInfoOp *pOp) {
    RunStep();
}

// 0x00186220
void MinimumSaveSpaceMCT::OnOpenRead(OpenReadOp *pOp) {
    mStatus = pOp->mStatus;
    mFile = pOp->mFile;
    RunStep();
}

// 0x00186250
void MinimumSaveSpaceMCT::OnClose(CloseOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus == kMemcardStatusOk) {
        RunStep();
        return;
    }

    mCard->Cancel(mCookie);
    Finish();
}

// 0x001861c0
void MinimumSaveSpaceMCT::Finish() {
    mState = kMemcardTaskFinished;
    mUser->OnMinimumSaveSpace(mPortSlot, mSpace);
}
