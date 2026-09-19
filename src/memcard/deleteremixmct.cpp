#include "memcard/deleteremixmct.h"

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

DeleteRemixMCT::DeleteRemixMCT(
    MemcardUser *pUser, Memcard *pCard, int nPortSlot, void *pCookie, const HxStr &remixName)
    : MemcardTask(pUser, pCard, nPortSlot, pCookie), mRemixName(remixName),
      mStream(g_abRemixStagingBuffer, kRemixStagingBufferSize) {
    // The binary reads IOBPreallocMemStream::mBuffer directly. The accessor stands in for it
    // because that member is declared private.
    mBuffer = mStream.Buffer();
}

DeleteRemixMCT::~DeleteRemixMCT() {
}

void DeleteRemixMCT::ListRemixDir() {
    mStep = 0;
    HxStr pattern(g_saveDirBase);
    pattern += g_remixDirSuffix;
    pattern += kAnyDirectory;
    mCard->ListDir(this, mPortSlot, pattern, mCookie, kListDirModeFresh);
}

void DeleteRemixMCT::OnCheckInfo(CheckInfoOp *pOp) {
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

void DeleteRemixMCT::OnDeleteFile(DeleteFileOp *pOp) {
    mStatus = pOp->mStatus;
    if (pOp->mStatus != kMemcardStatusOk) {
        mCard->Cancel(mCookie);
        Finish();
    }
    // Yes, the next step runs even after the abandon path above has reported the task finished.
    DeleteNextFile();
}

void DeleteRemixMCT::OnFileSaved(int nStatus) {
    mStatus = nStatus;
    DeleteNextFile();
}

void DeleteRemixMCT::Finish() {
    mState = kMemcardTaskFinished;
    mUser->OnRemixDeleted(mPortSlot, mStatus);
}

void DeleteRemixMCT::Execute() {
    mState = kMemcardTaskRunning;
    mCard->CheckInfo(this, mPortSlot, mCookie);
}
