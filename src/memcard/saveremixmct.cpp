#include "memcard/saveremixmct.h"

#include "memcard/checkinfoop.h"
#include "memcard/memcard.h"
#include "memcard/memcardop.h"
#include "memcard/memcardsavepaths.h"
#include "os/hxstr.h"

namespace {

// Wildcard appended to the save directory prefix so the listing enumerates every remix directory.
static const char *const kAnyDirectory = "*";

// The sceMcGetDir() mode that starts a fresh listing rather than continuing the previous one.
constexpr unsigned kListDirModeFresh = 0;

// The step OnFileSaved() rewrites the index after.
constexpr int kSaveRemixStepPayloadWritten = 3;

// The step the index rewrite runs under.
constexpr int kSaveRemixStepWriteIndex = 4;

} // namespace

SaveRemixMCT::~SaveRemixMCT() {
}

void SaveRemixMCT::ListRemixDir() {
    HxStr pattern(g_saveDirBase);
    pattern += g_remixDirSuffix;
    pattern += kAnyDirectory;
    mCard->ListDir(this, mPortSlot, pattern, mCookie, kListDirModeFresh);
}

void SaveRemixMCT::OnCheckInfo(CheckInfoOp *pOp) {
    mStatus = pOp->mStatus;
    if (pOp->mStatus != kMemcardStatusUnknown && pOp->mStatus != kMemcardStatusNotFormatted) {
        if (pOp->mFree >= kRemixSaveMinimumFreeClusters) {
            ListRemixDir();
            return;
        }
        mStatus = kMemcardStatusCardFull;
    }
    if (mStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish();
    }
}

void SaveRemixMCT::OnFileSaved(int nStatus) {
    mStatus = nStatus;
    if (nStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish();
        return;
    }
    if (mStep == kSaveRemixStepPayloadWritten) {
        mStep = kSaveRemixStepWriteIndex;
        WriteIndex();
        return;
    }
    Finish();
}

void SaveRemixMCT::Finish() {
    mState = kMemcardTaskFinished;
    mUser->OnRemixSaved(mPortSlot, mStatus);
}

void SaveRemixMCT::Execute() {
    mState = kMemcardTaskRunning;
    mStep = 0;
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
