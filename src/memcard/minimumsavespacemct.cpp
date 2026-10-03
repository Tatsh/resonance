#include "memcard/minimumsavespacemct.h"

#include "game/globalsettings.h"
#include "memcard/closeop.h"
#include "memcard/memcard.h"
#include "memcard/memcardsavepaths.h"
#include "memcard/memcarduser.h"
#include "memcard/openreadop.h"

namespace {

// Execute() seeds the estimate with the global settings count less this many clusters.
#ifdef VIDEO_STANDARD_PAL
constexpr int kMinimumSaveSpaceSettingsAllowance = 87;

// Joins each directory to its file name. The European file names have no leading `/`.
const char kPathSeparator[] = "/";
#else
constexpr int kMinimumSaveSpaceSettingsAllowance = 100;
#endif

// The enquiry the first step queues addresses port 1 slot 1 rather than the slot the task was
// constructed for. Every later step uses the constructed slot.
constexpr int kMinimumSaveSpaceEnquirySlot = 0;

} // namespace

MinimumSaveSpaceMCT::MinimumSaveSpaceMCT(MemcardUser *pUser,
                                         Memcard *pCard,
                                         int nPortSlot,
                                         int nCookie)
#ifdef VIDEO_STANDARD_PAL
    : MemcardTask(pUser, pCard, nPortSlot, nCookie), mSkipWarning(0), mCampaign(0) {
#else
    : MemcardTask(pUser, pCard, nPortSlot, nCookie) {
#endif
}

// NTSC-U/C: 0x00184c18, PAL: 0x0018a140
MinimumSaveSpaceMCT::~MinimumSaveSpaceMCT() {
}

// NTSC-U/C: 0x00178328, PAL: 0x0017b740
void MinimumSaveSpaceMCT::Execute() {
    mState = kMemcardTaskRunning;
    mSpace = GlobalSettings::shared()->mRequiredSaveSpace - kMinimumSaveSpaceSettingsAllowance;
#ifdef VIDEO_STANDARD_PAL
    mPersonaPath = g_saveDirBase + g_personasDirSuffix + kPathSeparator + g_personasFileName;
    mSettingsPath =
        g_saveDirBase + g_globalSettingsDirSuffix + kPathSeparator + g_globalSettingsFileName;
#else
    mPersonaPath = g_saveDirBase + g_personasDirSuffix + g_personasFileName;
    mSettingsPath = g_saveDirBase + g_globalSettingsDirSuffix + g_globalSettingsFileName;
#endif
    mStep = kMinimumSaveSpaceStepCheckInfo;
    RunStep();
}

// NTSC-U/C: 0x00178660, PAL: 0x0017bd20
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
        Finish();
        break;

    default:
        break;
    }
}

// NTSC-U/C: 0x00186200, PAL: 0x0018bcc0
void MinimumSaveSpaceMCT::OnCheckInfo([[maybe_unused]] CheckInfoOp *pOp) {
    RunStep();
}

// NTSC-U/C: 0x00186220, PAL: 0x0018bce0
void MinimumSaveSpaceMCT::OnOpenRead(OpenReadOp *pOp) {
    mStatus = pOp->mStatus;
    mFile = pOp->mFile;
    RunStep();
}

// NTSC-U/C: 0x00186250, PAL: 0x0018bd10
void MinimumSaveSpaceMCT::OnClose(CloseOp *pOp) {
    mStatus = pOp->mStatus;
    if (mStatus == kMemcardStatusOk) {
        RunStep();
        return;
    }

    mCard->Cancel(mCookie);
    Finish();
}

// NTSC-U/C: 0x001861c0, PAL: 0x0018bc78
void MinimumSaveSpaceMCT::Finish() {
    mState = kMemcardTaskFinished;
#ifdef VIDEO_STANDARD_PAL
    mUser->OnMinimumSaveSpace(mPortSlot, mSpace, mSkipWarning, mCampaign);
#else
    mUser->OnMinimumSaveSpace(mPortSlot, mSpace);
#endif
}
