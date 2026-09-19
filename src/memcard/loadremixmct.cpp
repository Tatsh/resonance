#include "memcard/loadremixmct.h"

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

} // namespace

LoadRemixMCT::~LoadRemixMCT() {
}

void LoadRemixMCT::ListRemixDir() {
    mStep = 0;
    HxStr pattern(g_saveDirBase);
    pattern += g_remixDirSuffix;
    pattern += kAnyDirectory;
    mCard->ListDir(this, mPortSlot, pattern, mCookie, kListDirModeFresh);
}

void LoadRemixMCT::OnCheckInfo(CheckInfoOp *pOp) {
    mStatus = pOp->mStatus;
    if (pOp->mStatus != kMemcardStatusUnknown && pOp->mStatus != kMemcardStatusNotFormatted) {
        ListRemixDir();
        return;
    }
    if (pOp->mStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish();
    }
}

void LoadRemixMCT::Finish() {
    mState = kMemcardTaskFinished;
    mUser->OnRemixLoaded(mPortSlot, mStatus);
}

void LoadRemixMCT::Execute() {
    mState = kMemcardTaskRunning;
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
